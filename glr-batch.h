#ifndef _GLR_BATCH_H_
#define _GLR_BATCH_H_

#include <GL/gl.h>
#include <glib.h>
#include "glr-context.h"
#include "glr-priv.h"
#include "glr-style.h"
#include "glr-tex-cache.h"

typedef struct _GlrBatch GlrBatch;

GlrBatch * glr_batch_new            (void);
GlrBatch * glr_batch_ref            (GlrBatch *self);
void       glr_batch_unref          (GlrBatch *self);

gboolean   glr_batch_is_full        (GlrBatch *self);
gboolean   glr_batch_add_instance   (GlrBatch                *self,
                                     const GlrInstanceConfig  config,
                                     const GlrLayout         *layout);

gboolean   glr_batch_draw           (GlrBatch *self,
                                     GLuint    shader_program);
void       glr_batch_reset          (GlrBatch *self);

goffset    glr_batch_add_dyn_attr   (GlrBatch   *self,
                                     const void *attr_data,
                                     gsize       size);

#endif /* _GLR_BATCH_H_ */
