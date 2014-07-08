#include "glr-target.h"

#include "glr-symbols.h"

struct _GlrTarget
{
  gint ref_count;

  GLuint fbo;
  GLuint fbo_render_buf;
  guint8 msaa_samples;
  guint32 width;
  guint32 height;
};

static void
glr_target_free (GlrTarget *self)
{
  glDeleteRenderbuffers (1, &self->fbo_render_buf);
  glDeleteFramebuffers (1, &self->fbo);

  g_slice_free (GlrTarget, self);
  self = NULL;
}

/* public API */

GlrTarget *
glr_target_new (guint32 width, guint32 height, guint8 msaa_samples)
{
  GlrTarget *self;

  self = g_slice_new0 (GlrTarget);
  self->ref_count = 1;

  self->msaa_samples = msaa_samples;
  self->width = width;
  self->height = height;

  glEnable (GL_MULTISAMPLE);

  glGenRenderbuffers (1, &self->fbo_render_buf);
  glBindRenderbuffer (GL_RENDERBUFFER, self->fbo_render_buf);
  glRenderbufferStorageMultisample (GL_RENDERBUFFER,
                                    self->msaa_samples,
                                    GL_RGBA8,
                                    self->width,
                                    self->height);

  glGenFramebuffers (1, &self->fbo);
  glBindFramebuffer (GL_FRAMEBUFFER, self->fbo);

  glFramebufferRenderbuffer (GL_FRAMEBUFFER,
                             GL_COLOR_ATTACHMENT0,
                             GL_RENDERBUFFER,
                             self->fbo_render_buf);

  if (glCheckFramebufferStatus (GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      g_printerr ("Failed to create a working framebuffer\n");
      return NULL;
    }

  glBindFramebuffer (GL_FRAMEBUFFER, 0);

  return self;
}

GlrTarget *
glr_target_ref (GlrTarget *self)
{
  g_assert (self != NULL);
  g_assert (self->ref_count > 0);

  g_atomic_int_inc (&self->ref_count);

  return self;
}

void
glr_target_unref (GlrTarget *self)
{
  g_assert (self != NULL);
  g_assert (self->ref_count > 0);

  if (g_atomic_int_dec_and_test (&self->ref_count))
    glr_target_free (self);
}

void
glr_target_get_size (GlrTarget *self, guint32 *width, guint32 *height)
{
  if (width != NULL)
    *width = self->width;

  if (height != NULL)
    *height = self->height;
}

GLuint
glr_target_get_framebuffer (GlrTarget *self)
{
  return self->fbo;
}

void
glr_target_resize (GlrTarget *self, guint width, guint height)
{
  self->width = width;
  self->height = height;

  glBindRenderbuffer (GL_RENDERBUFFER, self->fbo_render_buf);
  glRenderbufferStorageMultisample (GL_RENDERBUFFER,
                                    self->msaa_samples,
                                    GL_RGBA8,
                                    self->width,
                                    self->height);
}
