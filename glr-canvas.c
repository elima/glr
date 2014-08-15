#include "glr-canvas.h"

#include <GLES3/gl3.h>
#include <glib.h>
#include <assert.h>
#include "glr-batch.h"
#include "glr-priv.h"
#include "glr-shaders.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define M_PI 3.14159265358979323846

#define EQUALS(x, y)   (fabs (x - y) <= DBL_EPSILON * fabs (x + y))
#define UNEQUALS(x, y) (fabs (x - y)  > DBL_EPSILON * fabs (x + y))

#define LAYOUT_ATTR 0
#define COLOR_ATTR  1
#define CONFIG_ATTR 2

#define INSTANCE_TYPE_MASK          0xC3FFFFFF
#define BORDER_NUM_SAMPLES_MASK     0xFF0FFFFF
#define BACKGROUND_NUM_SAMPLES_MASK 0xFFF0FFFF

#define DEFAULT_EDGE_AA_OFFSET 1.0

#define DEFAULT_Z_DEPTH 5000.0

typedef float GlrColor4f[4];

typedef float Mat4[4][4];
typedef float Vec4[4];
typedef float Vec3[3];

typedef struct __attribute__((__packed__))
{
  Vec3 scale;
  Vec3 rotate;
  Vec3 translate;
  Vec3 origin;
} GlrTransform;

struct _GlrCanvas
{
  int ref_count;

  GlrContext *context;
  GlrTarget *target;

  GLuint shader_program;

  bool frame_initialized;

  uint32_t clear_color;
  bool pending_clear;

  GlrTransform transform;
  size_t current_transform_index;
  GlrBatch *batch;

  float aa_offset;
  float z_depth;

  GLuint proj_matrix_loc;
  GLuint transform_matrix_loc;
  GLuint persp_matrix_loc;
  GLuint aa_offset_loc;

  GlrTexCache *tex_cache;
};

static void
glr_canvas_free (GlrCanvas *self)
{
  glDeleteProgram (self->shader_program);

  glr_context_unref (self->context);
  if (self->target != NULL)
    glr_target_unref (self->target);

  glr_batch_unref (self->batch);

  glr_tex_cache_unref (self->tex_cache);

  g_slice_free (GlrCanvas, self);
  self = NULL;

  g_print ("GlrCanvas freed\n");
}

static void
copy_mat4 (Mat4 src, Mat4 dst)
{
  memcpy (&dst[0][0], &src[0][0], sizeof (Mat4));
}

static void
multiply_mat4 (Mat4 a, Mat4 b, Mat4 r)
{
  int i, j, k;
  Mat4 t;

  memset (t, 0.0, sizeof (Mat4));

  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      for (k = 0; k < 4; k++)
        t[i][j] += a[i][k]*b[k][j];

  memcpy (r, t, sizeof (Mat4));
}

