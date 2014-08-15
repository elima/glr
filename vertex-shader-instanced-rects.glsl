#version 300 es

precision highp float;

const uint MASK_8_BIT = uint (0x000000FF);

const float PI = 3.14159265359;

const vec2 RECT_VERTICES[4] = vec2[4] (
  vec2 (0.0, 0.0),
  vec2 (1.0, 0.0),
  vec2 (1.0, 1.0),
  vec2 (0.0, 1.0)
);

const uint INSTANCE_CHAR_GLYPH = uint (9);

const int BACKGROUND_TYPE_NONE         = 0;
const int BACKGROUND_TYPE_SOLID_COLOR  = 1;
const int BACKGROUND_TYPE_IMAGE        = 2;
const int BACKGROUND_TYPE_LINEAR_GRAD  = 3;
const int BACKGROUND_TYPE_RADIAL_GRAD  = 4;

layout (location = 0) in vec4  lyt_attr;
layout (location = 1) in vec4  color_attr;
layout (location = 2) in uvec4 config_attr;

     out vec4  color;
     out vec2  tex_coords;
     out float edge;
flat out vec2  aa_size;
flat out float aspect_ratio;
flat out vec2  norm;

flat out uint  instance_type;

flat out vec4  area_in_tex;
flat out int   tex_id;

flat out ivec4 border_style;
flat out vec4  border_width;
flat out vec2  border_radius[4];
flat out uvec4 border_color;

flat out int   background_type;

flat out vec4  linear_grad_colors[2];
flat out float linear_grad_steps[4];
flat out float linear_grad_angle;
flat out float linear_grad_gamma;
flat out float linear_grad_length;

uniform mat4  proj_matrix;
uniform mat4  transform_matrix;
uniform mat4  persp_matrix;
uniform float aa_offset;

const uint DYN_ATTRS_TEX_WIDTH  = uint (1024);
const uint DYN_ATTRS_TEX_HEIGHT = uint (1024);
const float DYN_ATTRS_TEX_STEP_X = 1.0 / float (DYN_ATTRS_TEX_WIDTH);
const float DYN_ATTRS_TEX_STEP_Y = 1.0 / float (DYN_ATTRS_TEX_HEIGHT);
uniform sampler2D dyn_attrs_tex;

vec4
get_dyn_attrs_sample (uint offset)
{
  // 1 is offset 0, 2 is offset 1, and so on
  offset = offset - uint (1);

  uint column = offset % DYN_ATTRS_TEX_WIDTH;
  uint row = offset / DYN_ATTRS_TEX_HEIGHT;

  vec2 pos = vec2 (float (column) * DYN_ATTRS_TEX_STEP_X,
                   float (row) * DYN_ATTRS_TEX_STEP_Y);
  vec4 result = texture (dyn_attrs_tex, pos);

  return result;
}

vec4
color_from_uint (uint color)
{
  return vec4 (float ( color >> 24)               / 255.0,
               float ((color >> 16) & MASK_8_BIT) / 255.0,
               float ((color >>  8) & MASK_8_BIT) / 255.0,
               float ( color        & MASK_8_BIT) / 255.0);
}

