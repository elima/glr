#version 300 es

const uint MASK_8_BIT = uint (0x000000FF);

const uint DYN_ATTRS_TEX_WIDTH  = uint (1024);
const uint DYN_ATTRS_TEX_HEIGHT = uint (1024);
const float DYN_ATTRS_TEX_STEP_X = 1.0 / float (DYN_ATTRS_TEX_WIDTH);
const float DYN_ATTRS_TEX_STEP_Y = 1.0 / float (DYN_ATTRS_TEX_HEIGHT);

const uint INSTANCE_CHAR_GLYPH = uint (9);

const vec2 RECT_VERTICES[4] = vec2[4] (
  vec2 (0.0, 0.0),
  vec2 (1.0, 0.0),
  vec2 (1.0, 1.0),
  vec2 (0.0, 1.0)
);

layout (location = 0) in vec4 lyt;
layout (location = 1) in uvec4 config;

uniform uint      canvas_width;
uniform uint      canvas_height;
uniform sampler2D dyn_attrs_buffer;

out highp vec4 tex_coord;

flat out lowp  vec4 color;
flat out highp vec4 area_in_tex;
flat out       int  tex_id;

flat out      uint  instance_type;
flat out      ivec4 border_style;
flat out lowp vec2  br[4];
flat out lowp vec4  border_width;
flat out      uvec4 border_color;

flat out highp   float norm_width;
flat out highp   float norm_height;
flat out highp   float aspect_ratio;
flat out highp   float aa_factor_x;
flat out highp   float aa_factor_y;

flat out mediump float scale_x;
flat out mediump float scale_y;

float rotation_z = 0.0;

vec4
color_from_uint (uint color)
{
  return vec4 (float ( color >> 24)               / 255.0,
               float ((color >> 16) & MASK_8_BIT) / 255.0,
               float ((color >>  8) & MASK_8_BIT) / 255.0,
               float ( color        & MASK_8_BIT) / 255.0);
}

vec4
apply_layout (vec4 vertex, vec4 lyt)
{
  mat4 st = mat4 (
    lyt.z * 2.0,            0.0, 0.0, -1.0 + lyt.x * 2.0,
            0.0, -(lyt.w * 2.0), 0.0,  1.0 - lyt.y * 2.0,
            0.0,            0.0, 2.0,                0.0,
            0.0,            0.0, 0.0,                1.0
  );

  return vertex * st;
}

vec4
get_dyn_attrs_sample_float (uint offset)
{
  // 1 is offset 0, 2 is offset 1, and so on
  offset -= uint (1);

  float column = float (offset % DYN_ATTRS_TEX_WIDTH);
  float row = float (offset / DYN_ATTRS_TEX_HEIGHT);

  vec2 pos = vec2 (column * DYN_ATTRS_TEX_STEP_X,
                   row * DYN_ATTRS_TEX_STEP_Y);
  vec4 result = texture (dyn_attrs_buffer, pos);

  return result;
}

/*
uvec4
get_dyn_attrs_sample_uint (uint offset)
{
  // 1 is offset 0, 2 is offset 1, and so on
  offset -= uint (1);

  float column = float (offset % DYN_ATTRS_TEX_WIDTH);
  float row = float (offset / DYN_ATTRS_TEX_HEIGHT);

  vec2 pos = vec2 (column * DYN_ATTRS_TEX_STEP_X,
                   row * DYN_ATTRS_TEX_STEP_Y);
  uvec4 result = uvec4 (texture (dyn_attrs_buffer, pos));

  return result;
}
*/

vec4
load_and_apply_transform (uint num_samples, uint offset, vec4 pos, vec4 lyt)
{
  float aspect_ratio_inv = float (canvas_height) / float (canvas_width);

  // @FIXME: implement support for different num_samples, by now assume always 2
  vec4 transform[2] = vec4[2] (get_dyn_attrs_sample_float (offset),
                               get_dyn_attrs_sample_float (offset + uint (1)));

  float aspect_ratio = (lyt.w / lyt.z) * aspect_ratio_inv;

  float origin_x = transform[0][0];
  float origin_y = transform[0][1];
  scale_x = transform[1][0];
  scale_y = transform[1][1];
  rotation_z = -transform[0][2];
  float pre_rotation_z = -transform[0][3];

  mat4 translate, translate_inv;
  mat4 window_scale, window_scale_inv;
  mat4 object_scale, object_scale_inv;

  window_scale = mat4 (
    aspect_ratio, 0.0, 0.0, 0.0,
             0.0, 1.0, 0.0, 0.0,
             0.0, 0.0, 1.0, 0.0,
             0.0, 0.0, 0.0, 1.0
  );
  window_scale_inv = inverse (window_scale);

  object_scale = mat4 (
      scale_x,     0.0, 0.0, 0.0,
          0.0, scale_y, 0.0, 0.0,
          0.0,     0.0, 1.0, 0.0,
          0.0,     0.0, 0.0, 1.0
  );
  object_scale_inv = inverse (object_scale);

  translate = mat4 (
    1.0, 0.0, 0.0, -origin_x,
    0.0, 1.0, 0.0, -origin_y,
    0.0, 0.0, 1.0,       0.0,
    0.0, 0.0, 0.0,       1.0
  );
  translate_inv = inverse (translate);

  // apply pre-rotation (round-corner primitives)
  if (pre_rotation_z != 0.0)
    {
      mat4 pre_rotation = mat4 (
        cos (pre_rotation_z), -sin (pre_rotation_z),  0.0, 0.0,
        sin (pre_rotation_z),  cos (pre_rotation_z),  0.0, 0.0,
        0.0,                      0.0,  1.0, 0.0,
        0.0,                      0.0,  0.0, 1.0
      );

      pos = pos * pre_rotation;
    }

  // apply instance's transform
  pos *= translate;
  if (rotation_z != 0.0)
    {
      mat4 rotation = mat4 (
        cos (rotation_z), -sin (rotation_z),  0.0, 0.0,
        sin (rotation_z),  cos (rotation_z),  0.0, 0.0,
                     0.0,               0.0,  1.0, 0.0,
                     0.0,               0.0,  0.0, 1.0
      );

      pos = window_scale * rotation * object_scale * window_scale_inv * pos;
    }
  else
    {
      pos = window_scale * object_scale * window_scale_inv * pos;
    }
  pos *= translate_inv;

  return pos;
}

