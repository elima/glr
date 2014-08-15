#version 300 es

precision highp float;

const float PI = 3.14159265359;

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

const int BACKGROUND_TYPE_NONE         = 0;
const int BACKGROUND_TYPE_SOLID_COLOR  = 1;
const int BACKGROUND_TYPE_IMAGE        = 2;
const int BACKGROUND_TYPE_LINEAR_GRAD  = 3;
const int BACKGROUND_TYPE_RADIAL_GRAD  = 4;

     out vec4  my_FragColor;

     in  vec4  color;
     in  vec2  tex_coords;

flat in  vec2  norm;
flat in  vec2  aa_size;

flat in  uint  instance_type;

flat in  vec4  area_in_tex;
flat in  int   tex_id;

flat in  ivec4 border_style;
flat in  vec4  border_width;
flat in  vec2  border_radius[4];
flat in  uvec4 border_color;

flat in  int   background_type;

flat in  vec4  linear_grad_colors[2];
flat in  float linear_grad_steps[4];
flat in  float linear_grad_angle;
flat in  float linear_grad_gamma;
flat in  float linear_grad_length;

uniform sampler2D glyph_cache[8];

vec4
sample_glyph_cache (int tex_id, mediump vec2 tex_coord)
{
  switch (tex_id)
    {
    case 0: return texture (glyph_cache[0], tex_coord);
    case 1: return texture (glyph_cache[1], tex_coord);
    case 2: return texture (glyph_cache[2], tex_coord);
    case 3: return texture (glyph_cache[3], tex_coord);
    case 4: return texture (glyph_cache[4], tex_coord);
    case 5: return texture (glyph_cache[5], tex_coord);
    case 6: return texture (glyph_cache[6], tex_coord);
    case 7: return texture (glyph_cache[7], tex_coord);
    }
}

bool
draw_round_corner (vec4 col, float x, float y, float rx, float ry)
{
  if (x >= rx || y >= ry || x < 0.0 || y < 0.0)
    return false;

  float ar = ry/rx;
  float o = (atan (y, x*ar)) + PI/2.0;

  float x1 = cos (o) * rx * ar;
  float y1 = sin (o) * ry;
  float h1 = length (vec2 (x1, y1));

  float kx = cos (o) * aa_size.x * ar;
  float ky = sin (o) * aa_size.y;
  float k = length (vec2 (kx, ky));

  float h = length (vec2 ((rx - x)*ar, ry - y));

  if (h > h1)
    discard;
  else if (h > h1 - k)
    col.a *= (1.0/k) * (h1 - h);

  my_FragColor = col;

  return true;
}

bool
draw_border_corner (vec4 col,
                    float x, float y,
                    float ar,
                    vec2 outer_radi,
                    vec2 inner_radi)
{
  if (x > outer_radi.x + aa_size.x || y > outer_radi.y + aa_size.y)
    return false;

  float o = (atan (y, x*ar)) + PI/2.0;

  float x1 = cos (o) * outer_radi.x * ar;
  float y1 = sin (o) * outer_radi.y;
  float h1 = length (vec2 (x1, y1));

  float x2 = cos (o) * inner_radi.x * ar;
  float y2 = sin (o) * inner_radi.y;
  float h2 = length (vec2 (x2, y2));

  float kx = cos (o) * aa_size.x * ar;
  float ky = sin (o) * aa_size.y;
  float k = length (vec2 (kx, ky));

  float h = length (vec2 ((outer_radi.x - x)*ar, outer_radi.y - y));

  if (h > h1 || h <= h2 - k)
    discard;
  else if (h > h1 - k)
    col.a *= (1.0/k) * (h1 - h);
  else if (h <= h2)
    col.a *= (1.0/k) * (h - (h2 - k));

  my_FragColor = col;

  return true;
}

vec4
apply_vertical_aa (float s, vec4 col)
{
  if (s < aa_size.s)
    col.a *= (1.0/aa_size.s) * s;
  else if (s > 1.0 - aa_size.s)
    col.a *= (1.0/aa_size.s) * (1.0 - s);

  return col;
}

vec4
apply_horiz_aa (float t, vec4 col)
{
  if (t < aa_size.t)
    col.a *= (1.0/aa_size.t) * t;
  else if (t > 1.0 - aa_size.t)
    col.a *= (1.0/aa_size.t) * (1.0 - t);

  return col;
}

vec4
apply_linear_gradient (vec4 color, float s, float t)
{
  float x, y, k;

  if (linear_grad_angle < PI/2.0)
    {
      x = s;
      y = t;
      k = y;
    }
  else if (linear_grad_angle < PI)
    {
      x = 1.0 - s;
      k = x;
      y = t;
    }
  else if (linear_grad_angle < PI/2.0*3.0)
    {
      x = 1.0 - s;
      y = 1.0 - t;
      k = y;
    }
  else
    {
      x = s;
      y = 1.0 - t;
      k = x;
    }

  float st = length (vec2 (x, y));
  float teta = asin (k / st);
  float d = st * sin (linear_grad_gamma + teta);

  color = mix (linear_grad_colors[0],
               linear_grad_colors[1],
               d / linear_grad_length);

  return color;
}

