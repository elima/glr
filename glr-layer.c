#include "glr-layer.h"

#include "glr-batch.h"
#include "glr-tex-cache.h"
#include "glr-priv.h"
#include <math.h>
#include <sys/syscall.h>
#include <string.h>
#include <unistd.h>

#define CHECK_LAYER_NOT_FINSHED(self) \
  if (self->finished) return g_warning ("Layer has been finished. Need to call redraw")

struct _GlrLayer
{
  gint ref_count;

  pid_t tid;
  GlrLayerStatus status;

  GlrContext *context;

  GHashTable *batches;
  GQueue *batch_queue;

  GlrTransform transform;
  GlrPaint paint;
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

  g_queue_free (self->batch_queue);
  g_hash_table_unref (self->batches);

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

static GlrBatch *
get_rect_batch (GlrLayer *self, GlrPaintStyle style)
{
  gchar *cmd_id;
  GlrBatch *batch;

  if (style == GLR_PAINT_STYLE_FILL)
    cmd_id = g_strdup_printf ("rect:fill");
  else
    cmd_id = g_strdup_printf ("rect:stroke");

  batch = g_hash_table_lookup (self->batches, cmd_id);
  if (batch == NULL)
    {
      const GlrPrimitive *primitive;

      if (style == GLR_PAINT_STYLE_FILL)
        primitive = glr_context_get_primitive (self->context,
                                               GLR_PRIMITIVE_RECT_FILL);
      else
        primitive = glr_context_get_primitive (self->context,
                                               GLR_PRIMITIVE_RECT_STROKE);

      batch = glr_batch_new (primitive);
      g_hash_table_insert (self->batches, g_strdup (cmd_id), batch);
    }

  g_free (cmd_id);

  if (g_queue_find (self->batch_queue, batch) == NULL)
    g_queue_push_tail (self->batch_queue, glr_batch_ref (batch));

  return batch;
}

static GlrBatch *
get_round_corner_batch (GlrLayer      *self,
                        GlrPaintStyle  style,
                        gdouble        radius,
                        gdouble        border_width1,
                        gdouble        border_width2)
{
  gchar *cmd_id;
  GlrBatch *batch = NULL;
  gdouble dyn_value1, dyn_value2;

  if (style == GLR_PAINT_STYLE_FILL)
    {
      cmd_id = g_strdup_printf ("round-corner:fill");
    }
  else
    {
      dyn_value1 = border_width1 / radius;
      dyn_value2 = border_width2 / radius;

      cmd_id = g_strdup_printf ("round-corner:stroke:%08f:%08f", dyn_value1, dyn_value2);
    }

  batch = g_hash_table_lookup (self->batches, cmd_id);
  if (batch == NULL)
    {
      const GlrPrimitive *primitive;

      if (style == GLR_PAINT_STYLE_FILL)
        {
          primitive = glr_context_get_primitive (self->context,
                                                 GLR_PRIMITIVE_ROUND_CORNER_FILL);
        }
      else
        {
          primitive =
            glr_context_get_dynamic_primitive (self->context,
                                               GLR_PRIMITIVE_ROUND_CORNER_STROKE,
                                               dyn_value1,
                                               dyn_value2);
        }

      batch = glr_batch_new (primitive);
      g_hash_table_insert (self->batches, g_strdup (cmd_id), batch);
    }

  g_free (cmd_id);

  if (g_queue_find (self->batch_queue, batch) == NULL)
    g_queue_push_tail (self->batch_queue, glr_batch_ref (batch));

  return batch;
}

static void
reset_batch (GlrBatch *batch, gpointer user_data)
{
  glr_batch_reset (batch);
}

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
add_instance_with_relative_transform (GlrLayer  *self,
                                      GlrBatch  *batch,
                                      GlrLayout *layout,
                                      gfloat     sx,
                                      gfloat     sy,
                                      gfloat     rx,
                                      gfloat     ry,
                                      gfloat     pre_rotation_z,
                                      GlrPaint  *paint)
{
  GlrTransform transform;
  gdouble ox, oy;

  ox = (rx - layout->left*sx) / (layout->width*sx);
  oy = (ry - layout->top*sy) / (layout->height*sy);
  copy_transform (&self->transform, &transform);
  transform.origin_x = ox;
  transform.origin_y = oy;
  transform.pre_rotation_z = pre_rotation_z;

  layout->left += self->translate_x;
  layout->top += self->translate_y;

  glr_batch_add_instance (batch,
                          layout,
                          paint->color,
                          &transform,
                          NULL);
}

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