void
main (void)
{
  scale_x = 1.0;
  scale_y = 1.0;

  // load config, which is encoded in 4 uint32 words (config[0] to config[3])
  // ---------------------------------------------------------------------------

  // bits 30 to 31 (2 bits) of config0 are reserved, and must be zero

  // bits 26 to 29 (4 bits) of config0 encode the instance type
  instance_type = (config[0] >> 26) & uint (0x0F);

  // bits 24 to 25 (2 bits) of config0 encode the number of samples to describe
  // the geometry transformation
  uint transform_num_samples = (config[0] >> 24) & uint (0x03);

  // bits 20 to 23 (4 bits) of config0 encode the number of samples to describe
  // the border
  uint border_num_samples = (config[0] >> 20) & uint (0x0F);

  // bits 16 to 19 (4 bits) of config0 encode the number of samples to describe
  // the background
  uint background_num_samples = (config[0] >> 16) & uint (0x0F);


  // load instance's layout
  // ---------------------------------------------------------------------------
  vec4 l = vec4 (lyt.x / float (canvas_width),
                 lyt.y / float (canvas_height),
                 lyt.z / float (canvas_width),
                 lyt.w / float (canvas_height));

  vec2 vertex = RECT_VERTICES[gl_VertexID];
  vec4 pos = vec4 (vertex, 0.0, 1.0);

  tex_coord = vec4 (vertex, l.xy);
  aspect_ratio = lyt.z / lyt.w;

  // load and apply geometry transform
  // ---------------------------------------------------------------------------
  if (transform_num_samples > uint (0))
    {
      // the offset is encoded in config1 word,
      // 16 bits for x and 16 for y
      uint transform_offset = config[1];

      pos = load_and_apply_transform (transform_num_samples,
                                      transform_offset,
                                      pos,
                                      l);
    }

  // apply layout
  // ---------------------------------------------------------------------------
  gl_Position = apply_layout (pos, l);

  // load border
  // ---------------------------------------------------------------------------

  // no borders
  if (border_num_samples == uint (0))
    {
      border_width = vec4 (0.0);
      br = vec2[4] (vec2 (0.0), vec2 (0.0), vec2 (0.0), vec2 (0.0));
    }
  // simplest case: width, radius and color of all borders are equal
  if (border_num_samples == uint (1))
    {
      uint border_offset = config[2];
      vec4 border = get_dyn_attrs_sample_float (border_offset);

      border_style = ivec4 (int (border[0]),
                            int (border[0]),
                            int (border[0]),
                            int (border[0]));
      border_width = vec4 (border[1] * scale_x,
                           border[1] * scale_y,
                           border[1] * scale_x,
                           border[1] * scale_y);
      br[0] = br[1] = br[2] = br[3] = vec2 (border[2] * scale_x,
                                            border[2] * scale_y);
      border_color = uvec4 (uint (border[3]),
                            uint (border[3]),
                            uint (border[3]),
                            uint (border[3]));
    }
  else
    {
      /* @TODO: other cases not yet implemented */
    }

  // load background
  // ---------------------------------------------------------------------------

  // for simple backgrounds and font glyphs, color is encoded in config3
  color = color_from_uint (config[3]);

  // character glyph
  if (instance_type == uint (INSTANCE_CHAR_GLYPH))
    {
      uint tex_area_offset = config[2];
      area_in_tex = get_dyn_attrs_sample_float (tex_area_offset);

      // bits 12 to 15 (4 bits) of config0 encode the texture unit to use,
      // either for glyphs, background-image or border-image */
      tex_id = int (config[0] >> 12) & 0x0F;
    }
  else
    {
      /* @TODO: other cases not yet implemented */
    }

  // prepare distance-to-edge anti-aliasing
  // ---------------------------------------------------------------------------
  norm_width = 1.0 / (lyt.z * scale_x);
  norm_height = 1.0 / (lyt.w * scale_y);

  // @FIXME: load this from a uniform
  float edge_aa = 1.2;

  aa_factor_x = edge_aa * norm_width;
  aa_factor_y = edge_aa * norm_height;
}