static void
matrix_from_transform (GlrTransform *transform, Mat4 result)
{
  GlrTransform t = *transform;
  Mat4 mat;

  Mat4 translate_mat = {
    {                         1.0,                          0.0,                          0.0, 0.0},
    {                         0.0,                          1.0,                          0.0, 0.0},
    {                         0.0,                          0.0,                          1.0, 0.0},
    {t.origin[0] + t.translate[0], t.origin[1] + t.translate[1], t.origin[2] + t.translate[2], 1.0}
  };

  Mat4 origin_mat = {
    {         1.0,          0.0,          0.0, 0.0},
    {         0.0,          1.0,          0.0, 0.0},
    {         0.0,          0.0,          1.0, 0.0},
    {-t.origin[0], -t.origin[1], -t.origin[2], 1.0}
  };

  float a_x = t.rotate[0];
  float sin_a = sin (a_x);
  float cos_a = cos (a_x);
  Mat4 rot_x_mat = {
    {1.0,    0.0,   0.0, 0.0},
    {0.0,  cos_a, sin_a, 0.0},
    {0.0, -sin_a, cos_a, 0.0},
    {0.0,    0.0,   0.0, 1.0}
  };

  float a_y = t.rotate[1];
  sin_a = sin (a_y);
  cos_a = cos (a_y);
  Mat4 rot_y_mat = {
    { cos_a, 0.0, sin_a, 0.0},
    {   0.0, 1.0,   0.0, 0.0},
    {-sin_a, 0.0, cos_a, 0.0},
    {   0.0, 0.0,   0.0, 1.0}
  };

  float a_z = t.rotate[2];
  sin_a = sin (a_z);
  cos_a = cos (a_z);
  Mat4 rot_z_mat = {
    { cos_a, sin_a, 0.0, 0.0},
    {-sin_a, cos_a, 0.0, 0.0},
    {   0.0,   0.0, 1.0, 0.0},
    {   0.0,   0.0, 0.0, 1.0}
  };

  Mat4 scale_mat = {
    { t.scale[0],        0.0,        0.0, 0.0},
    {        0.0, t.scale[1],        0.0, 0.0},
    {        0.0,        0.0, t.scale[2], 0.0},
    {        0.0,        0.0,        0.0, 1.0}
  };

  multiply_mat4 (origin_mat, scale_mat, mat);
  multiply_mat4 (mat, rot_z_mat, mat);
  multiply_mat4 (mat, rot_y_mat, mat);
  multiply_mat4 (mat, rot_x_mat, mat);
  multiply_mat4 (mat, translate_mat, mat);

  copy_mat4 (mat, result);
}

static bool
print_shader_log (GLuint shader)
{
  GLint length;
  char buffer[1024] = {0};
  GLint success;

  glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &length);
  if (length == 0)
    return true;

  glGetShaderInfoLog (shader, 1024, NULL, buffer);
  if (strlen (buffer) > 0)
    printf ("Shader compilation log: %s\n", buffer);

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
  static const uint32_t MASK_8_BIT = 0x000000FF;

  glClearColor ( (self->clear_color >> 24              ) / 255.0,
                ((self->clear_color >> 16) & MASK_8_BIT) / 255.0,
                ((self->clear_color >>  8) & MASK_8_BIT) / 255.0,
                ( self->clear_color        & MASK_8_BIT) / 255.0);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void
initialize_frame_if_needed (GlrCanvas *self)
{
  uint32_t width, height;

  if (self->frame_initialized)
    return;

  self->frame_initialized = true;

  if (self->target != NULL)
    {
      glBindFramebuffer (GL_FRAMEBUFFER,
                         glr_target_get_framebuffer (self->target));

      glr_target_get_size (self->target, &width, &height);

      glViewport (0, 0, width, height);
    }
  else
    {
      GLint viewport[4];

      glGetIntegerv (GL_VIEWPORT, viewport);
      width = (uint32_t) viewport[2];
      height = (uint32_t) viewport[3];
    }

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable (GL_DEPTH_TEST);
  glDisable (GL_CULL_FACE);

  glUseProgram (self->shader_program);

  if (self->pending_clear)
    {
      clear_background (self);
      self->pending_clear = false;
    }

  /* @FIXME: get the glyph texture ids from texture cache,
     instead of hardcoding it here */
  int i;
  for (i = 0; i < 8; i++)
    {
      GLuint loc;
      char *st;

      st = g_strdup_printf ("glyph_cache[%d]", i);
      loc = glGetUniformLocation (self->shader_program, st);
      glUniform1i (loc, i);
      g_free (st);
    }

  // projection matrix
  Mat4 proj_matrix = {
    {2.0 / width,           0.0,                 0.0, 0.0},
    {        0.0, -2.0 / height,                 0.0, 0.0},
    {        0.0,           0.0, 2.0 / self->z_depth, 0.0},
    {       -1.0,           1.0,                 0.0, 1.0}
  };
  glUniformMatrix4fv (self->proj_matrix_loc,
                      1,
                      GL_FALSE,
                      &(proj_matrix[0][0]));

  // @FIXME:
  Mat4 transform_matrix;
  GlrTransform t;

  // @FIXME: do this only if perspective is enabled
  memcpy (&t, &self->transform, sizeof (GlrTransform));
  t.translate[2] += -(self->z_depth / 2.0);

  // canvas' global transform matrix
  matrix_from_transform (&t, transform_matrix);
  glUniformMatrix4fv (self->transform_matrix_loc,
                      1,
                      GL_FALSE,
                      &(transform_matrix[0][0]));
}

