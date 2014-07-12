#ifndef _GLR_CANVAS_H_
#define _GLR_CANVAS_H_

#include <GL/gl.h>
#include <glib.h>
#include "glr-context.h"
#include "glr-layer.h"
#include "glr-paint.h"
#include "glr-target.h"

G_BEGIN_DECLS

typedef struct _GlrCanvas GlrCanvas;

typedef void (* GlrCanvasLayerDrawFunc) (GlrCanvas *self,
                                         GlrLayer  *layer,
                                         guint64    frame_count,
                                         gpointer   user_data);

GlrCanvas *         glr_canvas_new                  (GlrContext *context,
                                                     GlrTarget  *target);
GlrCanvas *         glr_canvas_ref                  (GlrCanvas *self);
void                glr_canvas_unref                (GlrCanvas *self);

GlrTarget *         glr_canvas_get_target           (GlrCanvas *self);

void                glr_canvas_start_frame          (GlrCanvas *self);
void                glr_canvas_finish_frame         (GlrCanvas *self);
void                glr_canvas_attach_layer         (GlrCanvas *self,
                                                     guint      index,
                                                     GlrLayer  *layer);

void                glr_canvas_draw_rect            (GlrCanvas *self,
                                                     gfloat     left,
                                                     gfloat     top,
                                                     gfloat     width,
                                                     gfloat     height,
                                                     GlrPaint  *paint);

void                glr_canvas_clear                (GlrCanvas *self,
                                                     GlrPaint  *paint);

void                glr_canvas_layer_draw           (GlrCanvas              *self,
                                                     guint16                 layer_index,
                                                     GlrCanvasLayerDrawFunc  func,
                                                     gpointer                user_data);

G_END_DECLS

#endif /* _GLR_CANVAS_H_ */
