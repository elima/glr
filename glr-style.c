#include "glr-style.h"

#include <math.h>

enum
  {
    BORDER_INDEX_LEFT   = 0,
    BORDER_INDEX_TOP    = 1,
    BORDER_INDEX_RIGHT  = 2,
    BORDER_INDEX_BOTTOM = 3
  };

void
glr_border_set_width (GlrBorder *border, guint which, gdouble width)
{
  if ((which & GLR_BORDER_LEFT) > 0)
    border->width[BORDER_INDEX_LEFT] = width;

  if ((which & GLR_BORDER_TOP) > 0)
    border->width[BORDER_INDEX_TOP] = width;

  if ((which & GLR_BORDER_RIGHT) > 0)
    border->width[BORDER_INDEX_RIGHT] = width;

  if ((which & GLR_BORDER_BOTTOM) > 0)
    border->width[BORDER_INDEX_BOTTOM] = width;
}

void
glr_border_set_color (GlrBorder *border, guint which, GlrColor color)
{
  if ((which & GLR_BORDER_LEFT) > 0)
    border->color[BORDER_INDEX_LEFT] = color;

  if ((which & GLR_BORDER_TOP) > 0)
    border->color[BORDER_INDEX_TOP] = color;

  if ((which & GLR_BORDER_RIGHT) > 0)
    border->color[BORDER_INDEX_RIGHT] = color;

  if ((which & GLR_BORDER_BOTTOM) > 0)
    border->color[BORDER_INDEX_BOTTOM] = color;
}

void
glr_border_set_radius (GlrBorder *border, guint which, gdouble radius)
{
  if ((which & GLR_BORDER_TOP_LEFT) > 0)
    border->radius[BORDER_INDEX_LEFT] = radius;

  if ((which & GLR_BORDER_TOP_RIGHT) > 0)
    border->radius[BORDER_INDEX_TOP] = radius;

  if ((which & GLR_BORDER_BOTTOM_LEFT) > 0)
    border->radius[BORDER_INDEX_RIGHT] = radius;

  if ((which & GLR_BORDER_BOTTOM_RIGHT) > 0)
    border->radius[BORDER_INDEX_BOTTOM] = radius;
}

void
glr_background_set_color (GlrBackground *bg, GlrColor color)
{
  bg->type |= GLR_BACKGROUND_COLOR;
  bg->color = color;
}

GlrColor
glr_color_from_rgba (guint8 red, guint8 green, guint8 blue, guint8 alpha)
{
  return
    red   << 24 |
    green << 16 |
    blue  <<  8 |
    alpha;
}

GlrColor
glr_color_from_hue (guint hue, guint8 alpha)
{
  gint red_shift = 180;
  gint green_shift = 60;
  gint blue_shift = -60;
  gdouble r, g, b;

  r = (MAX (MIN ((180 - abs (180 - (hue + red_shift) % 360)), 120), 60) - 60) / 60.0;
  g = (MAX (MIN ((180 - abs (180 - (hue + green_shift) % 360)), 120), 60) - 60) / 60.0;
  b = (MAX (MIN ((180 - abs (180 - (hue + blue_shift) % 360)), 120), 60) - 60) / 60.0;

  return
    ((guint8) (r * 255)) << 24 |
    ((guint8) (g * 255)) << 16 |
    ((guint8) (b * 255)) <<  8 |
    alpha;
}
