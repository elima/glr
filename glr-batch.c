#include "glr-batch.h"

#include <GL/gl.h>
#include "glr-symbols.h"
#include <math.h>
#include <string.h>

#define DYN_ATTRS_TEX_WIDTH  1024
#define DYN_ATTRS_TEX_HEIGHT 1024

#define INITIAL_LAYOUT_BUFFER_SIZE    (8192 * 1) /* 8KB */
#define INITIAL_CONFIG_BUFFER_SIZE     INITIAL_LAYOUT_BUFFER_SIZE
#define INITIAL_DYN_ATTRS_BUFFER_SIZE (INITIAL_LAYOUT_BUFFER_SIZE * 2)

#define MAX_LAYOUT_BUFFER_SIZE    (8192 * 256) /* 2MB */
#define MAX_CONFIG_BUFFER_SIZE     MAX_LAYOUT_BUFFER_SIZE
#define MAX_DYN_ATTRS_BUFFER_SIZE (MAX_LAYOUT_BUFFER_SIZE * 2)

#define LAYOUT_ATTR 0
#define CONFIG_ATTR 1

struct _GlrBatch
{
  gint ref_count;

  GLuint num_instances_loc;
  gsize num_instances;

  gfloat *layout_buffer;
  gsize layout_buffer_size;

  guint32 *config_buffer;
  gsize config_buffer_size;

  gfloat *dyn_attrs_buffer;
  gsize dyn_attrs_buffer_size;
  goffset dyn_attrs_buffer_count;
};

static void
glr_batch_free (GlrBatch *self)
{
  g_slice_free1 (self->layout_buffer_size, self->layout_buffer);
  g_slice_free1 (self->config_buffer_size, self->config_buffer);
  g_slice_free1 (self->dyn_attrs_buffer_size, self->dyn_attrs_buffer);

  g_slice_free (GlrBatch, self);
  self = NULL;

  // g_print ("GlrBatch freed\n");
}

static gpointer
grow_buffer (gpointer buffer, gsize size, gsize new_size)
{
  gpointer tmp_buf;

  tmp_buf = buffer;
  buffer = g_slice_alloc (new_size);
  memcpy (buffer, tmp_buf, size);
  g_slice_free1 (size, tmp_buf);

  return buffer;
}

static gboolean
check_fixed_attrs_buffers_maybe_grow (GlrBatch *self)
{
  gsize new_size;

  new_size = (self->num_instances + 1) * sizeof (GlrLayout);
  if (new_size < self->layout_buffer_size)
    return TRUE;

  /* need to grow buffers */

  /* check if that's possible */
  if (new_size > MAX_LAYOUT_BUFFER_SIZE)
    return FALSE;

  self->layout_buffer = grow_buffer (self->layout_buffer,
                                     self->layout_buffer_size,
                                     self->layout_buffer_size * 2);
  self->layout_buffer_size *= 2;

  self->config_buffer = grow_buffer (self->config_buffer,
                                     self->config_buffer_size,
                                     self->config_buffer_size * 2);
  self->config_buffer_size *= 2;

  return TRUE;
}

static gboolean
check_dyn_attrs_buffer_maybe_grow (GlrBatch *self, gsize needed_length)
{
  gsize new_size;

  new_size = (self->dyn_attrs_buffer_count + needed_length) * 4 * sizeof (gfloat);
  if (new_size < self->dyn_attrs_buffer_size)
    return TRUE;

  /* need to grow buffers */

  /* check if that's possible */
  if (new_size > MAX_DYN_ATTRS_BUFFER_SIZE)
    {
      return FALSE;
    }

  new_size = MIN (self->dyn_attrs_buffer_size * 2, MAX_DYN_ATTRS_BUFFER_SIZE);
  self->dyn_attrs_buffer = grow_buffer (self->dyn_attrs_buffer,
                                        self->dyn_attrs_buffer_size,
                                        new_size);
  self->dyn_attrs_buffer_size = new_size;

  return TRUE;
}

/* public */

