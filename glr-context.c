#include "glr-context.h"

#include <EGL/egl.h>
#include <glib.h>
#include "glr-priv.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#define MAX_PRIMITIVES 8

#define DEFAULT_NUM_ROUND_CORNER_VERTICES 32

struct _GlrContext
{
  gint ref_count;
  pid_t tid;

  EGLDisplay egl_display;
  EGLContext gl_context;

  GlrTexCache *tex_cache;

  GHashTable *dyn_primitives;
  GlrPrimitive *primitives[MAX_PRIMITIVES];
  guint num_primitives;

  GQueue *cmd_queue;
  GMutex cmd_queue_mutex;
  GCond cmd_queue_cond;

  GLuint tex_table[64];

  GThread *gl_thread;
  GMutex gl_mutex;
  gboolean gl_thread_quit;
};

typedef struct
{
  guint type;
  gpointer data;
  GCond *cond;
} GlrDeferred;

static const gfloat rect_vertices[] = {
   0.0,  0.0,
   1.0,  0.0,
   1.0,  1.0,
   0.0,  1.0,
};

static pid_t
gettid (void)
{
  return (pid_t) syscall (SYS_gettid);
}

static void
free_primitive (GlrPrimitive *primitive)
{
  g_slice_free1 (primitive->num_vertices * sizeof (gfloat) * 2,
                 primitive->vertices);

  g_slice_free (GlrPrimitive, primitive);
}

static void
glr_context_free (GlrContext *self)
{
  gint i;

  /* stop GL thread */
  g_mutex_lock (&self->gl_mutex);
  self->gl_thread_quit = TRUE;
  g_mutex_unlock (&self->gl_mutex);

  g_cond_signal (&self->cmd_queue_cond);
  g_thread_join (self->gl_thread);

  /* free primitives */
  for (i = 0; i < self->num_primitives; i++)
    {
      if (self->primitives[i] != NULL)
        free_primitive (self->primitives[i]);
    }
  g_hash_table_unref (self->dyn_primitives);

  glr_tex_cache_unref (self->tex_cache);

  g_mutex_lock (&self->cmd_queue_mutex);
  g_queue_free_full (self->cmd_queue, g_free);
  g_mutex_unlock (&self->cmd_queue_mutex);
  g_mutex_clear (&self->cmd_queue_mutex);
  g_cond_clear (&self->cmd_queue_cond);

  g_mutex_clear (&self->gl_mutex);

  g_slice_free (GlrContext, self);
  self = NULL;

  g_print ("GlrContext freed\n");
}

