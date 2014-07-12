#include "glr-canvas.h"

#include <glib.h>
#include "glr-batch.h"
#include "glr-layer.h"
#include "glr-paint.h"
#include "glr-symbols.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#define TRANSFORM_TEX_WIDTH  1024
#define TRANSFORM_TEX_HEIGHT  512

#define PRIMITIVE_ATTR 0
#define LAYOUT_ATTR    1
#define COLOR_ATTR     2
#define CONFIG_ATTR    3
#define TEX_AREA_ATTR  4

struct _GlrCanvas
{
  gint ref_count;

  pid_t canvas_tid;

  GlrTarget *target;

  GLuint shader_program;

  GLuint width_loc;
  GLuint height_loc;

  guint64 frame_count;
  gboolean frame_started;
  gboolean frame_initialized;

  GLuint transform_buffer_tex;
  GLuint tex_area_buffer_tex;

  guint32 clear_color;
  gboolean pending_clear;

  GMutex layers_mutex;
  GQueue *attached_layers;
};

typedef struct
{
  guint index;
  GlrLayer *layer;
} LayerAttachment;

static pid_t
gettid (void)
{
  return (pid_t) syscall (SYS_gettid);
}

static void
glr_canvas_free (GlrCanvas *self)
{
  g_queue_free_full (self->attached_layers, (GDestroyNotify) glr_layer_unref);
  g_mutex_clear (&self->layers_mutex);

  glDeleteProgram (self->shader_program);

  glDeleteTextures (1, &self->transform_buffer_tex);
  glDeleteTextures (1, &self->tex_area_buffer_tex);

  g_slice_free (GlrCanvas, self);
  self = NULL;

  g_print ("GlrCanvas freed\n");
}

static gboolean
print_shader_log (GLuint shader)
{
  GLint  length;
  gchar buffer[1024];
  GLint success;

  glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &length);
  if (length == 0)
    return TRUE;

  glGetShaderInfoLog (shader, 1024, NULL, buffer);
  if (strlen (buffer) > 0)
    g_printerr ("Shader compilation log: %s", buffer);

  glGetShaderiv (shader, GL_COMPILE_STATUS, &success);

  return success == GL_TRUE;
}

static GLuint
load_shader (const char *shader_source, GLenum type)
{
  GLuint shader = glCreateShader (type);

  glShaderSource (shader, 1, &shader_source, NULL);
  glCompileShader (shader);

  print_shader_log (shader);

  return shader;
}

static void
clear_background (GlrCanvas *self)
{
  static const guint32 MASK_8_BIT = 0x000000FF;

  glClearColor ( (self->clear_color >> 24              )  / 255.0,
                ((self->clear_color >> 16) & MASK_8_BIT) / 255.0,
                ((self->clear_color >>  8) & MASK_8_BIT) / 255.0001,
                ( self->clear_color        & MASK_8_BIT) / 255.0);
  glClear (GL_COLOR_BUFFER_BIT);
}

static void
free_layer_attachment (LayerAttachment *layer_attachment)
{
  glr_layer_unref (layer_attachment->layer);
  g_slice_free (LayerAttachment, layer_attachment);
}

static gint
sort_layers_func (LayerAttachment *layer1,
                  LayerAttachment *layer2,
                  gpointer         user_data)
{
  if (layer1->index < layer2->index)
    return -1;
  else if (layer1->index == layer2->index)
    return 0;
  else
    return 1;
}

static void
initialize_frame_if_needed (GlrCanvas *self)
{
  guint32 width, height;

  if (self->frame_initialized)
    return;

  self->frame_initialized = TRUE;

  glBindFramebuffer (GL_FRAMEBUFFER,
                     glr_target_get_framebuffer (self->target));
  glUseProgram (self->shader_program);

  if (self->pending_clear)
    {
      clear_background (self);
      self->pending_clear = FALSE;
    }

  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glr_target_get_size (self->target, &width, &height);

  glPushAttrib (GL_VIEWPORT_BIT);
  glViewport (0, 0, width, height);

  glUniform1ui (self->width_loc, width);
  glUniform1ui (self->height_loc, height);

  /* @FIXME: get the glyph texture id from texture cache, instead of hardcoding it here */
  gint i;
  for (i = 0; i < 8; i++)
    {
      GLuint loc;
      gchar *st;

      st = g_strdup_printf ("glyph_cache[%d]", i);
      loc = glGetUniformLocation (self->shader_program, st);
      glUniform1i (loc, i);
    }
}

static void
draw_batch (GlrBatch *batch, gpointer user_data)
{
  GlrCanvas *self = user_data;

  glr_batch_draw (batch, self->shader_program);
}

static void
flush (GlrCanvas *self)
{
  LayerAttachment *layer_attachment;

  initialize_frame_if_needed (self);

  /* bind the transform buffer texture */
  glActiveTexture (GL_TEXTURE8);
  glBindTexture (GL_TEXTURE_2D, self->transform_buffer_tex);

  g_mutex_lock (&self->layers_mutex);

  while ((layer_attachment = g_queue_pop_head (self->attached_layers)) != NULL)
    {
      GQueue *batch_queue;

      /* The call to glr_layer_get_batches() will block until finish()
         is called on the layer, if it has not finished yet. */
      batch_queue = glr_layer_get_batches (layer_attachment->layer);

      /* execute each batched drawing in the layer, in order */
      g_queue_foreach (batch_queue, (GFunc) draw_batch, self);

      free_layer_attachment (layer_attachment);
    }

  g_queue_free_full (self->attached_layers,
                     (GDestroyNotify) free_layer_attachment);
  self->attached_layers = g_queue_new ();

  g_mutex_unlock (&self->layers_mutex);

  /* unbind the transform buffer texture */
  glBindTexture (GL_TEXTURE_2D, 0);
}

