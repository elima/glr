#include "glr-style.h"

#include <glib.h>
#include <stdlib.h>

enum
  {
    BORDER_INDEX_LEFT   = 0,
    BORDER_INDEX_TOP    = 1,
    BORDER_INDEX_RIGHT  = 2,
    BORDER_INDEX_BOTTOM = 3
  };

void
glr_border_set_width (GlrBorder *border, uint8_t which, double width)
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
glr_border_set_color (GlrBorder *border, uint8_t which, GlrColor color)
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
glr_border_set_radius (GlrBorder *border, uint8_t which, double radius)
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
  bg->type = GLR_BACKGROUND_COLOR;
  bg->color = color;
}

void
glr_background_set_linear_gradient (GlrBackground *bg,
                                    float          angle,
                                    GlrColor       first_color,
                                    GlrColor       last_color)
{
  bg->type = GLR_BACKGROUND_LINEAR_GRADIENT;

  bg->linear_grad_angle = angle;
  bg->linear_grad_colors[0] = first_color;
  bg->linear_grad_colors[1] = last_color;
  bg->linear_grad_steps[0] = 0.0;
  bg->linear_grad_steps[1] = 1.0;
  bg->linear_grad_steps_count = 2;
}

GlrColor
glr_color_from_rgba (uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
  return
    red   << 24 |
    green << 16 |
    blue  <<  8 |
    alpha;
}

GlrColor
glr_color_from_hue (uint32_t hue, uint8_t alpha)
{
  int32_t red_shift = 180;
  int32_t green_shift = 60;
  int32_t blue_shift = -60;
  double r, g, b;

  r = (MAX (MIN ((180 - abs (180 - (hue + red_shift) % 360)), 120), 60) - 60) / 60.0;
  g = (MAX (MIN ((180 - abs (180 - (hue + green_shift) % 360)), 120), 60) - 60) / 60.0;
  b = (MAX (MIN ((180 - abs (180 - (hue + blue_shift) % 360)), 120), 60) - 60) / 60.0;

  return
    ((guint8) (r * 255)) << 24 |
    ((guint8) (g * 255)) << 16 |
    ((guint8) (b * 255)) <<  8 |
    alpha;
}
