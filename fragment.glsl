#version 300 es

const int BACKGROUND_COLOR   = 0;
const int BACKGROUND_TEXTURE = 1;

uniform sampler2D atlas;
// uniform usampler2D atlas;

in mediump vec4 color;
flat in uvec4 area_in_atlas;
in mediump vec4 tex_coord;
flat in int background_type;
out mediump vec4 my_FragColor;

void main()
{
  if (background_type == BACKGROUND_TEXTURE)
    {
      int width = int (area_in_atlas.z);
      int height = int (area_in_atlas.w);

      int x, y;
      x = int (area_in_atlas.x) * 4 + int (int (tex_coord.s) * width * 3);
      y = int (area_in_atlas.y) + int (int (tex_coord.t) * height);

      mediump float f = 0.0;

      // uvec4 a = texelFetch (atlas, ivec2 (x / 4, y), 0);
      mediump vec4 a = texture2D (atlas, vec2 (float (x) / 4.0 / 1024.0, float (y) / 4096.0));

      // f = float (a.r) / 255.0;
      f = a.r;

      my_FragColor = vec4 (color.rgb, color.a * f);
    }
  else
    {
      my_FragColor = color;
    }
}
