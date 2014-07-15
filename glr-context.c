#include "glr-context.h"

#include <EGL/egl.h>
#include <glib.h>
#include "glr-priv.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#define DEFAULT_NUM_ROUND_CORNER_VERTICES 32

struct _GlrContext
{
  gint ref_count;
  pid_t tid;

  EGLDisplay egl_display;
  EGLContext gl_context;

  GlrTexCache *tex_cache;

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
glr_context_free (GlrContext *self)
{
  /* stop GL thread */
  g_mutex_lock (&self->gl_mutex);
  self->gl_thread_quit = TRUE;
  g_mutex_unlock (&self->gl_mutex);

  g_cond_signal (&self->cmd_queue_cond);
  g_thread_join (self->gl_thread);

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

void
glr_context_lock_gl (GlrContext *self)
{
  g_mutex_lock (&self->gl_mutex);
}

void
glr_context_unlock_gl (GlrContext *self)
{
  g_mutex_unlock (&self->gl_mutex);
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
