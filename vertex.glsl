#version 130
#extension GL_ARB_draw_instanced : enable

// const float PI = 3.1415926535897932384626433832795;
const int TRANSFORM_TEX_WIDTH  = int (1024);
const int TRANSFORM_TEX_HEIGHT = int (512);

const uint MASK_8_BIT = uint (0x000000FF);

const int BACKGROUND_COLOR   = 0;
const int BACKGROUND_TEXTURE = 1;

uniform uint width;
uniform uint height;

uniform sampler2D transform_buffer;

attribute vec4 vertex;
attribute uint col;
attribute vec4 lyt;
attribute uvec2 config;
attribute uint tex_area;

out vec4 color;
flat out uvec4 area_in_atlas;
out vec4 tex_coord;
flat out int background_type;

void main()
{
  float aspect_ratio_inv = float (height) / float (width);
  float step;

  // load layout
  vec4 l = vec4 (lyt.x / width, lyt.y / height, lyt.z / width, lyt.w / height);
  mat4 st = mat4 (
    l.z * 2.0,          0.0, 0.0, -1.0 + l.x * 2.0,
          0.0, -(l.w * 2.0), 0.0,  1.0 - l.y * 2.0,
          0.0,          0.0, 2.0,              0.0,
          0.0,          0.0, 0.0,              1.0
  );

  vec4 pos = vec4 (vertex.xy, 0.0, 1.0);

  uint transform_index = config.y;

  if (transform_index > uint (0))
    {
      float step_x = 1.0 / (TRANSFORM_TEX_WIDTH);
      float step_y = 1.0 / (TRANSFORM_TEX_HEIGHT);

      int offset = int (transform_index) - 1;
      int column = offset % TRANSFORM_TEX_WIDTH;
      int row = offset / TRANSFORM_TEX_WIDTH;

      vec2 transform1_pos = vec2 (column * step_x, row * step_y);
      vec2 transform2_pos = vec2 ((column + 1) * step_x, row * step_y);
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
      // window_scale_inv = inverse (window_scale);
      window_scale_inv = mat4 (
        1.0 / aspect_ratio, 0.0, 0.0, 0.0,
                       0.0, 1.0, 0.0, 0.0,
                       0.0, 0.0, 1.0, 0.0,
                       0.0, 0.0, 0.0, 1.0
      );

      object_scale = mat4 (
          scale_x,     0.0, 0.0, 0.0,
              0.0, scale_y, 0.0, 0.0,
              0.0,     0.0, 1.0, 0.0,
              0.0,     0.0, 0.0, 1.0
      );
      // object_scale_inv = inverse (object_scale);
      object_scale_inv = mat4 (
          1.0/scale_x,         0.0, 0.0, 0.0,
                  0.0, 1.0/scale_y, 0.0, 0.0,
                  0.0,         0.0, 1.0, 0.0,
                  0.0,         0.0, 0.0, 1.0
      );

      translate = mat4 (
        1.0, 0.0, 0.0, -origin_x,
        0.0, 1.0, 0.0, -origin_y,
        0.0, 0.0, 1.0,       0.0,
        0.0, 0.0, 0.0,       1.0
      );
      // translate_inv = inverse (translate);
      translate_inv = mat4 (
        1.0, 0.0, 0.0, origin_x,
        0.0, 1.0, 0.0, origin_y,
        0.0, 0.0, 1.0,      0.0,
        0.0, 0.0, 0.0,      1.0
      );

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
      // parent_translate_inv = inverse (parent_translate);
      parent_translate_inv = mat4 (
        1.0, 0.0, 0.0, parent_origin_x,
        0.0, 1.0, 0.0, parent_origin_y,
        0.0, 0.0, 1.0,             0.0,
        0.0, 0.0, 0.0,             1.0
      );

      pos = pos * object_scale_inv * parent_translate;

      if (parent_rotation_z != 0)
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
  pos *= st;

  gl_Position = pos;

  // resolve background
  uint background_config = config.x;

  // 3 most significant bits describe the type of background
  background_type = int (background_config >> 29);

  color = vec4 ( (col >> 24)               / 255.0,
                ((col >> 16) & MASK_8_BIT) / 255.0,
                ((col >>  8) & MASK_8_BIT) / 255.0,
                ( col        & MASK_8_BIT) / 255.0);

  if (background_type == BACKGROUND_TEXTURE)
    {
      tex_coord = vec4 (vertex.xy, 0.0, 1.0);

      uint tex_area_left = (tex_area >> 16) & uint (0xFFFF);
      uint tex_area_top = tex_area & uint (0xFFFF);
      area_in_atlas = uvec4 (tex_area_left, tex_area_top, lyt.z, lyt.w);
    }
}
