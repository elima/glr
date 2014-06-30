#version 130

const int BACKGROUND_COLOR   = 0;
const int BACKGROUND_TEXTURE = 1;

uniform usampler2D atlas;

in vec4 color;
flat in uvec4 area_in_atlas;
in vec4 tex_coord;
flat in int background_type;

void main()
{
  if (background_type == BACKGROUND_TEXTURE)
    {
      int width = int (area_in_atlas.z);
      int height = int (area_in_atlas.w);

      int x, y;
      x = int (area_in_atlas.x) * 4 + int (tex_coord.s * width * 3);
      y = int (area_in_atlas.y) + int (tex_coord.t * height);

      uint byte;
      float f = 0.0;

      uvec4 a = texelFetch (atlas, ivec2 (x / 4, y), 0);
      byte = uint (a.r/* & uint (0xFF)*/);
      f = a.r / 255.0;

      gl_FragColor = vec4 (color.rgb, color.a * f);
    }
  else
    {
      gl_FragColor = color;
    }
}
