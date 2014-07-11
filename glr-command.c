#include "glr-command.h"

#include <float.h>
#include <GL/gl.h>
#include "glr-symbols.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define TRANSFORM_TEX_WIDTH  1024
#define TRANSFORM_TEX_HEIGHT  512

#define INITIAL_LAYOUT_BUFFER_SIZE    (8192 * 1) /* 8KB */
#define INITIAL_COLOR_BUFFER_SIZE      INITIAL_LAYOUT_BUFFER_SIZE / 4
#define INITIAL_CONFIG_BUFFER_SIZE     INITIAL_LAYOUT_BUFFER_SIZE / 2
#define INITIAL_TRANSFORM_BUFFER_SIZE (INITIAL_LAYOUT_BUFFER_SIZE * 2)
#define INITIAL_TEX_AREA_BUFFER_SIZE  (INITIAL_LAYOUT_BUFFER_SIZE / 4)

#define MAX_LAYOUT_BUFFER_SIZE    (8192 * 256) /* 2MB */
#define MAX_COLOR_BUFFER_SIZE      MAX_LAYOUT_BUFFER_SIZE / 4
#define MAX_CONFIG_BUFFER_SIZE     MAX_LAYOUT_BUFFER_SIZE / 2
#define MAX_TRANSFORM_BUFFER_SIZE (MAX_LAYOUT_BUFFER_SIZE * 2)
#define MAX_TEX_AREA_BUFFER_SIZE  (MAX_LAYOUT_BUFFER_SIZE / 4)

#define EQUALS(x, y) (fabs (x - y) < DBL_EPSILON * fabs (x + y))
// #define EQUALS(x, y) (fabs (x) - fabs(y) < DBL_EPSILON)

#define PRIMITIVE_ATTR 0
#define LAYOUT_ATTR    1
#define COLOR_ATTR     2
#define CONFIG_ATTR    3
#define TEX_AREA_ATTR  4

struct _GlrCommand
{
  const GlrPrimitive *primitive;

  GLuint num_instances_loc;
  gsize num_instances;

  guint32 *color_buffer;
  gsize color_buffer_size;

  gfloat *layout_buffer;
  gsize layout_buffer_size;

  guint32 *config_buffer;
  gsize config_buffer_size;

  gfloat *transform_buffer;
  gsize transform_buffer_size;
  guint32 transform_buffer_count;

  guint *tex_area_buffer;
  gsize tex_area_buffer_size;
  guint32 tex_area_buffer_count;
};

typedef enum
  {
    GLR_BACKGROUND_COLOR,
    GLR_BACKGROUND_TEXTURE
  } GlrBackgroundType;

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
check_buffers_maybe_grow (GlrCommand *self)
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

  self->color_buffer = grow_buffer (self->color_buffer,
                                    self->color_buffer_size,
                                    self->color_buffer_size * 2);
  self->color_buffer_size *= 2;

  self->config_buffer = grow_buffer (self->config_buffer,
                                     self->config_buffer_size,
                                     self->config_buffer_size * 2);
  self->config_buffer_size *= 2;

  self->transform_buffer = grow_buffer (self->transform_buffer,
                                        self->transform_buffer_size,
                                        self->transform_buffer_size * 2);
  self->transform_buffer_size *= 2;

  self->tex_area_buffer = grow_buffer (self->tex_area_buffer,
                                        self->tex_area_buffer_size,
                                        self->tex_area_buffer_size * 2);
  self->tex_area_buffer_size *= 2;

  return TRUE;
}

static gboolean
has_transform (const GlrTransform *transform)
{
  return
    ! EQUALS (transform->rotation_z, 0.0)
    || ! EQUALS (transform->parent_rotation_z, 0.0)
    || ! EQUALS (transform->scale_x, 1.0)
    || ! EQUALS (transform->scale_y, 1.0);
}

/* public */

GlrCommand *
glr_command_new (const GlrPrimitive *primitive)
{
  GlrCommand *self;

  self = g_slice_new0 (GlrCommand);
  self->primitive = primitive;

  // layout buffer
  self->layout_buffer_size = INITIAL_LAYOUT_BUFFER_SIZE;
  self->layout_buffer = g_slice_alloc (self->layout_buffer_size);

  // color buffer
  self->color_buffer_size = INITIAL_COLOR_BUFFER_SIZE;
  self->color_buffer = g_slice_alloc (self->color_buffer_size);

  // config buffer
  self->config_buffer_size = INITIAL_CONFIG_BUFFER_SIZE;
  self->config_buffer = g_slice_alloc (self->config_buffer_size);

  // transform buffer
  self->transform_buffer_count = 0;
  self->transform_buffer_size = INITIAL_TRANSFORM_BUFFER_SIZE;
  self->transform_buffer = g_slice_alloc (self->transform_buffer_size);

  // texture area buffer
  self->tex_area_buffer_count = 0;
  self->tex_area_buffer_size = INITIAL_TEX_AREA_BUFFER_SIZE;
  self->tex_area_buffer = g_slice_alloc (self->tex_area_buffer_size);

  return self;
}