/* public */

GlrCanvas *
glr_canvas_new (GlrTarget *target)
{
  GlrCanvas *self;
  GError *error = NULL;

  gchar *vertex_src;
  gchar *fragment_src;

  GLuint vertex_shader;
  GLuint fragment_shader;

  GLuint transform_buffer_loc;

  self = g_slice_new0 (GlrCanvas);
  self->ref_count = 1;

  self->canvas_tid = gettid ();
  self->target = target;

  /* @FIXME: load the shader sources at compile time */
  if (! g_file_get_contents (CURRENT_DIR "/vertex.glsl",
                             &vertex_src,
                             NULL,
                             &error))
    {
      g_printerr ("Failed to load vertex shader source: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }
  if (! g_file_get_contents (CURRENT_DIR "/fragment.glsl",
                             &fragment_src,
                             NULL,
                             &error))
    {
      g_printerr ("Failed to load fragment shader source: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }

  // setup the shaders
  vertex_shader = load_shader (vertex_src, GL_VERTEX_SHADER);
  fragment_shader = load_shader (fragment_src, GL_FRAGMENT_SHADER);
  g_free (vertex_src);
  g_free (fragment_src);

  self->shader_program = glCreateProgram ();
  glAttachShader (self->shader_program, vertex_shader);
  glAttachShader (self->shader_program, fragment_shader);

  glBindAttribLocation (self->shader_program, PRIMITIVE_ATTR, "vertex");
  glBindAttribLocation (self->shader_program, LAYOUT_ATTR, "lyt");
  glBindAttribLocation (self->shader_program, COLOR_ATTR, "col");
  glBindAttribLocation (self->shader_program, CONFIG_ATTR, "config");
  glBindAttribLocation (self->shader_program, TEX_AREA_ATTR, "tex_area");

  glLinkProgram (self->shader_program);

  glDeleteShader (vertex_shader);
  glDeleteShader (fragment_shader);

  glUseProgram (self->shader_program);

  // get uniform locations
  self->width_loc = glGetUniformLocation (self->shader_program, "canvas_width");
  self->height_loc = glGetUniformLocation (self->shader_program, "canvas_height");

  // transform buffer
  glEnable (GL_TEXTURE_2D);
  glGenTextures (1, &self->transform_buffer_tex);
  glActiveTexture (GL_TEXTURE8);
  glBindTexture (GL_TEXTURE_2D, self->transform_buffer_tex);
  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D (GL_TEXTURE_2D,
                0, GL_RGBA32F,
                TRANSFORM_TEX_WIDTH,
                TRANSFORM_TEX_HEIGHT,
                0,
                GL_RGBA, GL_FLOAT,
                NULL);
  transform_buffer_loc = glGetUniformLocation (self->shader_program,
                                               "transform_buffer");
  glUniform1i (transform_buffer_loc, 8);

  /* layers */
  g_mutex_init (&self->layers_mutex);
  self->attached_layers = g_queue_new ();

  return self;
}

GlrCanvas *
glr_canvas_ref (GlrCanvas *self)
{
  g_assert (self != NULL);
  g_assert (self->ref_count > 0);

  g_atomic_int_inc (&self->ref_count);

  return self;
}

void
glr_canvas_unref (GlrCanvas *self)
{
  g_assert (self != NULL);
  g_assert (self->ref_count > 0);

  if (g_atomic_int_dec_and_test (&self->ref_count))
    glr_canvas_free (self);
}

void
glr_canvas_start_frame (GlrCanvas *self)
{
  if (self->frame_started)
    {
      g_warning ("Frame already started. Ignoring.");
      return;
    }

  self->frame_started = TRUE;
  self->frame_initialized = FALSE;
}

void
glr_canvas_finish_frame (GlrCanvas *self)
{
  if (! self->frame_started)
    {
      g_warning ("No frame started. Ignoring.");
      return;
    }

  flush (self);

  glPopAttrib ();
  glActiveTexture (GL_TEXTURE0);

  self->frame_started = FALSE;

  self->frame_initialized = FALSE;
  self->frame_count++;
}

void
glr_canvas_clear (GlrCanvas *self, GlrPaint *paint)
{
  self->clear_color = paint->color;

  if (self->frame_initialized)
    {
      clear_background (self);
    }
  else
    {
      self->pending_clear = TRUE;
    }
}

GlrTarget *
glr_canvas_get_target (GlrCanvas *self)
{
  return self->target;
}

void
glr_canvas_attach_layer (GlrCanvas *self, guint index, GlrLayer *layer)
{
  LayerAttachment *layer_attachment;

  layer_attachment = g_slice_new0 (LayerAttachment);
  layer_attachment->index = index;
  layer_attachment->layer = glr_layer_ref (layer);

  /* @TODO: check that we are inside a frame request (i.e, glr_canvas_start_frame()) */

  g_mutex_lock (&self->layers_mutex);
  g_queue_insert_sorted (self->attached_layers,
                         layer_attachment,
                         (GCompareDataFunc) sort_layers_func,
                         NULL);
  g_mutex_unlock (&self->layers_mutex);
}
