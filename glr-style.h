#ifndef _GLR_STYLE_H_
#define _GLR_STYLE_H_

#include <stdint.h>

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
    GLR_BACKGROUND_NONE            = 0,
    GLR_BACKGROUND_COLOR           = 1,
    GLR_BACKGROUND_IMAGE           = 2,
    GLR_BACKGROUND_LINEAR_GRADIENT = 3,
    GLR_BACKGROUND_RADIAL_GRADIENT = 4
  } GlrBackgroundType;

typedef struct _GlrStyle      GlrStyle;
typedef struct _GlrBorder     GlrBorder;
typedef struct _GlrBackground GlrBackground;
typedef uint32_t              GlrColor;

struct _GlrBackground
{
  uint32_t type;
  GlrColor color;

  // @FIXME: by now only 2 color steps are implemented: first and last
  GlrColor linear_grad_colors[2];
  float linear_grad_steps[2];
  uint8_t linear_grad_steps_count;
  float linear_grad_angle;
};

struct _GlrBorder
{
  double width[4];
  double radius[4];
  GlrColor color[4];
  GlrBorderStyle style[4];
};

typedef struct _GlrFont GlrFont;
struct _GlrFont
{
  char *face;
  uint32_t face_index;
  uint32_t size;
};

struct _GlrStyle
{
  GlrBorder border;
  GlrBackground background;
};

void          glr_border_set_width                  (GlrBorder *border,
                                                     uint8_t    which,
                                                     double     width);
void          glr_border_set_color                  (GlrBorder *border,
                                                     uint8_t    which,
                                                     GlrColor   color);
void          glr_border_set_radius                 (GlrBorder *border,
                                                     uint8_t    which,
                                                     double     radius);

void          glr_background_set_color              (GlrBackground *bg,
                                                     GlrColor       color);
void          glr_background_set_linear_gradient    (GlrBackground *bg,
                                                     float          angle,
                                                     GlrColor       first_color,
                                                     GlrColor       last_color);

GlrColor      glr_color_from_rgba                   (uint8_t red,
                                                     uint8_t green,
                                                     uint8_t blue,
                                                     uint8_t alpha);
GlrColor      glr_color_from_hue                    (uint32_t hue,
                                                     uint8_t  alpha);

#endif /* _GLR_STYLE_H_ */