static void
encode_and_store_transform (GlrCanvas         *self,
                            float              left,
                            float              top,
                            GlrTransform      *transform,
                            GlrInstanceConfig  config)
{
  GlrTransform t;

  if (self->current_transform_index > 0)
    {
      config[1] = self->current_transform_index;
      return;
    }

  memcpy (&t, transform, sizeof (GlrTransform));
  t.origin[0] += left;
  t.origin[1] += top;

  Mat4 transform_matrix;
  size_t offset;

  // printf ("new transform encoded\n");

  matrix_from_transform (&t, transform_matrix);
  offset = glr_batch_add_dyn_attr (self->batch,
                                   &(transform_matrix[0][0]),
                                   sizeof (Mat4));
  config[1] = offset;
  self->current_transform_index = offset;
}

static bool
has_any_transform (const GlrTransform *transform)
{
  return
    UNEQUALS (transform->rotate[0], 0.0)
    || UNEQUALS (transform->rotate[1], 0.0)
    || UNEQUALS (transform->rotate[2], 0.0)

    || UNEQUALS (transform->scale[0], 1.0)
    || UNEQUALS (transform->scale[1], 1.0)
    || UNEQUALS (transform->scale[2], 1.0)

    || UNEQUALS (transform->translate[0], 0.0)
    || UNEQUALS (transform->translate[1], 0.0)
    || UNEQUALS (transform->translate[2], 0.0);
}

static void
instance_config_set_type (GlrInstanceConfig config, GlrInstanceType type)
{
  /* bits 26 to 29 (4 bits) of config0 encode the instance type */
  config[0] = (config[0] & INSTANCE_TYPE_MASK) | (type << 26);
}

static void
encode_and_store_border (GlrBatch          *batch,
                         GlrBorder         *border,
                         GlrInstanceConfig  config)
{
  size_t offset;
  float buf[16] = {0};
  uint8_t num_samples = 0;

  bool all_style_equal;
  bool all_width_equal;
  bool all_radius_equal;
  bool all_color_equal;

  all_style_equal = border->style[0] == border->style[1]
    && border->style[0] == border->style[2]
    && border->style[0] == border->style[3];

  all_width_equal = EQUALS (border->width[0], border->width[1])
    && EQUALS (border->width[0], border->width[2])
    && EQUALS (border->width[0], border->width[3]);

  all_radius_equal = EQUALS (border->radius[0], border->radius[1])
    && EQUALS (border->radius[0], border->radius[2])
    && EQUALS (border->radius[0], border->radius[3]);

  all_color_equal = border->color[0] == border->color[1]
    && border->color[0] == border->color[2]
    && border->color[0] == border->color[3];

  /* the simplest case: style, width, radius and color of all borders are equal */
  if (all_style_equal && all_width_equal && all_radius_equal && all_color_equal)
    {
      /* this case is encoded in 1 dyn attr sample */
      buf[0] = border->style[0];
      buf[1] = border->width[0];
      buf[2] = border->radius[0];
      buf[3] = border->color[0];

      num_samples = 1;
    }

  if (num_samples == 0)
    return;

  /* bits 20 to 23 (4 bits) of config0 encode the number of samples
     that describe the border */
  config[0] = (config[0] & BORDER_NUM_SAMPLES_MASK) | (num_samples << 20);

  offset = glr_batch_add_dyn_attr (batch,
                                   buf,
                                   sizeof (float) * 4 * num_samples);

  /* config2 encodes the offset of the border description */
  config[2] = offset;
}

static void
glr_color_decompose_float (GlrColor color, GlrColor4f color_4f)
{
  color_4f[0] = ((color >> 24) & 0xFF) / 255.0;
  color_4f[1] = ((color >> 16) & 0xFF) / 255.0;
  color_4f[2] = ((color >>  8) & 0xFF) / 255.0;
  color_4f[3] = (color         & 0xFF) / 255.0;
}

