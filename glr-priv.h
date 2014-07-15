#ifndef _GLR_PRIV_H_
#define _GLR_PRIV_H_

#include "glr-canvas.h"
#include "glr-context.h"
#include "glr-tex-cache.h"

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

typedef guint32 GlrInstanceConfig[4];

typedef enum
  {
    GLR_CMD_NONE,
    GLR_CMD_CREATE_TEX,
    GLR_CMD_UPLOAD_TO_TEX,
    GLR_CMD_COMMIT_CANVAS_FRAME
  } GlrCmdType;

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
  gfloat rotation_z;
  gfloat pre_rotation_z;
  gfloat scale_x;
  gfloat scale_y;
  gfloat _padding_0_;
  gfloat _padding_1_;
} GlrTransform;

typedef struct
{
  GLuint texture_id;
  GLint x_offset;
  GLint y_offset;
  GLsizei width;
  GLsizei height;
  GLvoid *buffer;
  GLenum format;
  GLenum type;
} GlrCmdUploadToTex;

typedef struct
{
  GLuint texture_id;
  GLint internal_format;
  GLsizei width;
  GLsizei height;
  GLenum format;
  GLenum type;
} GlrCmdCreateTex;

typedef struct
{
  GlrCanvas *canvas;
} GlrCmdCommitCanvasFrame;

GlrTexCache *          glr_context_get_texture_cache     (GlrContext *self);

void                   glr_context_queue_command         (GlrContext *self,
                                                          GlrCmdType  type,
                                                          gpointer    data,
                                                          GCond      *cond);

GlrTexCache *          glr_tex_cache_new                 (GlrContext *context);

GQueue *               glr_layer_get_batches             (GlrLayer *self);

void                   glr_canvas_commit_frame           (GlrCanvas *self);

#endif /* _GLR_PRIV_H_ */
