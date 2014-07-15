#include "glr-layer.h"

#include "glr-batch.h"
#include "glr-tex-cache.h"
#include "glr-priv.h"
#include <math.h>
#include <sys/syscall.h>
#include <string.h>
#include <unistd.h>

#define EQUALS(x, y)   (fabs (x - y) <= DBL_EPSILON * fabs (x + y))
#define UNEQUALS(x, y) (fabs (x - y)  > DBL_EPSILON * fabs (x + y))

#define CHECK_LAYER_NOT_FINSHED(self) \
  if (self->finished) return g_warning ("Layer has been finished. Need to call redraw")

#define INSTANCE_TYPE_MASK      0xC3FFFFFF
#define BORDER_NUM_SAMPLES_MASK 0xFF0FFFFF

typedef struct
{
  gfloat left;
  gfloat top;
  gfloat width;
  gfloat height;
} GlrRect;

struct _GlrLayer
{
  gint ref_count;

  pid_t tid;

  GlrContext *context;

  GlrBatch *current_batch;
  GQueue *batch_queue;

  GlrTransform transform;
  gfloat translate_x;
  gfloat translate_y;

  GMutex mutex;
  GCond cond;
  gboolean finished;

  GThread *thread;
  GMutex thread_mutex;
  GCond thread_cond;
  GlrLayerDrawFunc draw_func;
  gpointer draw_func_user_data;

  GlrTexCache *tex_cache;

  GlrRect clip_area;
};

static pid_t
gettid (void)
{
  return (pid_t) syscall (SYS_gettid);
}

static void
glr_layer_free (GlrLayer *self)
{
  g_mutex_lock (&self->mutex);

  g_queue_free_full (self->batch_queue, (GDestroyNotify) glr_batch_unref);

  if (self->thread != NULL)
    {
      g_mutex_lock (&self->thread_mutex);

      self->draw_func = NULL;
      g_cond_signal (&self->thread_cond);

      g_mutex_unlock (&self->thread_mutex);

      g_thread_join (self->thread);
    }

  g_mutex_unlock (&self->mutex);

  g_mutex_clear (&self->mutex);
  g_cond_clear (&self->cond);

  g_mutex_clear (&self->thread_mutex);
  g_cond_clear (&self->thread_cond);

  glr_tex_cache_unref (self->tex_cache);

  glr_context_unref (self->context);

  g_slice_free (GlrLayer, self);
  self = NULL;

  g_debug ("GlrLayer freed\n");
}

/*
static void
reset_batch (GlrBatch *batch, gpointer user_data)
{
  glr_batch_reset (batch);
}
*/

static gpointer
draw_in_thread (gpointer user_data)
{
  GlrLayer *self = user_data;

  g_mutex_lock (&self->thread_mutex);

  while (self->draw_func != NULL)
    {
      self->draw_func (self, self->draw_func_user_data);

      /* sleep waiting for more draw_in_thread() requests */
      g_cond_wait (&self->thread_cond, &self->thread_mutex);
    }

  g_mutex_unlock (&self->thread_mutex);

  return NULL;
}

static void
copy_transform (GlrTransform *src, GlrTransform *dst)
{
  memcpy (dst, src, sizeof (GlrTransform));
}

static void
encode_and_store_transform (GlrBatch          *batch,
                            GlrTransform      *transform,
                            GlrInstanceConfig  config)
{
  goffset offset;

  /* bits 24 to 25 (2 bits) of config0 encode the number of samples
     that describe the transformation, always 2 for now */
  config[0] |= 2 << 24;

  offset = glr_batch_add_dyn_attr (batch, transform, sizeof (GlrTransform));
  config[1] = offset;
}

static void
instance_config_set_type (GlrInstanceConfig config, GlrInstanceType type)
{
  /* bits 26 to 29 (4 bits) of config0 encode the instance type */
  config[0] = (config[0] & INSTANCE_TYPE_MASK) | (type << 26);
}

static void
add_instance_with_relative_transform1 (GlrLayer          *self,
                                       GlrBatch          *batch,
                                       GlrInstanceConfig  config,
                                       GlrLayout         *layout,
                                       gfloat             sx,
                                       gfloat             sy,
                                       gfloat             rx,
                                       gfloat             ry,
                                       gfloat             pre_rotation_z,
                                       GlrInstanceType    instance_type)
{
  GlrTransform transform;
  gdouble ox, oy;
  GlrLayout lyt;

  ox = (rx - layout->left*sx) / (layout->width*sx);
  oy = (ry - layout->top*sy) / (layout->height*sy);
  copy_transform (&self->transform, &transform);
  transform.origin_x = ox;
  transform.origin_y = oy;
  transform.pre_rotation_z = pre_rotation_z;

  memcpy (&lyt, layout, sizeof (GlrLayout));
  lyt.left += self->translate_x;
  lyt.top += self->translate_y;

  instance_config_set_type (config, instance_type);
  encode_and_store_transform (batch, &transform, config);

  glr_batch_add_instance (batch, config, &lyt);
}

