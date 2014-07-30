#ifndef _GLR_CONTEXT_H_
#define _GLR_CONTEXT_H_

#include <GL/gl.h>
#include <glib.h>
#include "glr-tex-cache.h"

typedef struct _GlrContext GlrContext;

GlrContext *        glr_context_new                  (void);
GlrContext *        glr_context_ref                  (GlrContext *self);
void                glr_context_unref                (GlrContext *self);

void                glr_context_lock_gl              (GlrContext *self);
void                glr_context_unlock_gl            (GlrContext *self);

GlrTexCache *       glr_context_get_texture_cache    (GlrContext *self);

#endif /* _GLR_CONTEXT_H_ */
