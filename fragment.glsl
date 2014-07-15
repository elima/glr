#version 300 es

const highp float PI         = 3.14159265359;
const uint        MASK_8_BIT = uint (0x000000FF);

const uint INSTANCE_RECT_BG             = uint (0);
const uint INSTANCE_BORDER_TOP          = uint (1);
const uint INSTANCE_BORDER_RIGHT        = uint (2);
const uint INSTANCE_BORDER_BOTTOM       = uint (3);
const uint INSTANCE_BORDER_LEFT         = uint (4);
const uint INSTANCE_BORDER_TOP_LEFT     = uint (5);
const uint INSTANCE_BORDER_TOP_RIGHT    = uint (6);
const uint INSTANCE_BORDER_BOTTOM_LEFT  = uint (7);
const uint INSTANCE_BORDER_BOTTOM_RIGHT = uint (8);
const uint INSTANCE_CHAR_GLYPH          = uint (9);

const int BACKGROUND_COLOR     = 0;
const int BACKGROUND_GLYPH_TEX = 1;
const int BACKGROUND_IMAGE_TEX = 2;

uniform sampler2D glyph_cache[8];

out mediump vec4 my_FragColor;

in highp vec4 tex_coord;

flat in lowp  vec4 color;
flat in highp vec4 area_in_tex;
flat in       int  tex_id;

flat in      uint  instance_type;
flat in      ivec4 border_style;
flat in lowp vec2  br[4];
flat in lowp vec4  border_width;
flat in      uvec4 border_color;

flat in highp   float norm_width;
flat in highp   float norm_height;
flat in highp   float aspect_ratio;
flat in highp   float aa_factor_x;
flat in highp   float aa_factor_y;

flat in mediump float scale_x;
flat in mediump float scale_y;

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

bool
draw_round_corner (mediump vec4 col,
                   mediump float  x, mediump float  y,
                   mediump float rx, mediump float ry)
{
  if (x >= rx || y >= ry || x < 0.0 || y < 0.0)
    return false;

  mediump float ar = aspect_ratio;
  mediump float o = (atan (y, x*ar)) + PI/2.0;
  mediump float x1 = cos (o) * rx * ar;
  mediump float y1 = sin (o) * ry;
  mediump float h1 = length (vec2 (x1, y1));

  mediump float kx = cos (o) * aa_factor_x * ar;
  mediump float ky = sin (o) * aa_factor_y;
  mediump float k = length (vec2 (kx, ky));

  mediump float h = length (vec2 ((rx - x)*ar, ry - y));

  if (h > h1)
    discard;
  else if (h > h1 - k)
    col.a *= (1.0/k) * (h1 - h);

  my_FragColor = col;

  return true;
}

