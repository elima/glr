#include "glr-batch.h"

#include <assert.h>
#include <glib.h>
#include <math.h>
#include <string.h>

#define FIXED_ATTRS_BUFFER_INITIAL_SIZE    (8192 *    1) /* 8KB */
#define FIXED_ATTRS_BUFFER_MAXIMUM_SIZE    (8192 * 1024) /* 8MB */

#define DYN_ATTRS_TEX_WIDTH  1024
#define DYN_ATTRS_TEX_HEIGHT 1024
#define DYN_ATTRS_BUFFER_INITIAL_SIZE (DYN_ATTRS_TEX_WIDTH  * 1 * 16)
#define DYN_ATTRS_BUFFER_MAXIMUM_SIZE (DYN_ATTRS_TEX_WIDTH * DYN_ATTRS_TEX_HEIGHT * 16)

#define LAYOUT_ATTR 0
#define COLOR_ATTR  1
#define CONFIG_ATTR 2

struct _GlrBatch
{
  int ref_count;

  size_t num_instances;

  GLuint fixed_attrs_vbo;
  uint8_t *fixed_attrs_buf;
  size_t fixed_attrs_vbo_size;
  size_t fixed_attrs_size;
  size_t fixed_attrs_used_size;
  size_t fixed_attrs_uploaded_size;

  GLuint dyn_attrs_tex;
  uint8_t *dyn_attrs_buffer;
  size_t dyn_attrs_buffer_size;
  size_t dyn_attrs_uploaded_samples;
  size_t dyn_attrs_sample_count;
};

typedef struct
{
  GlrLayout lyt;
  GlrColor color;
  GlrInstanceConfig config;
} InstanceAttr;

static void
glr_batch_free (GlrBatch *self)
{
  free (self->dyn_attrs_buffer);
  glDeleteTextures (1, &self->dyn_attrs_tex);

  free (self->fixed_attrs_buf);
  glDeleteBuffers (1, &self->fixed_attrs_vbo);

  free (self);
  self = NULL;

  // g_print ("GlrBatch freed\n");
}

static bool
check_fixed_attrs_buffers_maybe_grow (GlrBatch *self)
{
  size_t new_size;

  new_size = self->fixed_attrs_used_size
    + (sizeof (GlrLayout) + sizeof (GlrInstanceConfig));

  if (new_size < self->fixed_attrs_size)
    return true;

  // need to grow buffers

  // check if that's possible
  if (new_size > FIXED_ATTRS_BUFFER_MAXIMUM_SIZE)
    return false;

  self->fixed_attrs_size =
    MIN (self->fixed_attrs_size * 2, FIXED_ATTRS_BUFFER_MAXIMUM_SIZE);

  self->fixed_attrs_buf = realloc (self->fixed_attrs_buf,
                                   self->fixed_attrs_size);

  return true;
}

static bool
check_dyn_attrs_buffer_maybe_grow (GlrBatch *self, size_t samples_needed)
{
  size_t new_size;

  new_size = (self->dyn_attrs_sample_count + samples_needed) * 4 * sizeof (float);
  if (new_size < self->dyn_attrs_buffer_size)
    return TRUE;

  // need to grow buffers

  // check if that's possible
  if (new_size > DYN_ATTRS_BUFFER_MAXIMUM_SIZE)
    return false;

  self->dyn_attrs_buffer_size = MIN (self->dyn_attrs_buffer_size * 2,
                                     DYN_ATTRS_BUFFER_MAXIMUM_SIZE);
  self->dyn_attrs_buffer = realloc (self->dyn_attrs_buffer,
                                    self->dyn_attrs_buffer_size);

  return true;
}

/* public API */

GlrBatch *
glr_batch_new (void)
{
  GlrBatch *self;

  self = calloc (1, sizeof (GlrBatch));
  self->ref_count = 1;

  // fixed attrs buffer
  self->fixed_attrs_size = FIXED_ATTRS_BUFFER_INITIAL_SIZE;
  self->fixed_attrs_buf = malloc (self->fixed_attrs_size);
  self->fixed_attrs_vbo_size = 0;
  self->fixed_attrs_used_size = 0;
  self->fixed_attrs_uploaded_size = 0;

  glGenBuffers (1, &self->fixed_attrs_vbo);

  // dyn attrs buffer
  self->dyn_attrs_buffer_size = DYN_ATTRS_BUFFER_INITIAL_SIZE;
  self->dyn_attrs_buffer = malloc (self->dyn_attrs_buffer_size);
  self->dyn_attrs_sample_count = 0;
  self->dyn_attrs_uploaded_samples = 0;

  glGenTextures (1, &self->dyn_attrs_tex);
  glBindTexture (GL_TEXTURE_2D, self->dyn_attrs_tex);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D (GL_TEXTURE_2D,
                0,
                GL_RGBA32F,
                DYN_ATTRS_TEX_WIDTH,
                DYN_ATTRS_TEX_HEIGHT,
                0,
                GL_RGBA,
                GL_FLOAT,
                NULL);
  glBindTexture (GL_TEXTURE_2D, 0);

  return self;
}

GlrBatch *
glr_batch_ref (GlrBatch *self)
{
  assert (self != NULL);
  assert (self->ref_count > 0);

  g_atomic_int_inc (&self->ref_count);

  return self;
}

void
glr_batch_unref (GlrBatch *self)
{
  assert (self != NULL);
  assert (self->ref_count > 0);

  if (g_atomic_int_dec_and_test (&self->ref_count))
    glr_batch_free (self);
}