static gboolean
has_any_transform (const GlrTransform *transform)
{
  return UNEQUALS (transform->rotation_z, 0.0)
    || UNEQUALS (transform->pre_rotation_z, 0.0)
    || UNEQUALS (transform->scale_x, 1.0)
    || UNEQUALS (transform->scale_y, 1.0);
}

static gboolean
has_any_border (const GlrBorder *border)
{
  return UNEQUALS (border->width[0], 0.0)
    || UNEQUALS (border->width[1], 0.0)
    || UNEQUALS (border->width[2], 0.0)
    || UNEQUALS (border->width[3], 0.0)

    || UNEQUALS (border->radius[0], 0.0)
    || UNEQUALS (border->radius[1], 0.0)
    || UNEQUALS (border->radius[2], 0.0)
    || UNEQUALS (border->radius[3], 0.0);
}

static void
encode_and_store_border (GlrBatch          *batch,
                         GlrBorder         *border,
                         GlrInstanceConfig  config)
{
  goffset offset;
  gfloat buf[16] = {0};
  guint8 num_samples = 0;

  gboolean all_style_equal;
  gboolean all_width_equal;
  gboolean all_radius_equal;
  gboolean all_color_equal;

  all_style_equal = border->style[0] == border->style[1]
    && border->style[0] == border->style[2]
    && border->style[0] == border->style[3];

  all_width_equal = EQUALS (border->width[0], border->width[1])
    && EQUALS (border->width[0], border->width[2])
    && EQUALS (border->width[0], border->width[3]);

  all_radius_equal = EQUALS (border->radius[0], border->radius[1])
    && EQUALS (border->radius[0], border->radius[2])
    && EQUALS (border->radius[0], border->radius[3]);

  all_color_equal = border->color[0] == border->color[1]
    && border->color[0] == border->color[2]
    && border->color[0] == border->color[3];

  /* the simplest case: style, width, radius and color of all borders are equal */
  if (all_style_equal && all_width_equal && all_radius_equal && all_color_equal)
    {
      /* this case is encoded in 1 dyn attr sample */
      buf[0] = border->style[0];
      buf[1] = border->width[0];
      buf[2] = border->radius[0];
      buf[3] = border->color[0];

      num_samples = 1;
    }

  if (num_samples == 0)
    return;

  /* bits 20 to 23 (4 bits) of config0 encode the number of samples
     that describe the border */
  config[0] = (config[0] & BORDER_NUM_SAMPLES_MASK) | (num_samples << 20);

  offset = glr_batch_add_dyn_attr (batch,
                                   buf,
                                   sizeof (gfloat) * 4 * num_samples);

  /* config2 encodes the offset of the border description */
  config[2] = offset;
}

/* @TODO
static void
encode_and_store_background (GlrBatch          *batch,
                             GlrBackground     *bg,
                             GlrInstanceConfig  config)
{
  goffset offset;
  gfloat buf[16] = {0};
  guint8 num_samples = 0;
}
*/

/* internal API */

GQueue *
glr_layer_get_batches (GlrLayer *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  /* this is called by the canvas possibly from a different
     thread. We must block until we are finished */

  g_mutex_lock (&self->mutex);

  while (! self->finished)
    {
      /* sleep until finished() is called */
      g_cond_wait (&self->cond, &self->mutex);
    }

  g_mutex_unlock (&self->mutex);

  return self->batch_queue;
}

/* public API */

GlrLayer *
glr_layer_new (GlrContext *context)
{
  GlrLayer *self;

  self = g_slice_new0 (GlrLayer);
  self->ref_count = 1;

  self->context = glr_context_ref (context);

  self->finished = FALSE;

  self->tid = gettid ();
  g_mutex_init (&self->mutex);
  g_cond_init (&self->cond);

  self->thread = NULL;
  g_mutex_init (&self->thread_mutex);
  g_cond_init (&self->thread_cond);

  /* batches queue */
  self->current_batch = glr_batch_new ();
  self->batch_queue = g_queue_new ();
  g_queue_push_head (self->batch_queue, self->current_batch);

  /* texture cache */
  self->tex_cache = glr_context_get_texture_cache (self->context);
  glr_tex_cache_ref (self->tex_cache);

  /* init paint and transform */
  self->transform.scale_x = 1.0;
  self->transform.scale_y = 1.0;

  return self;
}