static void
encode_and_store_background (GlrBatch          *batch,
                             GlrBackground     *bg,
                             GlrInstanceConfig  config)
{
  goffset offset;
  float buf[16] = {0};
  uint8_t num_samples = 0;

  if (bg->type == GLR_BACKGROUND_LINEAR_GRADIENT)
    {
      // pre-calculate linear gradient values
      const float SEMI_DIAG_LENGTH = sqrt ((0.5*0.5)*2.0);
      float angle = bg->linear_grad_angle;

      if (angle < 0.0)
        angle = M_PI*2.0 - fmod (fabs (angle), M_PI * 2.0);
      else
        angle = fmod (angle, M_PI * 2.0);

      float gamma = M_PI/2.0;
      if (angle < M_PI/2.0)
        gamma -= angle;
      else if (angle < M_PI)
        gamma -= angle - M_PI/2.0;
      else if (angle < M_PI/2.0*3.0)
        gamma -= angle - M_PI;
      else
        gamma -= angle - M_PI/2.0*3.0;

      float beta = M_PI/4.0 + gamma;
      float length = SEMI_DIAG_LENGTH * sin (beta) * 2.0;

      buf[0] = angle;
      buf[1] = length;
      buf[2] = gamma;

      GlrColor4f color0, color1;
      glr_color_decompose_float (bg->linear_grad_colors[0], color0);
      glr_color_decompose_float (bg->linear_grad_colors[1], color1);

      memcpy (&(buf[4]), color0, sizeof (GlrColor4f));
      memcpy (&(buf[8]), color1, sizeof (GlrColor4f));

      num_samples = 3;
    }

  if (num_samples == 0)
    return;

  // bits 16 to 19 (4 bits) of config0 encode the number of samples to describe
  // the background
  config[0] = (config[0] & BACKGROUND_NUM_SAMPLES_MASK) | (num_samples << 16);

  offset = glr_batch_add_dyn_attr (batch,
                                   buf,
                                   sizeof (float) * 4 * num_samples);

  // config3 encodes the offset of the background description
  config[3] = offset;
}

static bool
has_any_border (const GlrBorder *border)
{
  return UNEQUALS (border->width[0], 0.0)
    || UNEQUALS (border->width[1], 0.0)
    || UNEQUALS (border->width[2], 0.0)
    || UNEQUALS (border->width[3], 0.0)

    || UNEQUALS (border->radius[0], 0.0)
    || UNEQUALS (border->radius[1], 0.0)
    || UNEQUALS (border->radius[2], 0.0)
    || UNEQUALS (border->radius[3], 0.0);
}

/* internal API */

/* public API */