bool
glr_batch_is_full (GlrBatch *self)
{
  return
    self->fixed_attrs_used_size + sizeof (InstanceAttr) > FIXED_ATTRS_BUFFER_MAXIMUM_SIZE;
}

bool
glr_batch_add_instance (GlrBatch                *self,
                        const GlrLayout         *layout,
                        GlrColor                 color,
                        const GlrInstanceConfig  config)
{
  if (! check_fixed_attrs_buffers_maybe_grow (self))
    {
      g_warning ("Fixed attributes buffers are full. Ignoring instance.");
      return false;
    }

  InstanceAttr attr;

  memcpy (&attr.lyt, layout, sizeof (GlrLayout));
  memcpy (&attr.color, &color, sizeof (GlrColor));
  memcpy (&attr.config, config, sizeof (GlrInstanceConfig));

  memcpy (self->fixed_attrs_buf + self->fixed_attrs_used_size,
          &attr, sizeof (InstanceAttr));
  self->fixed_attrs_used_size += sizeof (InstanceAttr);

  self->num_instances++;

  return true;
}

bool
glr_batch_draw (GlrBatch *self, GLuint shader_program)
{
  GLsizei tex_height;

  if (self->num_instances == 0)
    return false;

  glBindBuffer (GL_ARRAY_BUFFER, self->fixed_attrs_vbo);

  // upload fixed attribute's data to VBO
  if (self->fixed_attrs_vbo_size < self->fixed_attrs_used_size)
    {
      glBufferData (GL_ARRAY_BUFFER,
                    self->fixed_attrs_used_size,
                    self->fixed_attrs_buf,
                    GL_DYNAMIC_DRAW);
      self->fixed_attrs_vbo_size = self->fixed_attrs_used_size;
    }
  else if (self->fixed_attrs_uploaded_size < self->fixed_attrs_used_size)
    {
      glBufferSubData (GL_ARRAY_BUFFER,
                       0,
                       self->fixed_attrs_used_size,
                       self->fixed_attrs_buf);
    }

  self->fixed_attrs_uploaded_size = self->fixed_attrs_used_size;

  // layout attr
  glVertexAttribPointer (LAYOUT_ATTR,
                         4,
                         GL_FLOAT,
                         GL_FALSE,
                         sizeof (InstanceAttr),
                         (const GLvoid *) (0));
  glEnableVertexAttribArray (LAYOUT_ATTR);
  glVertexAttribDivisor (LAYOUT_ATTR, 1);

  // color attr
  glVertexAttribPointer (COLOR_ATTR,
                         4,
                         GL_UNSIGNED_BYTE,
                         GL_TRUE,
                         sizeof (InstanceAttr),
                         (const GLvoid *) (sizeof (GlrLayout)));
  glEnableVertexAttribArray (COLOR_ATTR);
  glVertexAttribDivisor (COLOR_ATTR, 1);

  // config attr
  // @FIXME: we need runtime detection of env to address GPU quirks
  const char *backend = getenv ("GLR_BACKEND");
  glVertexAttribPointer (CONFIG_ATTR,
                         4,
                         (backend == NULL || strcmp (backend, "fbdev") != 0) ? GL_FLOAT : GL_UNSIGNED_INT,
                         GL_FALSE,
                         sizeof (InstanceAttr),
                         (const GLvoid *) (sizeof (GlrLayout) + sizeof (GlrColor)));
  glEnableVertexAttribArray (CONFIG_ATTR);
  glVertexAttribDivisor (CONFIG_ATTR, 1);

  // upload dynamic attribute's data to texture
  glActiveTexture (GL_TEXTURE8);
  glBindTexture (GL_TEXTURE_2D, self->dyn_attrs_tex);
  glUniform1i (glGetUniformLocation (shader_program, "dyn_attrs_tex"), 8);

  tex_height = ((GLsizei) ceil (self->dyn_attrs_sample_count /
                                (float) DYN_ATTRS_TEX_WIDTH));
  if (tex_height > 0)
    {
      if (self->dyn_attrs_sample_count > self->dyn_attrs_uploaded_samples)
        {
          glTexSubImage2D (GL_TEXTURE_2D,
                           0,
                           0, 0,
                           DYN_ATTRS_TEX_WIDTH,
                           tex_height,
                           GL_RGBA,
                           GL_FLOAT,
                           self->dyn_attrs_buffer);
          self->dyn_attrs_uploaded_samples = self->dyn_attrs_sample_count;
        }
    }

  // draw
  glDrawArraysInstanced (GL_TRIANGLE_FAN,
                         0,
                         4,
                         self->num_instances);

  return true;
}

void
glr_batch_reset (GlrBatch *self)
{
  self->num_instances = 0;

  self->fixed_attrs_used_size = 0;
  self->fixed_attrs_uploaded_size = 0;

  self->dyn_attrs_sample_count = 0;
  self->dyn_attrs_uploaded_samples = 0;
}

size_t
glr_batch_add_dyn_attr (GlrBatch *self, const void *attr_data, size_t size)
{
  size_t num_samples;
  size_t offset;

  assert (size > 0);

  num_samples = size / 16;
  if (size % 16 > 0)
    num_samples++;

  if (! check_dyn_attrs_buffer_maybe_grow (self, num_samples))
    {
      g_warning ("Dynamic attributes buffer is full. Ignoring.");
      return 0;
    }

  offset = self->dyn_attrs_sample_count * 4 * sizeof (float);
  memcpy (self->dyn_attrs_buffer + offset, attr_data, size);
  self->dyn_attrs_sample_count += num_samples;

  return self->dyn_attrs_sample_count - num_samples + 1;
}