GlrLayer *
glr_layer_ref (GlrLayer *self)
{
  g_assert (self != NULL);
  g_assert (self->ref_count > 0);

  g_atomic_int_inc (&self->ref_count);

  return self;
}

void
glr_layer_unref (GlrLayer *self)
{
  g_assert (self != NULL);
  g_assert (self->ref_count > 0);

  if (g_atomic_int_dec_and_test (&self->ref_count))
    glr_layer_free (self);
}

void
glr_layer_redraw (GlrLayer *self)
{
  g_mutex_lock (&self->mutex);
  self->finished = FALSE;

  /* reset all batches */
  g_queue_free_full (self->batch_queue, (GDestroyNotify) glr_batch_unref);
  self->batch_queue = g_queue_new ();

  self->current_batch = glr_batch_new ();
  g_queue_push_head (self->batch_queue, self->current_batch);

  g_mutex_unlock (&self->mutex);
}

void
glr_layer_redraw_in_thread (GlrLayer         *self,
                            GlrLayerDrawFunc  draw_func,
                            gpointer          user_data)
{
  GError *error = NULL;

  g_assert (draw_func != NULL);

  glr_layer_redraw (self);

  g_mutex_lock (&self->thread_mutex);

  self->draw_func = draw_func;
  self->draw_func_user_data = user_data;

  g_mutex_lock (&self->mutex);

  if (self->thread != NULL)
    {
      /* a thread was already spawned. Wake it up, we have work to do */
      g_cond_signal (&self->thread_cond);
    }
  else
    {
      /* launch thread */
      self->thread = g_thread_try_new (NULL,
                                       draw_in_thread,
                                       self,
                                       &error);
      if (self->thread == NULL)
        {
          g_printerr ("Error launching draw thread: %s\n", error->message);
          g_error_free (error);
        }
    }

  g_mutex_unlock (&self->mutex);
  g_mutex_unlock (&self->thread_mutex);
}

void
glr_layer_finish (GlrLayer *self)
{
  CHECK_LAYER_NOT_FINSHED (self);

  g_mutex_lock (&self->mutex);

  self->finished = TRUE;

  /* wake up any canvas waiting for me to finish */
  g_cond_broadcast (&self->cond);

  g_mutex_unlock (&self->mutex);
}

void
glr_layer_clear (GlrLayer *self)
{
  /* reset all batches */
  g_mutex_lock (&self->mutex);

  g_queue_free_full (self->batch_queue, (GDestroyNotify) glr_batch_unref);
  self->batch_queue = g_queue_new ();

  self->current_batch = glr_batch_new ();
  g_queue_push_head (self->batch_queue, self->current_batch);

  g_mutex_unlock (&self->mutex);
}

void
glr_layer_set_transform_origin (GlrLayer *self,
                                gfloat    origin_x,
                                gfloat    origin_y)
{
  self->transform.origin_x = origin_x;
  self->transform.origin_y = origin_y;
}

void
glr_layer_scale (GlrLayer *self, gfloat scale_x, gfloat scale_y)
{
  self->transform.scale_x = scale_x;
  self->transform.scale_y = scale_y;
}

void
glr_layer_rotate (GlrLayer *self, gfloat angle)
{
  self->transform.rotation_z = angle * ((((gfloat) M_PI * 2.0) / 360.0));
}

void
glr_layer_translate (GlrLayer *self, gfloat x, gfloat y)
{
  self->translate_x = x;
  self->translate_y = y;
}

void
glr_layer_clip (GlrLayer *self,
                gfloat    top,
                gfloat    left,
                gfloat    width,
                gfloat    height)
{
  self->clip_area.left = left;
  self->clip_area.top = top;
  self->clip_area.width = width;
  self->clip_area.height = height;
}

