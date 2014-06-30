#include "glr-layer.h"

#include "glr-target.h"
#include "glr-tex-cache.h"
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

  GHashTable *commands;
  GQueue *command_queue;

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

  g_queue_free (self->command_queue);
  g_hash_table_unref (self->commands);

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

static GlrCommand *
get_draw_rect_command (GlrLayer *self, GlrPaintStyle style)
{
  gchar *cmd_id;
  GlrCommand *command;

  if (style == GLR_PAINT_STYLE_FILL)
    cmd_id = g_strdup_printf ("rect:fill");
  else
    cmd_id = g_strdup_printf ("rect:stroke");

  command = g_hash_table_lookup (self->commands, cmd_id);
  if (command == NULL)
    {
      const GlrPrimitive *primitive;

      if (style == GLR_PAINT_STYLE_FILL)
        primitive = glr_context_get_primitive (self->context,
                                               GLR_PRIMITIVE_RECT_FILL);
      else
        primitive = glr_context_get_primitive (self->context,
                                               GLR_PRIMITIVE_RECT_STROKE);

      command = glr_command_new (primitive);
      g_hash_table_insert (self->commands, g_strdup (cmd_id), command);
    }

  g_free (cmd_id);

  if (g_queue_find (self->command_queue, command) == NULL)
    g_queue_push_tail (self->command_queue, command);

  return command;
}

static GlrCommand *
get_draw_round_corner_command (GlrLayer *self, GlrPaintStyle style)
{
  gchar *cmd_id;
  GlrCommand *command;

  if (style == GLR_PAINT_STYLE_FILL)
    cmd_id = g_strdup_printf ("round-corner:fill");
  else
    cmd_id = g_strdup_printf ("round-corner:stroke");

  command = g_hash_table_lookup (self->commands, cmd_id);
  if (command == NULL)
    {
      const GlrPrimitive *primitive;

      if (style == GLR_PAINT_STYLE_FILL)
        primitive = glr_context_get_primitive (self->context,
                                               GLR_PRIMITIVE_ROUND_CORNER_FILL);
      else
        g_assert_not_reached (); /* not implemeneted */

      command = glr_command_new (primitive);
      g_hash_table_insert (self->commands, g_strdup (cmd_id), command);
    }

  g_free (cmd_id);

  if (g_queue_find (self->command_queue, command) == NULL)
    g_queue_push_tail (self->command_queue, command);

  return command;
}

static void
reset_command (GlrCommand *command, gpointer user_data)
{
  glr_command_reset (command);
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

/* internal API */

GQueue *
glr_layer_get_commands (GlrLayer *self)
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

  return self->command_queue;
}

static void
copy_transform (GlrTransform *src, GlrTransform *dst)
{
  memcpy (dst, src, sizeof (GlrTransform));
}

static void
add_round_corner_with_transform (GlrCommand   *round_corner_cmd,
                                 GlrLayout    *layout,
                                 guint32       color,
                                 GlrTransform *transform,
                                 gdouble       rx,
                                 gdouble       ry,
                                 gdouble       corner_rotation)
{
  GlrTransform final_transform;
  gdouble sx, sy, ox, oy;

  sx = transform->scale_x;
  sy = transform->scale_y;

  copy_transform (transform, &final_transform);
  final_transform.rotation_z = corner_rotation;
  final_transform.origin_x = 0.0;
  final_transform.origin_y = 0.0;

  ox = (rx - layout->left*sx) / (layout->width*sx);
  oy = (ry - layout->top*sy) / (layout->height*sy);
  final_transform.parent_rotation_z = transform->rotation_z;
  final_transform.parent_origin_x = ox;
  final_transform.parent_origin_y = oy;

  glr_command_add_instance (round_corner_cmd,
                            layout,
                            color,
                            &final_transform,
                            NULL);
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

  /* commands table and queue */
  self->commands = g_hash_table_new_full (g_str_hash,
                                          g_str_equal,
                                          g_free,
                                          (GDestroyNotify) glr_command_free);
  self->command_queue = g_queue_new ();

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

  /* reset all commands */
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
  /* reset all commands */
  g_queue_foreach (self->command_queue, (GFunc) reset_command, NULL);
}