void
main ()
{
  vec4 col = color;

  float s = tex_coords.s;
  float t = tex_coords.t;

  vec2 br[4] = border_radius;
  vec4 bw = border_width;

  // character glyph
  // ---------------------------------------------------------------------------
  if (instance_type == INSTANCE_CHAR_GLYPH) {
    float f = 0.0;
    vec4 a;

    a = sample_glyph_cache (tex_id, vec2 (area_in_tex.x + s * area_in_tex.z,
                                          area_in_tex.y + t * area_in_tex.w));

    f = a.r;
    if (f == 0.0)
      discard;

    col.a *= f;
  }

  // rectangle background
  // ---------------------------------------------------------------------------
  else if (instance_type == INSTANCE_RECT_BG) {
    float rx, ry;

    // consider type of background

    // linear gradient
    if (background_type == BACKGROUND_TYPE_LINEAR_GRAD)
      col = apply_linear_gradient (col, s, t);

    // top-left round corner
    if (br[0].x * br[0].y > 0.0) {
        rx = br[0].x * norm.x;
        ry = br[0].y * norm.y;
        if (draw_round_corner (col, s, t, rx, ry))
          return;
      }

    // top-right round corner
    if (br[1].x * br[1].y > 0.0) {
        rx = br[1].x * norm.x;
        ry = br[1].y * norm.y;
        if (draw_round_corner (col, 1.0 - s, t, rx, ry))
          return;
    }

    // bottom-left round corner
    if (br[2].x * br[2].y > 0.0) {
        rx = br[2].x * norm.x;
        ry = br[2].y * norm.y;
        if (draw_round_corner (col, s, 1.0 - t, rx, ry))
          return;
      }

    // bottom-right round corner
    if (br[3].x * br[3].y > 0.0) {
        rx = br[3].x * norm.x;
        ry = br[3].y * norm.y;
        if (draw_round_corner (col, 1.0 - s, 1.0 - t, rx, ry))
          return;
    }

    col = apply_vertical_aa (s, col);
    col = apply_horiz_aa (t, col);
  }

  // solid border left or right
  // ---------------------------------------------------------------------------
  else if (instance_type == INSTANCE_BORDER_LEFT || instance_type == INSTANCE_BORDER_RIGHT) {
    col = apply_vertical_aa (s, col);
  }

  // solid border top or bottom
  // ---------------------------------------------------------------------------
  else if (instance_type == INSTANCE_BORDER_TOP || instance_type == INSTANCE_BORDER_BOTTOM) {
    col = apply_horiz_aa (t, col);
  }

  // top-left solid border corner
  // ---------------------------------------------------------------------------
  else if (instance_type == INSTANCE_BORDER_TOP_LEFT) {
    if (br[0].x > 0.0 && br[0].y > 0.0) {
      vec2 outer_radi = vec2 (br[0].x * norm.x, br[0].y * norm.y);
      vec2 inner_radi = vec2 (min ((bw[0] - br[0].x) * norm.x, 0.0),
                              min ((bw[1] - br[0].y) * norm.y, 0.0));
      float ar = max (br[0].y, bw[1]) / max (br[0].x, bw[0]);
      if (draw_border_corner (col, s, t, ar, outer_radi, inner_radi))
        return;
    }

    if (s < aa_size.s)
      col.a *= (1.0/aa_size.s) * s;
    if (t < aa_size.t)
      col.a *= (1.0/aa_size.t) * t;
  }

  // top-right solid border corner
  // ---------------------------------------------------------------------------
  else if (instance_type == INSTANCE_BORDER_TOP_RIGHT) {
    if (br[1].x > 0.0 && br[1].y > 0.0) {
      vec2 outer_radi = vec2 (br[1].x * norm.x, br[1].y * norm.y);
      vec2 inner_radi = vec2 (min ((bw[2] - br[1].x) * norm.x, 0.0),
                              min ((bw[1] - br[1].y) * norm.y, 0.0));
      float ar = max (br[1].y, bw[1]) / max (br[1].x, bw[2]);
      if (draw_border_corner (col, 1.0 - s, t, ar, outer_radi, inner_radi))
        return;
    }

    if (s > 1.0 - aa_size.s)
      col.a *= (1.0/aa_size.s) * (1.0 - s);
    if (t < aa_size.t)
      col.a *= (1.0/aa_size.t) * t;
  }

  // bottom-right solid border corner
  // ---------------------------------------------------------------------------
  else if (instance_type == INSTANCE_BORDER_BOTTOM_RIGHT) {
    if (br[2].x > 0.0 && br[2].y > 0.0) {
      vec2 outer_radi = vec2 (br[2].x * norm.x, br[2].y * norm.y);
      vec2 inner_radi = vec2 (min ((bw[2] - br[2].x) * norm.x, 0.0),
                              min ((bw[3] - br[2].y) * norm.y, 0.0));
      float ar = max (br[2].y, bw[2]) / max (br[2].x, bw[3]);
      if (draw_border_corner (col, 1.0 - s, 1.0 - t, ar, outer_radi, inner_radi))
        return;
    }

    if (s > 1.0 - aa_size.s)
      col.a *= (1.0/aa_size.s) * (1.0 - s);
    if (t > 1.0 - aa_size.t)
      col.a *= (1.0/aa_size.t) * (1.0 - t);
  }

  // bottom-left solid border corner
  // ---------------------------------------------------------------------------
  else if (instance_type == INSTANCE_BORDER_BOTTOM_LEFT) {
    if (br[3].x > 0.0 && br[3].y > 0.0) {
      vec2 outer_radi = vec2 (br[3].x * norm.x, br[3].y * norm.y);
      vec2 inner_radi = vec2 (min ((bw[0] - br[3].x) * norm.x, 0.0),
                              min ((bw[3] - br[3].y) * norm.y, 0.0));
      float ar = max (br[3].y, bw[0]) / max (br[3].x, bw[3]);
      if (draw_border_corner (col, s, 1.0 - t, ar, outer_radi, inner_radi))
        return;
    }

    if (s < aa_size.s)
      col.a *= (1.0/aa_size.s) * s;
    if (t > 1.0 - aa_size.t)
      col.a *= (1.0/aa_size.t) * (1.0 - t);
  }

  my_FragColor = col;
}