void
glr_layer_draw_char (GlrLayer *self,
                     guint32   unicode_char,
                     gfloat    left,
                     gfloat    top,
                     GlrFont  *font,
                     GlrColor  color)
{
  GlrBatch *batch;
  GlrLayout layout;
  GlrInstanceConfig config = {0};
  const GlrTexSurface *surface;
  gfloat tex_area[4] = {0};

  CHECK_LAYER_NOT_FINSHED (self);

  batch = self->current_batch;
  if (glr_batch_is_full (batch))
    {
      /* @TODO: need flusing */
      return;
    }

  /* @FIXME: provide a default font in case none is specified */
  surface = glr_tex_cache_lookup_font_glyph (self->tex_cache,
                                             font->face,
                                             font->face_index,
                                             font->size,
                                             unicode_char);
  if (surface == NULL)
    {
      /* @FIXME: implement batch breaking */
      return;
    }

  instance_config_set_type (config, GLR_INSTANCE_CHAR_GLYPH);

  if (has_any_transform (&self->transform))
    encode_and_store_transform (batch, &self->transform, config);

  layout.left = left + self->translate_x;
  layout.top = top + self->translate_y;
  layout.width = surface->pixel_width;
  layout.height = surface->pixel_height;

  tex_area[0] = surface->left;
  tex_area[1] = surface->top;
  tex_area[2] = surface->width;
  tex_area[3] = surface->height;

  /* @FIXME: move this to a more general way of describing background */
  goffset offset;
  offset = glr_batch_add_dyn_attr (batch, tex_area, sizeof (gfloat) * 4);
  config[2] = offset;
  config[3] = color;

  /* bits 12 to 15 (4 bits) of config0 encode the texture unit to use,
     either for glyphs, background-image or border-image */
  config[0] |= surface->tex_id << 12;

  glr_batch_add_instance (batch, config, &layout);
}

