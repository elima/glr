#version 300 es

const uint TRANSFORM_TEX_WIDTH  = uint (1024);
const uint TRANSFORM_TEX_HEIGHT = uint ( 512);

const mediump float GLYPH_TEX_WIDTH  = 1024.0;
const mediump float GLYPH_TEX_HEIGHT = 4096.0;

const uint MASK_8_BIT = uint (0x000000FF);

const int BACKGROUND_COLOR     = 0;
const int BACKGROUND_GLYPH_TEX = 1;
const int BACKGROUND_IMAGE_TEX = 2;

uniform uint canvas_width;
uniform uint canvas_height;

uniform sampler2D transform_buffer;

layout (location = 0) in vec4 vertex;
layout (location = 1) in vec4 lyt;
layout (location = 2) in uint col;
layout (location = 3) in uvec2 config;
layout (location = 4) in uint tex_area;

out vec4 color;
flat out highp vec4 area_in_tex;
flat out int tex_id;
out vec4 tex_coord;
flat out int background_type;

vec4
apply_layout (vec4 vertice, vec4 lyt)
{
  mat4 st = mat4 (
    lyt.z * 2.0,            0.0, 0.0, -1.0 + lyt.x * 2.0,
            0.0, -(lyt.w * 2.0), 0.0,  1.0 - lyt.y * 2.0,
            0.0,            0.0, 2.0,                0.0,
            0.0,            0.0, 0.0,                1.0
  );

  return vertice * st;
}

void main()
{
  float aspect_ratio_inv = float (canvas_height) / float (canvas_width);
  float step;

  // load layout
  vec4 l = vec4 (lyt.x / float (canvas_width),
                 lyt.y / float (canvas_height),
                 lyt.z / float (canvas_width),
                 lyt.w / float (canvas_height));

  vec4 pos = vec4 (vertex.xy, 0.0, 1.0);

  uint transform_index = config.y;

  if (transform_index > uint (0))
    {
      float step_x = 1.0 / float (TRANSFORM_TEX_WIDTH);
      float step_y = 1.0 / float (TRANSFORM_TEX_HEIGHT);

      uint offset = transform_index - uint (1);
      float column = float (offset % TRANSFORM_TEX_WIDTH);
      float row = float (offset / TRANSFORM_TEX_WIDTH);

      vec2 transform1_pos = vec2 (column * step_x, row * step_y);
      vec2 transform2_pos = vec2 ((column + 1.0) * step_x, row * step_y);
      vec4 transform1 = texture (transform_buffer, transform1_pos);
      vec4 transform2 = texture (transform_buffer, transform2_pos);

      float aspect_ratio = (l.w / l.z) * aspect_ratio_inv;

      float origin_x = transform1[0];
      float origin_y = transform1[1];
      float scale_x = transform1[2];
      float scale_y = transform1[3];
      float rotation_z = -transform2[0];
      float parent_rotation_z = -transform2[1];
      float parent_origin_x = transform2[2];
      float parent_origin_y = transform2[3];

      mat4 translate, translate_inv;
      mat4 parent_translate, parent_translate_inv;
      mat4 window_scale, window_scale_inv;
      mat4 object_scale, object_scale_inv;
      mat4 rotation;

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

      if (rotation_z != 0.0)
        {
          rotation = mat4 (
            cos (rotation_z), -sin (rotation_z),  0.0, 0.0,
            sin (rotation_z),  cos (rotation_z),  0.0, 0.0,
                         0.0,               0.0,  1.0, 0.0,
                         0.0,               0.0,  0.0, 1.0
          );
        }

      // apply instance's transform
      pos = pos * translate;
      if (rotation_z != 0.0)
        pos = window_scale * rotation * object_scale * window_scale_inv * pos;
      else
        pos = window_scale * object_scale * window_scale_inv * pos;
      pos = pos * translate_inv;


      // apply parent's transform

      parent_translate = mat4 (
        1.0, 0.0, 0.0, -parent_origin_x,
        0.0, 1.0, 0.0, -parent_origin_y,
        0.0, 0.0, 1.0,              0.0,
        0.0, 0.0, 0.0,              1.0
      );
      parent_translate_inv = inverse (parent_translate);

      pos = pos * object_scale_inv * parent_translate;
      if (parent_rotation_z != 0.0)
        {
          rotation = mat4 (
            cos (parent_rotation_z), -sin (parent_rotation_z),  0.0, 0.0,
            sin (parent_rotation_z),  cos (parent_rotation_z),  0.0, 0.0,
                                0.0,                      0.0,  1.0, 0.0,
                                0.0,                      0.0,  0.0, 1.0
          );
          pos = rotation * pos;
        }
      pos = pos * object_scale * parent_translate_inv;
    }

  // apply layout
  gl_Position = apply_layout (pos, l);

  // resolve background
  uint background_config = config.x;

  // 3 most significant bits describe the type of background
  background_type = int (background_config >> 29);

  color = vec4 (float ( col >> 24)               / 255.0,
                float ((col >> 16) & MASK_8_BIT) / 255.0,
                float ((col >>  8) & MASK_8_BIT) / 255.0,
                float ( col        & MASK_8_BIT) / 255.0);

  if (background_type == BACKGROUND_GLYPH_TEX || background_type == BACKGROUND_IMAGE_TEX)
    {
      tex_coord = vec4 (vertex.xy, 0.0, 1.0);

      // x and y offset in texture is encoded in 16 bits each
      uint tex_area_left = (tex_area >> 16) & uint (0xFFFF);
      uint tex_area_top = tex_area & uint (0xFFFF);

      // bits 25 to 28 in config encode the texture id
      tex_id = int (config >> 25) & 0x0F;

      area_in_tex = vec4 (float (tex_area_left) / GLYPH_TEX_WIDTH,
                          float (tex_area_top) / GLYPH_TEX_HEIGHT,
                          float (lyt.z) * 3.0 / GLYPH_TEX_WIDTH,
                          float (lyt.w) / GLYPH_TEX_HEIGHT);
    }
}