static gpointer
gl_thread_func (gpointer user_data)
{
  GlrContext *self = user_data;
  EGLContext egl_context;

  EGLint ctx_attr[] =
    {
      EGL_CONTEXT_CLIENT_VERSION, 3,
      EGL_NONE
    };
  egl_context = eglCreateContext (self->egl_display, NULL, self->gl_context, ctx_attr);
  if (egl_context == EGL_NO_CONTEXT)
    {
      g_printerr ("Unable to create EGL context (eglError: %u)\n", eglGetError());
      return NULL;
    }

  eglMakeCurrent (self->egl_display, NULL, NULL, egl_context);

  g_mutex_lock (&self->gl_mutex);
  g_mutex_lock (&self->cmd_queue_mutex);

  while (! self->gl_thread_quit)
    {
      GlrDeferred *cmd;

      while ( (cmd = g_queue_pop_head (self->cmd_queue)) != NULL)
        {
          gpointer data;

          g_mutex_unlock (&self->cmd_queue_mutex);

          if (cmd->type == GLR_CMD_UPLOAD_TO_TEX)
            {
              GlrCmdUploadToTex *args = data = cmd->data;

              glActiveTexture (GL_TEXTURE0 + args->texture_id);
              glBindTexture (GL_TEXTURE_2D, self->tex_table[args->texture_id]);
              glTexSubImage2D (GL_TEXTURE_2D,
                               0,
                               args->x_offset, args->y_offset,
                               args->width,
                               args->height,
                               args->format,
                               args->type,
                               args->buffer);
              g_free (args->buffer);
            }
          else if (cmd->type == GLR_CMD_CREATE_TEX)
            {
              GlrCmdCreateTex *args = data = cmd->data;
              GLuint tex;

              glActiveTexture (GL_TEXTURE0 + args->texture_id);
              glGenTextures (1, &tex);
              self->tex_table[args->texture_id] = tex;
              glBindTexture (GL_TEXTURE_2D, tex);
              glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
              glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
              glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
              glTexImage2D (GL_TEXTURE_2D,
                            0,
                            args->internal_format,
                            args->width, args->height,
                            0,
                            args->format,
                            args->type,
                            NULL);
            }
          else if (cmd->type == GLR_CMD_COMMIT_CANVAS_FRAME)
            {
              GlrCmdCommitCanvasFrame *args = data = cmd->data;

              gint i;
              for (i = 0; i < 8; i++)
                {
                  if (self->tex_table[i] != 0)
                    {
                      glActiveTexture (GL_TEXTURE0 + i);
                      glBindTexture (GL_TEXTURE_2D, self->tex_table[i]);
                    }
                }

              glr_canvas_commit_frame (args->canvas);
              glr_canvas_unref (args->canvas);
            }

          if (cmd->cond != NULL)
            g_cond_signal (cmd->cond);

          g_free (data);
          g_free (cmd);

          g_mutex_lock (&self->cmd_queue_mutex);
        }

      /* sleep until we have commands in queue again */
      while (! self->gl_thread_quit && g_queue_get_length (self->cmd_queue) == 0)
        {
          g_mutex_unlock (&self->gl_mutex);
          g_cond_wait (&self->cmd_queue_cond, &self->cmd_queue_mutex);
          g_mutex_lock (&self->gl_mutex);
        }
    }

  g_mutex_unlock (&self->gl_mutex);
  g_mutex_unlock (&self->cmd_queue_mutex);

  eglDestroyContext (self->egl_display, egl_context);

  return NULL;
}

/* internal API */

static GlrPrimitive *
glr_primitive_new (GLenum mode, gsize num_vertices)
{
  GlrPrimitive *self;

  self = g_slice_new0 (GlrPrimitive);
  self->mode = mode;
  self->num_vertices = num_vertices;
  self->vertices =
    g_slice_alloc (self->num_vertices * sizeof (gfloat) * 2);

  return self;
}

static void
glr_primitive_add_vertex (GlrPrimitive *self, gfloat x, gfloat y)
{
  self->vertices[self->vertex_count * 2 + 0] = x;
  self->vertices[self->vertex_count * 2 + 1] = y;
  self->vertex_count++;
}

const GlrPrimitive *
glr_context_get_primitive (GlrContext *self, guint primitive_id)
{
  GlrPrimitive *primitive;

  /* @FIXME: this will eventually need to be thread-safe */

  if (self->primitives[primitive_id] != NULL)
    return self->primitives[primitive_id];

  switch (primitive_id)
    {
    case GLR_PRIMITIVE_RECT_FILL:
      {
        primitive = glr_primitive_new (GL_TRIANGLE_FAN, 4);
        memcpy (primitive->vertices,
                rect_vertices,
                primitive->num_vertices * sizeof (gfloat) * 2);
        break;
      }

    case GLR_PRIMITIVE_ROUND_CORNER_FILL:
      {
        /* filled round corner primitive */
        primitive = glr_primitive_new (GL_TRIANGLE_FAN,
                                       DEFAULT_NUM_ROUND_CORNER_VERTICES);

        gint i, c = 0;
        gfloat step = (M_PI/2.0) / (primitive->num_vertices - 2);

        /* first vertice is (0,0) */
        c++;

        for (i = primitive->num_vertices - 2; i >= 0; i--)
          {
            gfloat a, x, y;

            a = step * i;
            x = cos (a);
            y = sin (a);

            glr_primitive_add_vertex (primitive, x, y);
          }

        break;
      }

    case GLR_PRIMITIVE_RECT_STROKE:
      {
        /* stroked rectangle primitive */
        primitive = glr_primitive_new (GL_LINE_LOOP, 4);
        memcpy (primitive->vertices,
                rect_vertices,
                primitive->num_vertices * sizeof (gfloat) * 2);
        break;
      }

    default:
      g_assert_not_reached ();
    }

  self->primitives[primitive_id] = primitive;
  self->num_primitives++;

  return primitive;
}

