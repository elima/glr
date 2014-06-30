#include "glr-paint.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void
glr_paint_set_color (GlrPaint *self,
                     guint8    red,
                     guint8    green,
                     guint8    blue,
                     guint8    alpha)
{
  self->color =
    red   << 24 |
    green << 16 |
    blue  <<  8 |
    alpha;
}

void
glr_paint_set_color_float (GlrPaint *self,
                           gfloat    red,
                           gfloat    green,
                           gfloat    blue,
                           gfloat    alpha)
{
  self->color =
    ((guint8) (red   * 255)) << 24 |
    ((guint8) (green * 255)) << 16 |
    ((guint8) (blue  * 255)) <<  8 |
    ((guint8) (alpha * 255));
}

void
glr_paint_set_color_hue (GlrPaint *self, guint hue, guint8 alpha)
{
  gint red_shift = 180;
  gint green_shift = 60;
  gint blue_shift = -60;
  gdouble r, g, b;

  r = (MAX (MIN ((180 - abs (180 - (hue + red_shift) % 360)), 120), 60) - 60) / 60.0;
  g = (MAX (MIN ((180 - abs (180 - (hue + green_shift) % 360)), 120), 60) - 60) / 60.0;
  b = (MAX (MIN ((180 - abs (180 - (hue + blue_shift) % 360)), 120), 60) - 60) / 60.0;

  glr_paint_set_color_float (self, r, g, b, alpha / 255.0);
}

void
glr_paint_set_style (GlrPaint *self, GlrPaintStyle style)
{
  self->style = style;
}
