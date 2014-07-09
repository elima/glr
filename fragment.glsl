#version 300 es

const int BACKGROUND_COLOR     = 0;
const int BACKGROUND_GLYPH_TEX = 1;
const int BACKGROUND_IMAGE_TEX = 2;

uniform sampler2D glyph_cache;

in mediump vec4 color;
flat in highp vec4 area_in_tex;
in mediump vec4 tex_coord;
flat in int background_type;
out mediump vec4 my_FragColor;

void main()
{
  mediump vec4 col = color;

  if (background_type == BACKGROUND_GLYPH_TEX)
    {
      // font glyph
      mediump float f = 0.0;
      mediump vec4 a = texture2D (glyph_cache,
                                  vec2 (area_in_tex.x + tex_coord.s * area_in_tex.z,
                                        area_in_tex.y + tex_coord.t * area_in_tex.w));
      f = a.r;
      if (f == 0.0)
        discard;

      my_FragColor = vec4 (col.rgb, col.a * f);
    }
  else if (background_type == BACKGROUND_COLOR)
    {
      // flat color
      my_FragColor = color;
    }
  else
    {
      // image texture
      // @TODO: not yet implemented
    }
}