GlrBatch *
glr_batch_new (void)
{
  GlrBatch *self;

  self = g_slice_new0 (GlrBatch);
  self->ref_count = 1;

  /* layout buffer */
  self->layout_buffer_size = INITIAL_LAYOUT_BUFFER_SIZE;
  self->layout_buffer = g_slice_alloc (self->layout_buffer_size);

  /* config buffer */
  self->config_buffer_size = INITIAL_CONFIG_BUFFER_SIZE;
  self->config_buffer = g_slice_alloc (self->config_buffer_size);

  /* dyn attrs buffer */
  self->dyn_attrs_buffer_size = INITIAL_DYN_ATTRS_BUFFER_SIZE;
  self->dyn_attrs_buffer = g_slice_alloc (self->dyn_attrs_buffer_size);

  return self;
}

GlrBatch *
glr_batch_ref (GlrBatch *self)
{
  g_assert (self != NULL);
  g_assert (self->ref_count > 0);

  g_atomic_int_inc (&self->ref_count);

  return self;
}

void
glr_batch_unref (GlrBatch *self)
{
  g_assert (self != NULL);
  g_assert (self->ref_count > 0);

  if (g_atomic_int_dec_and_test (&self->ref_count))
    glr_batch_free (self);
}

gboolean
glr_batch_is_full (GlrBatch *self)
{
  return
    (self->num_instances + 1) * sizeof (GlrLayout) > MAX_LAYOUT_BUFFER_SIZE;
}

gboolean
glr_batch_add_instance (GlrBatch                *self,
                        const GlrInstanceConfig  config,
                        const GlrLayout         *layout)
{
  goffset layout_offset;
  goffset config_offset;

  if (! check_fixed_attrs_buffers_maybe_grow (self))
    {
      g_warning ("Fixed attributes buffers are full. Ignoring instance.");
      return FALSE;
    }

  /* store layout */
  layout_offset = self->num_instances * 4;
  memcpy (&self->layout_buffer[layout_offset], layout, sizeof (GlrLayout));

  config_offset = self->num_instances * 4;
  memcpy (&self->config_buffer[config_offset], config, sizeof (GlrInstanceConfig));

  self->num_instances++;

  return TRUE;
}

gboolean
glr_batch_draw (GlrBatch *self, GLuint shader_program)
{
  GLsizei tex_height;

  if (self->num_instances == 0)
    return FALSE;

  /* upload layout VBO */
  glVertexAttribPointer (LAYOUT_ATTR,
                         4,
                         GL_FLOAT,
                         GL_FALSE,
                         0,
                         self->layout_buffer);
  glEnableVertexAttribArray (LAYOUT_ATTR);
  glVertexAttribDivisor (LAYOUT_ATTR, 1);

  /* upload config VBO */
  glVertexAttribPointer (CONFIG_ATTR,
                         4,
                         GL_FLOAT,
                         GL_FALSE,
                         0,
                         self->config_buffer);
  glEnableVertexAttribArray (CONFIG_ATTR);
  glVertexAttribDivisor (CONFIG_ATTR, 1);

  /* upload dynamic attribute's data to texture */
  tex_height = MAX ((GLsizei) ceil (self->dyn_attrs_buffer_count /
                                    (gfloat) DYN_ATTRS_TEX_WIDTH),
                    1);
  glActiveTexture (GL_TEXTURE9);
  glTexSubImage2D (GL_TEXTURE_2D,
                   0,
                   0, 0,
                   DYN_ATTRS_TEX_WIDTH,
                   tex_height,
                   GL_RGBA,
                   GL_FLOAT,
                   self->dyn_attrs_buffer);

  /* draw */
  glDrawArraysInstanced (GL_TRIANGLE_FAN,
                         0,
                         4,
                         self->num_instances);

  return TRUE;
}

void
glr_batch_reset (GlrBatch *self)
{
  self->num_instances = 0;
  self->dyn_attrs_buffer_count = 0;
}

goffset
glr_batch_add_dyn_attr (GlrBatch *self, const void *attr_data, gsize size)
{
  gsize num_samples;
  goffset offset;

  g_assert (size > 0);

  num_samples = size / 16;
  if (size % 16 > 0)
    num_samples++;

  if (! check_dyn_attrs_buffer_maybe_grow (self, num_samples))
    {
      g_warning ("Dynamic attributes buffer is full. Ignoring.");
      return 0;
    }

  offset = self->dyn_attrs_buffer_count * 4;
  memcpy (&self->dyn_attrs_buffer[offset], attr_data, size);
  self->dyn_attrs_buffer_count += num_samples;

  return self->dyn_attrs_buffer_count - num_samples + 1;
}
