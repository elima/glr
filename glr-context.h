#ifndef _GLR_CONTEXT_H_
#define _GLR_CONTEXT_H_

#include <GL/gl.h>
#include <glib.h>

typedef struct _GlrContext GlrContext;

GlrContext *        glr_context_new                  (void);
GlrContext *        glr_context_ref                  (GlrContext *self);
void                glr_context_unref                (GlrContext *self);

/* @FIXME: API below is internal and should be moved to glr-context-priv.h */

#include "glr-tex-cache.h"

typedef struct
{
  GLenum mode;
  gfloat *vertices;
  gsize num_vertices;
} GlrPrimitive;

enum
  {
    GLR_PRIMITIVE_RECT_FILL         = 0,
    GLR_PRIMITIVE_RECT_STROKE,
    GLR_PRIMITIVE_ROUND_CORNER_FILL
  };

const GlrPrimitive *   glr_context_get_primitive     (GlrContext *self,
                                                      guint       primitive_id);

GlrTexCache *          glr_context_get_texture_cache (GlrContext *self);

#endif /* _GLR_CONTEXT_H_ */