const GlrPrimitive *
glr_context_get_dynamic_primitive (GlrContext *self,
                                   guint       primitive_id,
                                   gfloat      dyn_value)
{
  gchar *dyn_id;
  GlrPrimitive *primitive;

  dyn_id = g_strdup_printf ("%d:%08f", primitive_id, dyn_value);

  primitive = g_hash_table_lookup (self->dyn_primitives, dyn_id);
  if (primitive != NULL)
    {
      g_free (dyn_id);
      return (const GlrPrimitive *) primitive;
    }

  switch (primitive_id)
    {
    case GLR_PRIMITIVE_ROUND_CORNER_STROKE:
      {
        gint i;

        /* stroked round corner primitive */
        primitive = glr_primitive_new (GL_TRIANGLE_STRIP,
                                       DEFAULT_NUM_ROUND_CORNER_VERTICES * 2);
        gfloat step = (M_PI/2.0) / (DEFAULT_NUM_ROUND_CORNER_VERTICES - 2);
        gfloat border_width = dyn_value;
        gfloat h1 = 1.0 - border_width;

        for (i = DEFAULT_NUM_ROUND_CORNER_VERTICES - 2; i >= 0; i--)
          {
            gfloat a, x, y, x1, y1;

            a = step * i;
            x = cos (a);
            y = sin (a);

            x1 = x * h1;
            y1 = y * h1;

            glr_primitive_add_vertex (primitive, x, y);
            glr_primitive_add_vertex (primitive, x1, y1);
          }

        break;
      }

    default:
      g_assert_not_reached ();
    }

  g_hash_table_insert (self->dyn_primitives, dyn_id, primitive);

  return primitive;
}

GlrTexCache *
glr_context_get_texture_cache (GlrContext *self)
{
  return self->tex_cache;
}

void
glr_context_queue_command (GlrContext *self,
                           GlrCmdType  type,
                           gpointer    data,
                           GCond      *cond)
{
  GlrDeferred *cmd;

  cmd = g_new0 (GlrDeferred, 1);
  cmd->type = type;
  cmd->data = data;
  cmd->cond = cond;

  g_mutex_lock (&self->cmd_queue_mutex);

  g_queue_push_tail (self->cmd_queue, cmd);
  g_cond_signal (&self->cmd_queue_cond);

  g_mutex_unlock (&self->cmd_queue_mutex);
}

/* public API */

GlrContext *
glr_context_new (void)
{
  GlrContext *self;
  GError *error = NULL;

  self = g_slice_new0 (GlrContext);
  self->ref_count = 1;
  self->tid = gettid ();

  /* GL thread */
  self->gl_thread_quit = FALSE;
  self->gl_thread = g_thread_try_new (NULL,
                                      gl_thread_func,
                                      self,
                                      &error);
  if (self->gl_thread == NULL)
    {
      g_printerr ("Failed to create context's thread: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }
  g_mutex_init (&self->gl_mutex);

  self->egl_display = eglGetCurrentDisplay ();
  self->gl_context = eglGetCurrentContext ();

  /* GL command queue */
  self->cmd_queue = g_queue_new ();
  g_mutex_init (&self->cmd_queue_mutex);
  g_cond_init (&self->cmd_queue_cond);

  /* texture id table */
  memset (self->tex_table, 0x00, 64);

  /* detect OpenGL/ES version */
  GLint major, minor;
  const gchar *ver_st;

  ver_st = (const gchar *) glGetString (GL_VERSION);
  glGetIntegerv (GL_MAJOR_VERSION, &major);
  glGetIntegerv (GL_MINOR_VERSION, &minor);
  g_print ("%s\n", ver_st);

  self->tex_cache = glr_tex_cache_new (self);

  /* dynamic primitives */
  self->dyn_primitives = g_hash_table_new_full (g_str_hash,
                                                g_str_equal,
                                                g_free,
                                                (GDestroyNotify) free_primitive);

  return self;
}

GlrContext *
glr_context_ref (GlrContext *self)
{
  g_assert (self != NULL);
  g_assert (self->ref_count > 0);

  g_atomic_int_inc (&self->ref_count);

  return self;
}

void
glr_context_unref (GlrContext *self)
{
  g_assert (self != NULL);
  g_assert (self->ref_count > 0);

  if (g_atomic_int_dec_and_test (&self->ref_count))
    glr_context_free (self);
}
