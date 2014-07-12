#ifndef _GLR_PRIV_H_
#define _GLR_PRIV_H_

#include "glr-canvas.h"
#include "glr-context.h"
#include "glr-tex-cache.h"

typedef struct
{
  GLenum mode;
  gfloat *vertices;
  gsize num_vertices;
} GlrPrimitive;

enum
  {
    GLR_PRIMITIVE_RECT_FILL = 0,
    GLR_PRIMITIVE_RECT_STROKE,
    GLR_PRIMITIVE_ROUND_CORNER_FILL
  };

typedef enum
  {
    GLR_CMD_NONE,
    GLR_CMD_CREATE_TEX,
    GLR_CMD_UPLOAD_TO_TEX,
    GLR_CMD_COMMIT_CANVAS_FRAME
  } GlrCmdType;

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

const GlrPrimitive *   glr_context_get_primitive        (GlrContext *self,
                                                         guint       primitive_id);

GlrTexCache *          glr_context_get_texture_cache    (GlrContext *self);

void                   glr_context_queue_command        (GlrContext *self,
                                                         GlrCmdType  type,
                                                         gpointer    data,
                                                         GCond      *cond);


GlrTexCache *          glr_tex_cache_new                (GlrContext *context);

GQueue *               glr_layer_get_batches            (GlrLayer *self);

void                   glr_canvas_commit_frame          (GlrCanvas *self);

#endif /* _GLR_PRIV_H_ */