void
main (void)
{
  mediump vec4 col = color;

  mediump float x = tex_coord.s;
  mediump float y = tex_coord.t;

  mediump float kx = aa_factor_x;
  mediump float ky = aa_factor_y;

  // character glyph
  // ---------------------------------------------------------------------------
  if (instance_type == INSTANCE_CHAR_GLYPH)
    {
      mediump float f = 0.0;
      mediump vec4 a;

      a = sample_glyph_cache (tex_id, vec2 (area_in_tex.x + tex_coord.s * area_in_tex.z,
                                            area_in_tex.y + tex_coord.t * area_in_tex.w));

      f = a.r;
      if (f == 0.0)
        discard;

      col.a *= f;
    }

  // (rounded) rectangle background
  // ---------------------------------------------------------------------------
  else if (instance_type == INSTANCE_RECT_BG)
    {
      mediump float rx, ry;

      // top-left corner
      if (br[0].x > 0.0 && br[0].y > 0.0)
        {
          rx = br[0].x * norm_width;
          ry = br[0].y * norm_height;
          if (draw_round_corner (col, x, y, rx, ry))
            return;
        }

      // top-right corner
      if (br[1].x > 0.0 && br[1].y > 0.0)
        {
          rx = br[1].x * norm_width;
          ry = br[1].y * norm_height;
          if (draw_round_corner (col, 1.0 - x, y, rx, ry))
            return;
        }

      // bottom-left corner
      if (br[2].x > 0.0 && br[2].y > 0.0)
        {
          rx = br[1].x * norm_width;
          ry = br[1].y * norm_height;
          if (draw_round_corner (col, x, 1.0 - y, rx, ry))
            return;
        }

      // bottom-right corner
      if (br[3].x > 0.0 && br[3].y > 0.0)
        {
          rx = br[1].x * norm_width;
          ry = br[1].y * norm_height;
          if (draw_round_corner (col, 1.0 - x, 1.0 - y, rx, ry))
            return;
        }

      // vertical edge anti-aliasing
      if (aa_factor_x > 0.0)
        {
          if (x < kx)
            col.a *= (1.0/kx) * x;
          else if (x >= 1.0 - kx)
            col.a *= (1.0/kx) * (1.0 - x);
        }

      // horizontal edge anti-aliasing
      if (aa_factor_y > 0.0)
        {
          if (y < ky)
            col.a *= (1.0/ky) * y;
          else if (y >= 1.0 - ky)
            col.a *= (1.0/ky) * (1.0 - y);
        }
    }

  // borders
  // @TODO: only solid borders are currently implemented
  // ---------------------------------------------------------------------------
  // left border
  else if (instance_type == INSTANCE_BORDER_LEFT)
    {
      // vertical edge anti-aliasing
      if (aa_factor_x > 0.0)
        {
          mediump float kx = aa_factor_x;
          if (x < kx)
            col.a *= (1.0/kx) * x;
          else if (x >= 1.0 - kx
                   && (y >= border_width[1] * norm_height + aa_factor_y/2.0 || br[0].y > 0.0))
            col.a *= (1.0/kx) * (1.0 - x);
        }

      // horizontal edge anti-aliasing
      if (aa_factor_y > 0.0 && br[0].x == 0.0)
        {
          mediump float ky = aa_factor_y;
          if (y < ky)
            col.a *= (1.0/ky) * y;
        }
    }

  // top border
  else if (instance_type == INSTANCE_BORDER_TOP)
    {
      // vertical edge anti-aliasing
      if (aa_factor_x > 0.0 && br[1].x == 0.0)
        {
          if (x >= 1.0 - kx)
            col.a *= (1.0/kx) * (1.0 - x);
        }

      // horizontal edge anti-aliasing
      if (aa_factor_y > 0.0)
        {
          if (y < ky)
            col.a *= (1.0/ky) * y;
          else if (y >= 1.0 - ky
                   && (1.0 - x >= border_width[2] * norm_width + aa_factor_x/2.0 || br[1].x > 0.0))
            col.a *= (1.0/ky) * (1.0 - y);
        }
    }

  // right border
  else if (instance_type == INSTANCE_BORDER_RIGHT)
    {
      // vertical edge anti-aliasing
      if (aa_factor_x > 0.0)
        {
          if (x < kx
              && (1.0 - y > border_width[1] * norm_height + aa_factor_y/2.0 || br[3].y > 0.0))
            col.a *= (1.0/kx) * x;
          else if (x >= 1.0 - kx)
            col.a *= (1.0/kx) * (1.0 - x);
        }

      // horizontal edge anti-aliasing
      if (aa_factor_y > 0.0 && br[3].y == 0.0)
        {
          if (y >= 1.0 - ky)
            col.a *= (1.0/ky) * (1.0 - y);
        }
    }

  // bottom border
  else if (instance_type == INSTANCE_BORDER_BOTTOM)
    {
      // vertical edge anti-aliasing
      if (aa_factor_x > 0.0 && br[2].x == 0.0)
        {
          if (x < kx)
            col.a *= (1.0/kx) * x;
        }

      // horizontal edge anti-aliasing
      if (aa_factor_y > 0.0)
        {
          if (y < ky
              && (x > border_width[0] * norm_width + aa_factor_x/2.0 || br[2].x > 0.0))
            col.a *= (1.0/ky) * y;
          else if (y >= 1.0 - ky)
            col.a *= (1.0/ky) * (1.0 - y);
        }
    }

  // round corner borders
  else if (instance_type == INSTANCE_BORDER_TOP_LEFT
           || instance_type == INSTANCE_BORDER_TOP_RIGHT
           || instance_type == INSTANCE_BORDER_BOTTOM_LEFT
           || instance_type == INSTANCE_BORDER_BOTTOM_RIGHT)
    {
      highp float radius;
      highp float a;
      highp float b;
      highp float h1;
      highp float x1, y1;
      highp float o;
      highp float h;
      highp float k;

      if (instance_type == INSTANCE_BORDER_TOP_LEFT)
        {
          a = border_width[0] / br[0].x;
          b = border_width[1] / br[0].y;
        }
      else if (instance_type == INSTANCE_BORDER_TOP_RIGHT)
        {
          a = border_width[1] / br[1].x;
          b = border_width[2] / br[1].y;
        }
      else if (instance_type == INSTANCE_BORDER_BOTTOM_LEFT)
        {
          a = border_width[3] / br[2].x;
          b = border_width[0] / br[2].y;
        }
      else
        {
          a = border_width[2] / br[3].x;
          b = border_width[3] / br[3].y;
        }

      h = length (tex_coord.st);
      o = (atan (y, x)) + PI/2.0;

      x1 = cos (o) * b;
      y1 = sin (o) * a;
      h1 = 1.0 - length (vec2 (x1, y1));

      mediump float kx = cos (o) * aa_factor_x * aspect_ratio;
      mediump float ky = sin (o) * aa_factor_y;
      k = length (vec2 (kx, ky));
      mediump float hk = k/2.0;

      if (h > 1.0 || h < h1 - k)
        {
          discard;
        }
      else if (h >= 1.0 - k)
        {
          col.a *= (1.0/k) * (1.0 - h);
        }
      else if (h <= h1)
        {
          col.a *= (1.0/k) * (h - h1 + k);
        }
    }
  else
    {
      discard;
    }

  my_FragColor = col;
}