  /* batches table and queue */
  self->batches = g_hash_table_new_full (g_str_hash,
                                          g_str_equal,
                                          g_free,
                                          (GDestroyNotify) glr_batch_unref);
  self->batch_queue = g_queue_new ();

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
  glr_layer_clear (self);

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
  g_queue_foreach (self->batch_queue, (GFunc) reset_batch, NULL);
  g_queue_free_full (self->batch_queue, (GDestroyNotify) glr_batch_unref);
  self->batch_queue = g_queue_new ();
}

void
glr_layer_draw_rect (GlrLayer *self,
                     gfloat    left,
                     gfloat    top,
                     gfloat    width,
                     gfloat    height,
                     GlrPaint *paint)
{
  GlrBatch *batch;
  GlrLayout layout;

  CHECK_LAYER_NOT_FINSHED (self);

  batch = get_rect_batch (self, paint->style);
  if (glr_batch_is_full (batch))
    {
      /* Fail! */
      return;
    }

  layout.left = left + self->translate_x;
  layout.top = top + self->translate_y;
  layout.width = width;
  layout.height = height;

  glr_batch_add_instance (batch,
                          &layout,
                          paint->color,
                          &self->transform,
                          NULL);
}

void
glr_layer_set_transform_origin (GlrLayer *self,
                                gfloat    origin_x,
                                gfloat    origin_y)
{
  CHECK_LAYER_NOT_FINSHED (self);

  self->transform.origin_x = origin_x;
  self->transform.origin_y = origin_y;
}

void
glr_layer_scale (GlrLayer *self, gfloat scale_x, gfloat scale_y)
{
  CHECK_LAYER_NOT_FINSHED (self);

  self->transform.scale_x = scale_x;
  self->transform.scale_y = scale_y;
}

void
glr_layer_rotate (GlrLayer *self, gfloat angle)
{
  CHECK_LAYER_NOT_FINSHED (self);

  self->transform.rotation_z = angle * ((((gfloat) M_PI * 2.0) / 360.0));
}

void
glr_layer_translate (GlrLayer *self, gfloat x, gfloat y)
{
  CHECK_LAYER_NOT_FINSHED (self);

  self->translate_x = x;
  self->translate_y = y;
}

GlrPaint *
glr_layer_get_default_paint (GlrLayer *self)
{
  return &self->paint;
}

void
glr_layer_draw_char (GlrLayer *self,
                     guint32   unicode_char,
                     gfloat    left,
                     gfloat    top,
                     GlrFont  *font,
                     GlrPaint *paint)
{
  GlrBatch *batch;
  GlrLayout layout;
  const GlrTexSurface *surface;

  CHECK_LAYER_NOT_FINSHED (self);


  batch = get_rect_batch (self, GLR_PAINT_STYLE_FILL);
  if (glr_batch_is_full (batch))
    return;

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

  layout.left = left + self->translate_x;
  layout.top = top + self->translate_y;
  layout.width = surface->width / 3;
  layout.height = surface->height;

  glr_batch_add_instance (batch,
                          &layout,
                          paint->color,
                          &self->transform,
                          surface);
}

