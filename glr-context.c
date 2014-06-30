#include "glr-context.h"

#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#define MAX_PRIMITIVES 32

#define DEFAULT_NUM_ROUND_CORNER_VERTICES 32

struct _GlrContext
{
  gint ref_count;
  pid_t tid;

  GlrTexCache *tex_cache;

  GlrPrimitive *primitives[MAX_PRIMITIVES];
  guint num_primitives;
};

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

  for (i = 0; i < self->num_primitives; i++)
    {
      g_assert (self->primitives[i] != NULL);
      free_primitive (self->primitives[i]);
    }

  glr_tex_cache_unref (self->tex_cache);

  g_slice_free (GlrContext, self);
  self = NULL;

  g_print ("GlrContext freed\n");
}

/* public */

GlrContext *
glr_context_new (void)
{
  GlrContext *self;
  GlrPrimitive * primitive;

  self = g_slice_new0 (GlrContext);
  self->ref_count = 1;
  self->tid = gettid ();

  self->tex_cache = glr_tex_cache_new ();

  /* filled rectangle primitive */
  primitive = g_slice_new (GlrPrimitive);
  primitive->mode = GL_TRIANGLE_FAN;
  primitive->num_vertices = 4;
  primitive->vertices =
    g_slice_alloc (primitive->num_vertices * sizeof (gfloat) * 2);
  memcpy (primitive->vertices,
          rect_vertices,
          primitive->num_vertices * sizeof (gfloat) * 2);

  self->primitives[GLR_PRIMITIVE_RECT_FILL] = primitive;
  self->num_primitives++;

  /* stroked rectangle primitive */
  primitive = g_slice_new (GlrPrimitive);
  primitive->mode = GL_LINE_LOOP;
  primitive->num_vertices = 4;
  primitive->vertices =
    g_slice_alloc (primitive->num_vertices * sizeof (gfloat) * 2);
  memcpy (primitive->vertices,
          rect_vertices,
          primitive->num_vertices * sizeof (gfloat) * 2);

  self->primitives[GLR_PRIMITIVE_RECT_STROKE] = primitive;
  self->num_primitives++;

  /* round corner primitive */
  primitive = g_slice_new (GlrPrimitive);
  primitive->mode = GL_TRIANGLE_FAN;
  primitive->num_vertices = DEFAULT_NUM_ROUND_CORNER_VERTICES;
  primitive->vertices =
    g_slice_alloc0 (primitive->num_vertices * sizeof (gfloat) * 2);

  gint i, c = 0;
  gfloat step = (M_PI/2.0) / (primitive->num_vertices - 2);

  /* first vetice is (0,0) */
  c++;

  for (i = primitive->num_vertices - 2; i >= 0; i--)
    {
      gfloat a, x, y;

      a = step * i;
      x = cos (a);
      y = sin (a);

      primitive->vertices[c * 2 + 0] = x;
      primitive->vertices[c * 2 + 1] = y;
      c++;
    }

  self->primitives[GLR_PRIMITIVE_ROUND_CORNER_FILL] = primitive;
  self->num_primitives++;

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

const GlrPrimitive *
glr_context_get_primitive (GlrContext *self, guint primitive_id)
{
  g_assert (primitive_id >= 0 && primitive_id < self->num_primitives);

  /* @FIXME: this will eventually need to be thread-safe */
  return self->primitives[primitive_id];
}

GlrTexCache *
glr_context_get_texture_cache (GlrContext *self)
{
  return self->tex_cache;
}
