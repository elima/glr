#ifndef _GLR_COMMAND_H_
#define _GLR_COMMAND_H_

#include <GL/gl.h>
#include <glib.h>
#include "glr-context.h"
#include "glr-tex-cache.h"

G_BEGIN_DECLS

typedef struct _GlrCommand GlrCommand;

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

GlrCommand *     glr_command_new            (const GlrPrimitive *primitive);
void             glr_command_free           (GlrCommand *self);

gboolean         glr_command_is_full        (GlrCommand *self);
gboolean         glr_command_add_instance   (GlrCommand          *self,
                                             const GlrLayout     *layout,
                                             guint32              color,
                                             const GlrTransform  *transform,
                                             const GlrTexSurface *tex_surface);

gboolean         glr_command_draw           (GlrCommand *self,
                                             GLuint      shader_program);
void             glr_command_reset          (GlrCommand *self);

G_END_DECLS

#endif /* _GLR_COMMAND_H_ */