void
glr_layer_draw_rect (GlrLayer *self,
                     gfloat    left,
                     gfloat    top,
                     gfloat    width,
                     gfloat    height,
                     GlrPaint *paint)
{
  GlrCommand *command;
  GlrLayout layout;

  CHECK_LAYER_NOT_FINSHED (self);

  command = get_draw_rect_command (self, paint->style);
  if (glr_command_is_full (command))
    {
      /* Fail! */
      return;
    }

  layout.left = left + self->translate_x;
  layout.top = top + self->translate_y;
  layout.width = width;
  layout.height = height;

  glr_command_add_instance (command,
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

  self->transform.rotation_z = angle * ((M_PI * 2.0) / 360.0);
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
  GlrCommand *command;
  GlrLayout layout;
  const GlrTexSurface *surface;

  CHECK_LAYER_NOT_FINSHED (self);

  command = get_draw_rect_command (self, GLR_PAINT_STYLE_FILL);
  if (glr_command_is_full (command))
    return;

  /* @FIXME: provide a default font in case none is specified */
  surface = glr_tex_cache_lookup_font_glyph (self->tex_cache,
                                             font->face,
                                             font->face_index,
                                             font->size,
                                             unicode_char);

  layout.left = left + self->translate_x;
  layout.top = top + self->translate_y;
  layout.width = surface->width / 3;
  layout.height = surface->height;

  glr_command_add_instance (command,
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
  GlrCommand *round_corner_cmd;
  GlrCommand *rect_cmd;
  GlrLayout layout;
  GlrTransform transform;

  CHECK_LAYER_NOT_FINSHED (self);

  /* @FIXME: only filled rrect current implemented */
  if (paint->style == GLR_PAINT_STYLE_STROKE)
    {
      g_warning ("Storked rrect not implemented. Ignoring");
      return;
    }

  round_corner_cmd = get_draw_round_corner_command (self, paint->style);
  rect_cmd = get_draw_rect_command (self, paint->style);
  if (glr_command_is_full (round_corner_cmd) ||
      glr_command_is_full (rect_cmd))
    {
      /* @TODO: implement flushing */
      return;
    }

  gdouble rx, ry, ox, oy, sx, sy;
  sx = self->transform.scale_x;
  sy = self->transform.scale_y;
  rx = left*sx + (width*sx) * self->transform.origin_x;
  ry = top*sy + (height*sy) * self->transform.origin_y;

  /* top left corner */
  layout.left = left + self->translate_x + border_radius;
  layout.top = top + self->translate_y + border_radius;
  layout.width = border_radius;
  layout.height = border_radius;

  add_round_corner_with_transform (round_corner_cmd,
                                   &layout,
                                   paint->color,
                                   &self->transform,
                                   rx, ry,
                                   M_PI / 2.0 * 2.0);

  /* top right corner */
  layout.left = left + self->translate_x + width - border_radius;
  layout.top = top + self->translate_y + border_radius;

  add_round_corner_with_transform (round_corner_cmd,
                                   &layout,
                                   paint->color,
                                   &self->transform,
                                   rx, ry,
                                   M_PI / 2.0 * 3.0);

  /* bottom left corner */
  layout.left = left + self->translate_x + border_radius;
  layout.top = top + self->translate_y + height - border_radius;

  add_round_corner_with_transform (round_corner_cmd,
                                   &layout,
                                   paint->color,
                                   &self->transform,
                                   rx, ry,
                                   M_PI / 2.0 * 1.0);

  /* bottom right corner */
  layout.left = left + self->translate_x + width - border_radius;
  layout.top = top + self->translate_y + height - border_radius;

  add_round_corner_with_transform (round_corner_cmd,
                                   &layout,
                                   paint->color,
                                   &self->transform,
                                   rx, ry,
                                   M_PI / 2.0 * 4.0);

  /* horiz rect */
  layout.left = left + self->translate_x;
  layout.top = top + self->translate_y + border_radius;
  layout.width = width;
  layout.height = height - border_radius * 2;

  ox = (rx - layout.left*sx) / (layout.width*sx);
  oy = (ry - layout.top*sy) / (layout.height*sy);
  copy_transform (&self->transform, &transform);
  transform.origin_x = ox;
  transform.origin_y = oy;

  glr_command_add_instance (rect_cmd,
                            &layout,
                            paint->color,
                            &transform,
                            NULL);

  /* top rect */
  layout.left = left + self->translate_x + border_radius;
  layout.top = top + self->translate_y;
  layout.width = width - border_radius * 2;
  layout.height = border_radius;

  ox = (rx - layout.left*sx) / (layout.width*sx);
  oy = (ry - layout.top*sy) / (layout.height*sy);
  copy_transform (&self->transform, &transform);
  transform.origin_x = ox;
  transform.origin_y = oy;

  glr_command_add_instance (rect_cmd,
                            &layout,
                            paint->color,
                            &transform,
                            NULL);

  /* top rect */
  layout.left = left + self->translate_x + border_radius;
  layout.top = top + self->translate_y + height - border_radius;
  layout.width = width - border_radius * 2;
  layout.height = border_radius;

  ox = (rx - layout.left*sx) / (layout.width*sx);
  oy = (ry - layout.top*sy) / (layout.height*sy);
  copy_transform (&self->transform, &transform);
  transform.origin_x = ox;
  transform.origin_y = oy;

  glr_command_add_instance (rect_cmd,
                            &layout,
                            paint->color,
                            &transform,
                            NULL);
}
