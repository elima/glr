#ifndef _GLR_BATCH_H_
#define _GLR_BATCH_H_

#include "glr-priv.h"
#include "glr-style.h"
#include <stdbool.h>

typedef struct _GlrBatch GlrBatch;

GlrBatch * glr_batch_new            (void);
GlrBatch * glr_batch_ref            (GlrBatch *self);
void       glr_batch_unref          (GlrBatch *self);

bool       glr_batch_is_full        (GlrBatch *self);
bool       glr_batch_add_instance   (GlrBatch                *self,
                                     const GlrLayout         *layout,
                                     GlrColor                 color,
                                     const GlrInstanceConfig  config);

bool       glr_batch_draw           (GlrBatch *self,
                                     GLuint    shader_program);

void       glr_batch_reset          (GlrBatch *self);

size_t     glr_batch_add_dyn_attr   (GlrBatch   *self,
                                     const void *attr_data,
                                     size_t      size);

#endif /* _GLR_BATCH_H_ */
