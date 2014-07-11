#version 300 es

const int BACKGROUND_COLOR     = 0;
const int BACKGROUND_GLYPH_TEX = 1;
const int BACKGROUND_IMAGE_TEX = 2;

uniform sampler2D glyph_cache[8];

in mediump vec4 color;
flat in mediump vec4 area_in_tex;
flat in int tex_id;
in mediump vec4 tex_coord;
flat in int background_type;
out mediump vec4 my_FragColor;

mediump vec4
sample_glyph_cache (int tex_id, mediump vec2 tex_coord)
{
  switch (tex_id)
    {
    case 0: return texture2D (glyph_cache[0], tex_coord);
    case 1: return texture2D (glyph_cache[1], tex_coord);
    case 2: return texture2D (glyph_cache[2], tex_coord);
    case 3: return texture2D (glyph_cache[3], tex_coord);
    case 4: return texture2D (glyph_cache[4], tex_coord);
    case 5: return texture2D (glyph_cache[5], tex_coord);
    case 6: return texture2D (glyph_cache[6], tex_coord);
    case 7: return texture2D (glyph_cache[7], tex_coord);
    }
}

void main()
{
  mediump vec4 col = color;

  if (background_type == BACKGROUND_GLYPH_TEX)
    {
      // font glyph
      mediump float f = 0.0;
      mediump vec4 a;

      a = sample_glyph_cache (tex_id, vec2 (area_in_tex.x + tex_coord.s * area_in_tex.z,
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
