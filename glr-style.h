#ifndef _GLR_STYLE_H_
#define _GLR_STYLE_H_

#include <GL/gl.h>
#include <glib.h>

#define GLR_STYLE_DEFAULT {{{0}}};

#define GLR_COLOR_NONE 0x00000000

typedef enum
  {
    GLR_BORDER_NONE         =      0,
    GLR_BORDER_LEFT         = 1 << 0,
    GLR_BORDER_TOP          = 1 << 1,
    GLR_BORDER_RIGHT        = 1 << 2,
    GLR_BORDER_BOTTOM       = 1 << 3,
    GLR_BORDER_TOP_LEFT     = 1 << 4,
    GLR_BORDER_TOP_RIGHT    = 1 << 5,
    GLR_BORDER_BOTTOM_LEFT  = 1 << 6,
    GLR_BORDER_BOTTOM_RIGHT = 1 << 7,
    GLR_BORDER_ALL          =   0xFF
  } GlrBorderType;

typedef enum
  {
    GLR_BORDER_STYLE_NONE =  0,
    GLR_BORDER_STYLE_HIDDEN,
    GLR_BORDER_STYLE_DOTTED,
    GLR_BORDER_STYLE_DASHED,
    GLR_BORDER_STYLE_SOLID,
    GLR_BORDER_STYLE_DOUBLE,
    GLR_BORDER_STYLE_GROOVE,
    GLR_BORDER_STYLE_RIDGE,
    GLR_BORDER_STYLE_INSET,
    GLR_BORDER_STYLE_OUTSET
  } GlrBorderStyle;

typedef enum
  {
    GLR_BACKGROUND_NONE  =      0,
    GLR_BACKGROUND_COLOR = 1 << 0,
    GLR_BACKGROUND_IMAGE = 1 << 1
  } GlrBackgroundType;

typedef struct _GlrStyle      GlrStyle;
typedef struct _GlrBorder     GlrBorder;
typedef struct _GlrBackground GlrBackground;
typedef guint32               GlrColor;

struct _GlrBackground
{
  guint type;
  GlrColor color;
};

struct _GlrBorder
{
  gdouble width[4];
  gdouble radius[4];
  GlrColor color[4];
  GlrBorderStyle style[4];
};

typedef struct _GlrFont GlrFont;
struct _GlrFont
{
  gchar *face;
  guint face_index;
  guint size;
};

struct _GlrStyle
{
  GlrBorder border;
  GlrBackground background;
};

void          glr_border_set_width                  (GlrBorder *border,
                                                     guint      which,
                                                     gdouble    width);
void          glr_border_set_color                  (GlrBorder *border,
                                                     guint      which,
                                                     GlrColor   color);
void          glr_border_set_radius                 (GlrBorder *border,
                                                     guint      which,
                                                     gdouble    radius);

void          glr_background_set_color              (GlrBackground *bg,
                                                     GlrColor       color);

GlrColor      glr_color_from_rgba                   (guint8 red,
                                                     guint8 green,
                                                     guint8 blue,
                                                     guint8 alpha);
GlrColor      glr_color_from_hue                    (guint  hue,
                                                     guint8 alpha);

#endif /* _GLR_STYLE_H_ */