void
glr_layer_draw_rect (GlrLayer *self,
                     gfloat    left,
                     gfloat    top,
                     gfloat    width,
                     gfloat    height,
                     GlrStyle *style)
{
  GlrBatch *batch;
  GlrLayout layout;
  GlrBorder *br = &(style->border);
  GlrBackground *bg = &(style->background);
  gdouble rx, ry, sx, sy;
  GlrInstanceConfig config = {0};
  gboolean has_transform = FALSE;
  gboolean has_border = FALSE;

  gfloat aa = 1.2;

  CHECK_LAYER_NOT_FINSHED (self);

  batch = self->current_batch;
  if (glr_batch_is_full (batch))
    {
      /* @TODO: implement flushing */
      return;
    }

  has_transform = has_any_transform (&self->transform);
  has_border = has_any_border (br);

  /* encode and submit border, which is common to all sub-instances */
  if (has_border)
    encode_and_store_border (batch, br, config);

  /* background */
  if (bg->type != GLR_BACKGROUND_NONE)
    {
      instance_config_set_type (config, GLR_INSTANCE_RECT_BG);

      /* the simplest type of background is flat color. In that case
         config3 is the background color */
      config[3] = style->background.color;

      if (has_transform)
        encode_and_store_transform (batch, &self->transform, config);

      layout.left = left + self->translate_x - aa/2.0;
      layout.top = top  + self->translate_y - aa/2.0;
      layout.width = width + aa;
      layout.height = height + aa;

      glr_batch_add_instance (batch, config, &layout);
    }

  /* pre-calculations for the relative transform origin */
  sx = self->transform.scale_x;
  sy = self->transform.scale_y;
  rx = left*sx + (width*sx) * self->transform.origin_x;
  ry = top*sy + (height*sy) * self->transform.origin_y;

  /* left border */
  if (UNEQUALS (br->width[0], 0.0))
    {
      // g_print ("left border\n");
      config[3] = br->color[0];

      layout.left = left - aa/2.0;
      layout.width = br->width[0] + aa;
      layout.top = top - aa/2.0 + br->radius[0];

      if (UNEQUALS (br->radius[0], 0.0))
        {
          layout.height = height + aa - br->radius[2]
            - MAX (br->radius[0], br->width[0]);
        }
      else
        {
          layout.height = height - br->radius[2]
            - MAX (br->radius[0], br->width[0]);
        }

      add_instance_with_relative_transform1 (self,
                                             batch,
                                             config,
                                             &layout,
                                             sx, sy, rx, ry,
                                             0.0,
                                             GLR_INSTANCE_BORDER_LEFT);
    }

  /* top border */
  if (UNEQUALS (br->width[1], 0.0))
    {
      // g_print ("top border\n");
      config[3] = br->color[1];

      if (UNEQUALS (br->radius[0], 0.0))
        {
          layout.left = left - aa/2.0 + MAX (br->radius[0], br->width[0]);
          layout.width = width + aa
            - MAX (br->radius[0], br->width[0])
            - br->radius[1];
        }
      else
        {
          layout.left = left + aa/2.0 + MAX (br->radius[0], br->width[0]);
          layout.width = width
            - MAX (br->radius[0], br->width[0])
            - br->radius[1];
        }

      layout.top = top - aa/2.0;
      layout.height = br->width[1] + aa;

      add_instance_with_relative_transform1 (self,
                                             batch,
                                             config,
                                             &layout,
                                             sx, sy, rx, ry,
                                             0.0,
                                             GLR_INSTANCE_BORDER_TOP);
    }

  /* right border */
  if (UNEQUALS (br->width[2], 0.0))
    {
      // g_print ("top border\n");
      config[3] = br->color[2];

      layout.left = left + width - br->width[2] - aa/2.0;
      layout.width = br->width[2] + aa;

      if (UNEQUALS (br->radius[1], 0.0))
        {
          layout.top = top - aa/2.0 + MAX (br->radius[1], br->width[1]);
          layout.height = height + aa
            - MAX (br->radius[1], br->width[1])
            - br->radius[3];
        }
      else
        {
          layout.top = top + aa/2.0 + MAX (br->radius[1], br->width[1]);
          layout.height = height
            - MAX (br->radius[1], br->width[1])
            - br->radius[3];
        }

      add_instance_with_relative_transform1 (self,
                                             batch,
                                             config,
                                             &layout,
                                             sx, sy, rx, ry,
                                             0.0,
                                             GLR_INSTANCE_BORDER_RIGHT);
    }

  /* bottom border */
  if (UNEQUALS (br->width[2], 0.0))
    {
      // g_print ("top border\n");
      config[3] = br->color[3];

      if (UNEQUALS (br->radius[2], 0.0))
        {
          layout.width = width + aa
            - MAX (br->radius[2], br->width[0])
            - br->radius[3];
        }
      else
        {
          layout.width = width
            - MAX (br->radius[2], br->width[0])
            - br->radius[3];
        }

      layout.left = left - aa/2.0 + br->radius[2];
      layout.top = top + height - br->width[3] - aa/2.0;
      layout.height = br->width[3] + aa;

      add_instance_with_relative_transform1 (self,
                                             batch,
                                             config,
                                             &layout,
                                             sx, sy, rx, ry,
                                             0.0,
                                             GLR_INSTANCE_BORDER_BOTTOM);
    }

  /* top left border */
  if (UNEQUALS (br->radius[0], 0.0)
      && (UNEQUALS (br->width[0], 0.0)
          || UNEQUALS (br->width[1], 0.0)))
    {
      config[3] = br->color[0];
      layout.left = left + br->radius[0] - aa/2.0;
      layout.top = top + br->radius[0] - aa/2.0;
      layout.width = br->radius[0];
      layout.height = br->radius[0];
      add_instance_with_relative_transform1 (self,
                                             batch,
                                             config,
                                             &layout,
                                             sx, sy, rx, ry,
                                             M_PI / 2.0 * 2.0,
                                             GLR_INSTANCE_BORDER_TOP_LEFT);
    }

  /* top right border */
  if (UNEQUALS (br->radius[1], 0.0)
      && (UNEQUALS (br->width[1], 0.0)
          || UNEQUALS (br->width[2], 0.0)))
    {
      config[3] = br->color[0];
      layout.left = left + width - br->radius[1] + aa/2.0;
      layout.top = top + br->radius[1] - aa/2.0;
      layout.width = br->radius[1];
      layout.height = br->radius[1];
      add_instance_with_relative_transform1 (self,
                                             batch,
                                             config,
                                             &layout,
                                             sx, sy, rx, ry,
                                             M_PI / 2.0 * 1.0,
                                             GLR_INSTANCE_BORDER_TOP_RIGHT);
    }

  /* bottom left border */
  if (UNEQUALS (br->radius[2], 0.0)
      && UNEQUALS (br->width[0], 0.0)
      && UNEQUALS (br->width[3], 0.0))
    {
      layout.left = left + br->radius[2] - aa/2.0;
      layout.top = top + height - br->radius[2] + aa/2.0;
      layout.width = br->radius[2];
      layout.height = br->radius[2];
      add_instance_with_relative_transform1 (self,
                                             batch,
                                             config,
                                             &layout,
                                             sx, sy, rx, ry,
                                             M_PI / 2.0 * 3.0,
                                             GLR_INSTANCE_BORDER_BOTTOM_LEFT);
    }

  /* bottom right border */
  if (UNEQUALS (br->radius[3], 0.0)
      && UNEQUALS (br->width[2], 0.0)
      && UNEQUALS (br->width[3], 0.0))
    {
      layout.left = left + width - br->radius[3] + aa/2.0;
      layout.top = top + height - br->radius[3] + aa/2.0;
      layout.width = br->radius[3];
      layout.height = br->radius[3];
      add_instance_with_relative_transform1 (self,
                                             batch,
                                             config,
                                             &layout,
                                             sx, sy, rx, ry,
                                             M_PI / 2.0 * 4.0,
                                             GLR_INSTANCE_BORDER_BOTTOM_LEFT);
    }
}