void
glr_layer_draw_rounded_rect (GlrLayer *self,
                             gfloat    left,
                             gfloat    top,
                             gfloat    width,
                             gfloat    height,
                             gfloat    border_radius,
                             GlrPaint *paint)
{
  GlrBatch *round_corner_batch;
  GlrBatch *rect_batch;
  GlrLayout layout;

  CHECK_LAYER_NOT_FINSHED (self);

  round_corner_batch = get_round_corner_batch (self,
                                               paint->style,
                                               border_radius,
                                               paint->border_width,
                                               paint->border_width);
  rect_batch = get_rect_batch (self, GLR_PAINT_STYLE_FILL);
  if (glr_batch_is_full (round_corner_batch) ||
      glr_batch_is_full (rect_batch))
    {
      /* @TODO: implement flushing */
      return;
    }

  gdouble rx, ry, sx, sy;
  sx = self->transform.scale_x;
  sy = self->transform.scale_y;
  rx = left*sx + (width*sx) * self->transform.origin_x;
  ry = top*sy + (height*sy) * self->transform.origin_y;

  /* top left corner */
  layout.left = left + border_radius;
  layout.top = top + border_radius;
  layout.width = border_radius;
  layout.height = border_radius;

  add_instance_with_relative_transform (self,
                                        round_corner_batch,
                                        &layout,
                                        sx, sy, rx, ry,
                                        M_PI / 2.0 * 2.0,
                                        paint);

  /* top right corner */
  layout.left = left + width - border_radius;
  layout.top = top + border_radius;

  add_instance_with_relative_transform (self,
                                        round_corner_batch,
                                        &layout,
                                        sx, sy, rx, ry,
                                        M_PI / 2.0 * 1.0,
                                        paint);

  /* bottom left corner */
  layout.left = left + border_radius;
  layout.top = top + height - border_radius;

  add_instance_with_relative_transform (self,
                                        round_corner_batch,
                                        &layout,
                                        sx, sy, rx, ry,
                                        M_PI / 2.0 * 3.0,
                                        paint);

  /* bottom right corner */
  layout.left = left + width - border_radius;
  layout.top = top + height - border_radius;

  add_instance_with_relative_transform (self,
                                        round_corner_batch,
                                        &layout,
                                        sx, sy, rx, ry,
                                        M_PI / 2.0 * 4.0,
                                        paint);

  if (paint->style == GLR_PAINT_STYLE_STROKE)
    {
      /* left border */
      layout.left = left;
      layout.top = top + border_radius;
      layout.width = paint->border_width;
      layout.height = height - border_radius * 2;

      add_instance_with_relative_transform (self,
                                            rect_batch,
                                            &layout,
                                            sx, sy, rx, ry,
                                            0.0,
                                            paint);

      /* right border */
      layout.left = left + width - paint->border_width;
      layout.top = top + border_radius;
      layout.width = paint->border_width;
      layout.height = height - border_radius * 2;

      add_instance_with_relative_transform (self,
                                            rect_batch,
                                            &layout,
                                            sx, sy, rx, ry,
                                            0.0,
                                            paint);

      /* top border */
      layout.left = left + border_radius;
      layout.top = top;
      layout.width = width - border_radius * 2;
      layout.height = paint->border_width;

      add_instance_with_relative_transform (self,
                                            rect_batch,
                                            &layout,
                                            sx, sy, rx, ry,
                                            0.0,
                                            paint);

      /* bottom border */
      layout.left = left + border_radius;
      layout.top = top + height - paint->border_width;
      layout.width = width - border_radius * 2;
      layout.height = paint->border_width;

      add_instance_with_relative_transform (self,
                                            rect_batch,
                                            &layout,
                                            sx, sy, rx, ry,
                                            0.0,
                                            paint);
    }
  else
    {
      /* horiz rect */
      layout.left = left;
      layout.top = top + border_radius;
      layout.width = width;
      layout.height = height - border_radius * 2;

      add_instance_with_relative_transform (self,
                                            rect_batch,
                                            &layout,
                                            sx, sy, rx, ry,
                                            0.0,
                                            paint);

      /* top rect */
      layout.left = left + border_radius;
      layout.top = top;
      layout.width = width - border_radius * 2;
      layout.height = border_radius;

      add_instance_with_relative_transform (self,
                                            rect_batch,
                                            &layout,
                                            sx, sy, rx, ry,
                                            0.0,
                                            paint);

      /* bottom rect */
      layout.left = left + border_radius;
      layout.top = top + height - border_radius;
      layout.width = width - border_radius * 2;
      layout.height = border_radius;

      add_instance_with_relative_transform (self,
                                            rect_batch,
                                            &layout,
                                            sx, sy, rx, ry,
                                            0.0,
                                            paint);
    }
}
