#ifndef _GLR_PRIV_H_
#define _GLR_PRIV_H_

#include "glr-context.h"

typedef enum
  {
    GLR_INSTANCE_RECT_BG = 0,
    GLR_INSTANCE_BORDER_TOP,
    GLR_INSTANCE_BORDER_RIGHT,
    GLR_INSTANCE_BORDER_BOTTOM,
    GLR_INSTANCE_BORDER_LEFT,
    GLR_INSTANCE_BORDER_TOP_LEFT,
    GLR_INSTANCE_BORDER_TOP_RIGHT,
    GLR_INSTANCE_BORDER_BOTTOM_LEFT,
    GLR_INSTANCE_BORDER_BOTTOM_RIGHT,
    GLR_INSTANCE_CHAR_GLYPH,
  } GlrInstanceType;

typedef uint32_t GlrInstanceConfig[4];

typedef struct __attribute__((__packed__))
{
  float left;
  float top;
  float width;
  float height;
} GlrLayout;

GlrTexCache *          glr_tex_cache_new                 (GlrContext *context);

#endif /* _GLR_PRIV_H_ */
