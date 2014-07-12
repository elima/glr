#ifndef _GLR_BATCH_H_
#define _GLR_BATCH_H_

#include <GL/gl.h>
#include <glib.h>
#include "glr-context.h"
#include "glr-priv.h"
#include "glr-tex-cache.h"

G_BEGIN_DECLS

typedef struct _GlrBatch GlrBatch;

typedef struct __attribute__((__packed__))
{
  gfloat left;
  gfloat top;
  gfloat width;
  gfloat height;
} GlrLayout;

typedef struct __attribute__((__packed__))
{
  gfloat origin_x;
  gfloat origin_y;
  gfloat scale_x;
  gfloat scale_y;
  gfloat rotation_z;
  gfloat parent_rotation_z;
  gfloat parent_origin_x;
  gfloat parent_origin_y;
} GlrTransform;

GlrBatch * glr_batch_new            (const GlrPrimitive *primitive);
void       glr_batch_free           (GlrBatch *self);

gboolean   glr_batch_is_full        (GlrBatch *self);
gboolean   glr_batch_add_instance   (GlrBatch          *self,
                                     const GlrLayout     *layout,
                                     guint32              color,
                                     const GlrTransform  *transform,
                                     const GlrTexSurface *tex_surface);

gboolean   glr_batch_draw           (GlrBatch *self,
                                     GLuint      shader_program);
void       glr_batch_reset          (GlrBatch *self);

G_END_DECLS

#endif /* _GLR_BATCH_H_ */