GlrCanvas *
glr_canvas_new (GlrContext *context, GlrTarget *target)
{
  GlrCanvas *self;

  GLuint vertex_shader;
  GLuint fragment_shader;

  assert (context != NULL);

  self = g_slice_new0 (GlrCanvas);
  self->ref_count = 1;

  self->context = glr_context_ref (context);

  if (target != NULL)
    self->target = glr_target_ref (target);

  // setup the shaders
  vertex_shader = load_shader (INSTANCED_VERTEX_SHADER_SRC, GL_VERTEX_SHADER);
  fragment_shader = load_shader (INSTANCED_FRAGMENT_SHADER_SRC, GL_FRAGMENT_SHADER);

  self->shader_program = glCreateProgram ();
  glAttachShader (self->shader_program, vertex_shader);
  glAttachShader (self->shader_program, fragment_shader);

  glBindAttribLocation (self->shader_program, LAYOUT_ATTR, "lyt_attr");
  glBindAttribLocation (self->shader_program, COLOR_ATTR, "color_attr");
  glBindAttribLocation (self->shader_program, CONFIG_ATTR, "config_attr");

  glLinkProgram (self->shader_program);

  glDeleteShader (vertex_shader);
  glDeleteShader (fragment_shader);

  glUseProgram (self->shader_program);

  // get uniform locations
  self->proj_matrix_loc = glGetUniformLocation (self->shader_program,
                                                "proj_matrix");
  self->transform_matrix_loc = glGetUniformLocation (self->shader_program,
                                                     "transform_matrix");
  self->persp_matrix_loc = glGetUniformLocation (self->shader_program,
                                                 "persp_matrix");
  self->aa_offset_loc = glGetUniformLocation (self->shader_program,
                                              "aa_offset");

  // batch
  self->batch = glr_batch_new ();

  // edge-aa
  self->aa_offset = DEFAULT_EDGE_AA_OFFSET;
  glUniform1f (self->aa_offset_loc, self->aa_offset);

  // projection
  self->z_depth = DEFAULT_Z_DEPTH;

  // perspective matrix
  float fov = 90.0;
  float n = 0.0;
  float f = self->z_depth;
  float S = 1.0 / ( tan (fov * 0.5 * M_PI/180.0) );

  Mat4 persp_matrix = {
    {S,   0.0,          0.0,  0.0},
    {0.0,   S,          0.0,  0.0},
    {0.0, 0.0,   -(f/(f-n)), -1.0},
    {0.0, 0.0, -(f*n/(f-n)),  0.0}
  };

  glUniformMatrix4fv (self->persp_matrix_loc,
                      1,
                      GL_FALSE,
                      &(persp_matrix[0][0]));

  // transform matrix
  glr_canvas_reset_transform (self);
  self->current_transform_index = 0;

  // texture cache
  self->tex_cache = glr_context_get_texture_cache (self->context);
  glr_tex_cache_ref (self->tex_cache);

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
glr_canvas_clear (GlrCanvas *self, GlrColor color)
{
  assert (self != NULL);

  self->clear_color = color;

  if (self->frame_initialized)
    clear_background (self);
  else
    self->pending_clear = true;

  glr_batch_reset (self->batch);
  self->frame_initialized = false;
}

GlrTarget *
glr_canvas_get_target (GlrCanvas *self)
{
  return self->target;
}

void
glr_canvas_flush (GlrCanvas *self)
{
  assert (self != NULL);

  initialize_frame_if_needed (self);

  glr_batch_draw (self->batch, self->shader_program);

  self->frame_initialized = false;
}

void
glr_canvas_translate (GlrCanvas *self, float x, float y, float z)
{
  assert (self != NULL);

  self->transform.translate[0] = x;
  self->transform.translate[1] = y;
  self->transform.translate[2] = z;

  self->current_transform_index = 0;
}

void
glr_canvas_rotate (GlrCanvas *self, float angle_x, float angle_y, float angle_z)
{
  assert (self != NULL);

  self->transform.rotate[0] = angle_x;
  self->transform.rotate[1] = angle_y;
  self->transform.rotate[2] = angle_z;

  self->current_transform_index = 0;
}

void
glr_canvas_set_transform_origin (GlrCanvas *self,
                                 float      origin_x,
                                 float      origin_y,
                                 float      origin_z)
{
  assert (self != NULL);

  self->transform.origin[0] = origin_x;
  self->transform.origin[1] = origin_y;
  self->transform.origin[2] = origin_z;

  self->current_transform_index = 0;
}

void
glr_canvas_scale (GlrCanvas *self,
                  float      scale_x,
                  float      scale_y,
                  float      scale_z)
{
  assert (self != NULL);

  self->transform.scale[0] = scale_x;
  self->transform.scale[1] = scale_y;
  self->transform.scale[2] = scale_z;

  self->current_transform_index = 0;
}

void
glr_canvas_reset_transform (GlrCanvas *self)
{
  assert (self != NULL);

  memset (&self->transform, 0x00, sizeof (GlrTransform));

  self->transform.scale[0] = 1.0;
  self->transform.scale[1] = 1.0;
  self->transform.scale[2] = 1.0;

  self->current_transform_index = 0;
}

void
glr_canvas_draw_rect (GlrCanvas *self,
                      float      left,
                      float      top,
                      float      width,
                      float      height,
                      GlrStyle  *style)
{
  assert (self != NULL);
  assert (style != NULL);

  if (glr_batch_is_full (self->batch))
    {
      // @TODO: flush batch
      return;
    }

  GlrLayout lyt;
  GlrInstanceConfig config = {0};
  GlrBorder *br = &(style->border);
  GlrBackground *bg = &(style->background);
  GlrColor color;

  bool has_transform = has_any_transform (&self->transform);
  bool has_border = has_any_border (br);
  bool has_background = bg->type != GLR_BACKGROUND_NONE;

  // encode and submit border, which is common to all sub-instances
  if (has_border)
    encode_and_store_border (self->batch, br, config);

  lyt.left = left - self->aa_offset / 2.0;
  lyt.top = top - self->aa_offset / 2.0;

  if (has_transform && (has_border || has_background))
    encode_and_store_transform (self,
                                lyt.left, lyt.top,
                                &self->transform,
                                config);

  // background
  if (has_background)
    {
      instance_config_set_type (config, GLR_INSTANCE_RECT_BG);

      color = bg->color;
      encode_and_store_background (self->batch,
                                   bg,
                                   config);

      lyt.width = width + self->aa_offset;
      lyt.height = height + self->aa_offset;

      glr_batch_add_instance (self->batch, &lyt, color, config);
    }

  if (! has_border)
    return;

  // left border
  if (br->width[0] > 0.0)
    {
      instance_config_set_type (config, GLR_INSTANCE_BORDER_LEFT);

      lyt.width = br->width[0] + self->aa_offset;
      lyt.height = height
        - MAX (br->radius[0], br->width[1])
        - MAX (br->radius[3], br->width[3]);
      lyt.left = left - self->aa_offset / 2.0;
      lyt.top = top + MAX (br->radius[0], br->width[1]);

      glr_batch_add_instance (self->batch, &lyt, br->color[0], config);
    }

  // top border
  if (br->width[1] > 0.0)
    {
      instance_config_set_type (config, GLR_INSTANCE_BORDER_TOP);

      lyt.width = width
        - MAX (br->radius[0], br->width[0])
        - MAX (br->radius[1], br->width[2]);
      lyt.height = br->width[1] + self->aa_offset;
      lyt.left = left + MAX (br->radius[0], br->width[0]);
      lyt.top = top - self->aa_offset / 2.0;

      glr_batch_add_instance (self->batch, &lyt, br->color[1], config);
    }

  // right border
  if (br->width[2] > 0.0)
    {
      instance_config_set_type (config, GLR_INSTANCE_BORDER_RIGHT);

      lyt.width = br->width[2] + self->aa_offset;
      lyt.height = height
        - MAX (br->radius[1], br->width[1])
        - MAX (br->radius[2], br->width[3]);
      lyt.left = left - self->aa_offset / 2.0 + width - br->width[2];
      lyt.top = top + MAX (br->radius[1], br->width[1]);

      glr_batch_add_instance (self->batch, &lyt, br->color[2], config);
    }

  // bottom border
  if (br->width[3] > 0.0)
    {
      instance_config_set_type (config, GLR_INSTANCE_BORDER_BOTTOM);

      lyt.width = width
        - MAX (br->radius[3], br->width[0])
        - MAX (br->radius[2], br->width[2]);
      lyt.height = br->width[3] + self->aa_offset;
      lyt.left = left + MAX (br->radius[3], br->width[0]);
      lyt.top = top - self->aa_offset / 2.0 + height - br->width[3];

      glr_batch_add_instance (self->batch, &lyt, br->color[3], config);
    }

  // top-left corner
  instance_config_set_type (config, GLR_INSTANCE_BORDER_TOP_LEFT);

  lyt.width = MAX (br->radius[0], br->width[0]) + self->aa_offset / 2.0;
  lyt.height = MAX (br->radius[0], br->width[1]) + self->aa_offset / 2.0;
  lyt.left = left - self->aa_offset / 2.0;
  lyt.top = top - self->aa_offset / 2.0;

  glr_batch_add_instance (self->batch, &lyt, br->color[0], config);

  // top-right corner
  instance_config_set_type (config, GLR_INSTANCE_BORDER_TOP_RIGHT);

  lyt.width = MAX (br->radius[1], br->width[2]) + self->aa_offset / 2.0;
  lyt.height = MAX (br->radius[1], br->width[1]) + self->aa_offset / 2.0;
  lyt.left = left - self->aa_offset / 2.0
    + width - MAX (br->radius[1], br->width[2]) + self->aa_offset / 2.0;
  lyt.top = top - self->aa_offset / 2.0;

  glr_batch_add_instance (self->batch, &lyt, br->color[1], config);

  // bottom-right corner
  instance_config_set_type (config, GLR_INSTANCE_BORDER_BOTTOM_RIGHT);

  lyt.width = MAX (br->radius[2], br->width[2]) + self->aa_offset / 2.0;
  lyt.height = MAX (br->radius[2], br->width[3]) + self->aa_offset / 2.0;
  lyt.left = left - self->aa_offset / 2.0
    + width - MAX (br->radius[1], br->width[2]) + self->aa_offset / 2.0;
  lyt.top = top - self->aa_offset / 2.0
    + height - MAX (br->radius[2], br->width[3]) + self->aa_offset / 2.0;

  glr_batch_add_instance (self->batch, &lyt, br->color[2], config);

  // bottom-left corner
  instance_config_set_type (config, GLR_INSTANCE_BORDER_BOTTOM_LEFT);

  lyt.width = MAX (br->radius[3], br->width[0]) + self->aa_offset / 2.0;
  lyt.height = MAX (br->radius[3], br->width[3]) + self->aa_offset / 2.0;
  lyt.left = left - self->aa_offset / 2.0;
  lyt.top = top - self->aa_offset / 2.0
    + height - MAX (br->radius[3], br->width[3]) + self->aa_offset / 2.0;

  glr_batch_add_instance (self->batch, &lyt, br->color[3], config);
}

void
glr_canvas_draw_char (GlrCanvas *self,
                      uint32_t   code_point,
                      float      left,
                      float      top,
                      GlrFont   *font,
                      GlrColor   color)
{
  GlrLayout lyt;
  GlrInstanceConfig config = {0};
  const GlrTexSurface *surface;
  float tex_area[4] = {0};

  if (glr_batch_is_full (self->batch))
    {
      // @TODO: need flusing
      return;
    }

  // @FIXME: provide a default font in case none is specified
  surface = glr_tex_cache_lookup_font_glyph (self->tex_cache,
                                             font->face,
                                             font->face_index,
                                             font->size,
                                             code_point);
  if (surface == NULL)
    {
      // @FIXME: implement tex-cache flushing
      return;
    }

  lyt.left = left + surface->pixel_left;
  lyt.top = top - surface->pixel_top;
  lyt.width = surface->pixel_width;
  lyt.height = surface->pixel_height;

  instance_config_set_type (config, GLR_INSTANCE_CHAR_GLYPH);

  if (has_any_transform (&self->transform))
    encode_and_store_transform (self,
                                lyt.left,
                                lyt.top,
                                &self->transform,
                                config);

  tex_area[0] = surface->left;
  tex_area[1] = surface->top;
  tex_area[2] = surface->width;
  tex_area[3] = surface->height;

  // @FIXME: move this to a more general way of describing background
  size_t offset;
  offset = glr_batch_add_dyn_attr (self->batch, tex_area, sizeof (float) * 4);
  config[2] = offset;

  // bits 12 to 15 (4 bits) of config0 encode the texture unit to use,
  // either for glyphs, background-image or border-image
  config[0] |= surface->tex_id << 12;

  glr_batch_add_instance (self->batch, &lyt, color, config);
}

void
glr_canvas_draw_char_unicode (GlrCanvas *self,
                              uint32_t   unicode_char,
                              float      left,
                              float      top,
                              GlrFont   *font,
                              GlrColor   color)
{
  assert (self != NULL);
  assert (font != NULL);

  FT_Face face;
  face = glr_tex_cache_lookup_face (self->tex_cache,
                                    font->face,
                                    font->face_index);

  FT_UInt glyph_index;
  glyph_index = FT_Get_Char_Index (face, unicode_char);

  glr_canvas_draw_char (self, glyph_index, left, top, font, color);
}