void
main ()
{
  vec4 pos;
  tex_coords = RECT_VERTICES[gl_VertexID];
  float scale_x = 1.0;
  float scale_y = 1.0;

  // load config, which is encoded in 4 uint32 words (config[0] to config[3])
  // ---------------------------------------------------------------------------

  // bits 30 to 31 (2 bits) of config0 are reserved, and must be zero

  // bits 26 to 29 (4 bits) of config0 encode the instance type
  instance_type = (config_attr[0] >> 26) & uint (0x0F);

  // bits 20 to 23 (4 bits) of config0 encode the number of samples to describe
  // the border
  uint border_num_samples = (config_attr[0] >> 20) & uint (0x0F);

  // bits 16 to 19 (4 bits) of config0 encode the number of samples to describe
  // the background
  uint background_num_samples = (config_attr[0] >> 16) & uint (0x0F);

  // load instance's layout
  // ---------------------------------------------------------------------------
  pos = vec4 (lyt_attr.x + lyt_attr.z * tex_coords.x,
              lyt_attr.y + lyt_attr.w * tex_coords.y,
              0.0, 1.0);

  // load and apply linear transformation, if any
  // ---------------------------------------------------------------------------
  // the transformation matrix is a 4-sample dynamic attribute
  // whose offset is encoded in config1.
  if (config_attr[1] > uint (0)) {
    mat4 transform_matrix = mat4 (
      get_dyn_attrs_sample (config_attr[1]),
      get_dyn_attrs_sample (config_attr[1] + uint (1)),
      get_dyn_attrs_sample (config_attr[1] + uint (2)),
      get_dyn_attrs_sample (config_attr[1] + uint (3))
    );
    pos = transform_matrix * pos;
  }

  // apply projection and perspective transformations
  // ---------------------------------------------------------------------------
  pos = persp_matrix * proj_matrix * transform_matrix * pos;

  // @FIXME: the Z-coordinate must be inverted to the positive plane to use the
  // depth test
  // pos.z = -pos.z; // @FIXME: this gives some artifacts in Arndate

  gl_Position = pos;

  // load color
  // ---------------------------------------------------------------------------
  color = vec4 (color_attr.a, color_attr.b, color_attr.g, color_attr.r);
  color = mix (vec4 (color.rgb, 0.0), color, 2.00 - gl_Position.z);

  // character glyph
  if (instance_type == uint (INSTANCE_CHAR_GLYPH)) {
    uint tex_area_offset = config_attr[2];
    area_in_tex = vec4 (get_dyn_attrs_sample (tex_area_offset));

    // bits 12 to 15 (4 bits) of config0 encode the texture unit to use,
    // either for glyphs, background-image or border-image
    tex_id = int (config_attr[0] >> 12) & 0x0F;
  }

  else {
    // load border
    // ---------------------------------------------------------------------------

    // no borders
    if (border_num_samples == uint (0)) {
      border_width = vec4 (0.0);
      border_radius = vec2[4] (vec2 (0.0), vec2 (0.0), vec2 (0.0), vec2 (0.0));
    }
    // simplest case: width, radius and color of all borders are equal
    else if (border_num_samples == uint (1)) {
      uint border_offset = config_attr[2];
      vec4 border = get_dyn_attrs_sample (border_offset);

      border_style = ivec4 (int (border[0]),
                            int (border[0]),
                            int (border[0]),
                            int (border[0]));
      border_width = vec4 (border[1] * scale_x,
                           border[1] * scale_y,
                           border[1] * scale_x,
                           border[1] * scale_y);
      border_radius[0] =
        border_radius[1] =
        border_radius[2] =
        border_radius[3] = vec2 (border[2] * scale_x, border[2] * scale_y);

      border_color = uvec4 (uint (border[3]),
                            uint (border[3]),
                            uint (border[3]),
                            uint (border[3]));
    }
    else {
      // @TODO: other cases not yet implemented
    }

    // load background
    // ---------------------------------------------------------------------------
    if (background_num_samples == uint (3))
      {
        // linear gradient
        uint bg_offset = config_attr[3];
        vec4 bg = get_dyn_attrs_sample (bg_offset);

        background_type = BACKGROUND_TYPE_LINEAR_GRAD;
        linear_grad_angle = bg[0];
        linear_grad_length = bg[1];
        linear_grad_gamma = bg[2];

        linear_grad_colors = vec4[2] (get_dyn_attrs_sample (bg_offset + uint(1)),
                                      get_dyn_attrs_sample (bg_offset + uint(2)));
      }
    else
      {
        // @TODO: other cases not yet implemented, assume solid color
        background_type = BACKGROUND_TYPE_SOLID_COLOR;
      }

    // @FIXME: we need actual values for scale_x and scale_y
    norm.x = 1.0 / abs (lyt_attr.z * scale_x);
    norm.y = 1.0 / abs (lyt_attr.w * scale_y);
    aa_size = vec2 (aa_offset * norm.x, aa_offset * norm.y);
  }
}
