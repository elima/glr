#ifndef _GLR_PAINT_H_
#define _GLR_PAINT_H_

#include <GL/gl.h>
#include <glib.h>

G_BEGIN_DECLS

typedef enum
  {
    GLR_PAINT_STYLE_FILL,
    GLR_PAINT_STYLE_STROKE
  } GlrPaintStyle;

typedef struct _GlrPaint GlrPaint;
struct _GlrPaint
{
  GlrPaintStyle style;

  guint32 color;

  gfloat stroke_width;
  gfloat stroke_factor;
  gfloat stroke_pattern;

  gfloat border_width;
};

typedef struct _GlrFont GlrFont;
struct _GlrFont
{
  gchar *face;
  guint face_index;
  guint size;
};

void        glr_paint_set_color              (GlrPaint *self,
                                              guint8    red,
                                              guint8    green,
                                              guint8    blue,
                                              guint8    alpha);
void        glr_paint_set_color_float        (GlrPaint *self,
                                              gfloat    red,
                                              gfloat    green,
                                              gfloat    blue,
                                              gfloat    alpha);
void        glr_paint_set_color_hue          (GlrPaint *self,
                                              guint     hue,
                                              guint8    alpha);

void        glr_paint_set_style              (GlrPaint      *self,
                                              GlrPaintStyle  style);

G_END_DECLS

#endif /* _GLR_PAINT_H_ */
