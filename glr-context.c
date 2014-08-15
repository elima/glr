#include "glr-context.h"

#include <assert.h>
#include <glib.h>
#include "glr-priv.h"

struct _GlrContext
{
  int ref_count;

  GlrTexCache *tex_cache;
};

static void
glr_context_free (GlrContext *self)
{
  glr_tex_cache_unref (self->tex_cache);

  free (self);
  self = NULL;

  printf ("GlrContext freed\n");
}

/* public API */

GlrContext *
glr_context_new (void)
{
  GlrContext *self;

  self = calloc (1, sizeof (GlrContext));
  self->ref_count = 1;

  /* @TODO: detect OpenGL ES version */
  const char *ver_st;
  ver_st = (const char *) glGetString (GL_VERSION);
  printf ("%s\n", ver_st);

  const char *ext_st;
  ext_st = (const char *) glGetString (GL_EXTENSIONS);
  printf ("%s\n", ext_st);

  const char *vendor_st;
  vendor_st = (const char *) glGetString (GL_VENDOR);
  printf ("%s\n", vendor_st);

  self->tex_cache = glr_tex_cache_new (self);

  return self;
}

GlrContext *
glr_context_ref (GlrContext *self)
{
  assert (self != NULL);
  assert (self->ref_count > 0);

  g_atomic_int_inc (&self->ref_count);

  return self;
}

void
glr_context_unref (GlrContext *self)
{
  assert (self != NULL);
  assert (self->ref_count > 0);

  if (g_atomic_int_dec_and_test (&self->ref_count))
    glr_context_free (self);
}

GlrTexCache *
glr_context_get_texture_cache (GlrContext *self)
{
  return self->tex_cache;
}
