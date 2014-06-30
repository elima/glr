#ifndef _GLR_TARGET_H_
#define _GLR_TARGET_H_

#include <GL/gl.h>
#include <glib.h>

G_BEGIN_DECLS

typedef struct _GlrTarget GlrTarget;

GlrTarget *     glr_target_new             (guint32 width,
                                            guint32 height,
                                            guint8  msaa_samples);
GlrTarget *     glr_target_ref             (GlrTarget *self);
void            glr_target_unref           (GlrTarget *self);

GLuint          glr_target_get_framebuffer (GlrTarget *self);
GLuint          glr_target_get_texture     (GlrTarget *self);

void            glr_target_get_size        (GlrTarget *self,
                                            guint32   *width,
                                            guint32   *height);

G_END_DECLS

#endif /* _GLR_TARGET_H_ */