void
glr_command_free (GlrCommand *self)
{
  g_slice_free1 (self->layout_buffer_size, self->layout_buffer);
  g_slice_free1 (self->color_buffer_size, self->color_buffer);
  g_slice_free1 (self->config_buffer_size, self->config_buffer);
  g_slice_free1 (self->transform_buffer_size, self->transform_buffer);
  g_slice_free1 (self->tex_area_buffer_size, self->tex_area_buffer);

  g_slice_free (GlrCommand, self);
  self = NULL;
}

gboolean
glr_command_is_full (GlrCommand *self)
{
  return
    (self->num_instances + 1) * sizeof (GlrLayout) > MAX_LAYOUT_BUFFER_SIZE;
}

gboolean
glr_command_add_instance (GlrCommand          *self,
                          const GlrLayout     *layout,
                          guint32              color,
                          const GlrTransform  *transform,
                          const GlrTexSurface *tex_surface)
{
  guint32 layout_offset;
  guint32 color_offset;
  guint32 config_offset;
  guint32 tex_area_offset;
  guint32 config1 = 0;

  if (! check_buffers_maybe_grow (self))
    {
      g_warning ("Draw buffers full. Consider creating another layer. Ignoring instance.");
      return FALSE;
    }

  /* store layout */
  layout_offset = self->num_instances * 4;
  memcpy (&self->layout_buffer[layout_offset], layout, sizeof (GlrLayout));

  /* store color */
  color_offset = self->num_instances;
  self->color_buffer[color_offset] = (guint32) color;

  config_offset = self->num_instances * 2;

  if (tex_surface != NULL)
    {
      /* store texture surface */
      tex_area_offset = self->num_instances;
      self->tex_area_buffer[tex_area_offset] =
        (tex_surface->left & 0xFFFF) << 16 | (tex_surface->top & 0xFFFF);
      self->tex_area_buffer_count += 1;

      /* first config word (32bits) stores background type (3bits) */
      config1 = (GLR_BACKGROUND_TEXTURE << 29);

      /* bits 25 to 28 encode texutre id */
      config1 |= (tex_surface->tex_id << 25);
    }
  else
    {
      config1 = (GLR_BACKGROUND_COLOR << 29);
    }

  self->config_buffer[config_offset] = config1;

  /* store transform */
  if (has_transform (transform))
    {
      guint32 transform_offset;

      transform_offset = self->transform_buffer_count * 4;
      memcpy (&self->transform_buffer[transform_offset],
              transform,
              sizeof (GlrTransform));
      self->config_buffer[config_offset + 1] = self->transform_buffer_count + 1;
      self->transform_buffer_count += 2;
    }
  else
    {
      self->config_buffer[config_offset + 1] = 0;
    }

  self->num_instances++;

  return TRUE;
}

gboolean
glr_command_draw (GlrCommand *self, GLuint shader_program)
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

  /* upload color VBO */
  glVertexAttribPointer (COLOR_ATTR,
                         1,
                         GL_FLOAT,
                         GL_FALSE,
                         0,
                         self->color_buffer);
  glEnableVertexAttribArray (COLOR_ATTR);
  glVertexAttribDivisor (COLOR_ATTR, 1);

  /* upload config VBO */
  glVertexAttribPointer (CONFIG_ATTR,
                         2,
                         GL_FLOAT,
                         GL_FALSE,
                         0,
                         self->config_buffer);
  glEnableVertexAttribArray (CONFIG_ATTR);
  glVertexAttribDivisor (CONFIG_ATTR, 1);

  /* upload tex surface VBO */
  glVertexAttribPointer (TEX_AREA_ATTR,
                         1,
                         GL_FLOAT,
                         GL_FALSE,
                         0,
                         self->tex_area_buffer);
  glEnableVertexAttribArray (TEX_AREA_ATTR);
  glVertexAttribDivisor (TEX_AREA_ATTR, 1);

  /* upload transform data */
  tex_height = MAX ((GLsizei) ceil ((self->transform_buffer_count * 1.0) / TRANSFORM_TEX_WIDTH), 1);
  glTexSubImage2D (GL_TEXTURE_2D,
                   0,
                   0, 0,
                   TRANSFORM_TEX_WIDTH,
                   tex_height,
                   GL_RGBA,
                   GL_FLOAT,
                   self->transform_buffer);

  /* upload primitive */
  glVertexAttribPointer (PRIMITIVE_ATTR,
                         2,
                         GL_FLOAT,
                         GL_TRUE,
                         0,
                         self->primitive->vertices);
  glEnableVertexAttribArray (PRIMITIVE_ATTR);

  /* draw */
  glDrawArraysInstanced (self->primitive->mode,
                         0,
                         self->primitive->num_vertices,
                         self->num_instances);

  return TRUE;
}

void
glr_command_reset (GlrCommand *self)
{
  self->num_instances = 0;
  self->transform_buffer_count = 0;
  self->tex_area_buffer_count = 0;
}
