#ifndef _GLR_CANVAS_H_
#define _GLR_CANVAS_H_

#include <glib.h>
#include "glr-context.h"
#include "glr-layer.h"
#include "glr-paint.h"
#include "glr-target.h"

typedef struct _GlrCanvas GlrCanvas;

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

void                glr_canvas_clear                (GlrCanvas *self,
                                                     GlrPaint  *paint);

#endif /* _GLR_CANVAS_H_ */
