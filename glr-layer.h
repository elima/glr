#ifndef _GLR_LAYER_H_
#define _GLR_LAYER_H_

#include <GL/gl.h>
#include <glib.h>
#include "glr-command.h"
#include "glr-context.h"
#include "glr-paint.h"

typedef struct _GlrCanvas GlrCanvas;
typedef struct _GlrLayer GlrLayer;

typedef enum
  {
    GLR_LAYER_STATUS_NONE,
    GLR_LAYER_STATUS_DRAWING,
    GLR_LAYER_STATUS_FINISHED
  } GlrLayerStatus;

typedef void (*GlrLayerDrawFunc) (GlrLayer *layer, gpointer user_data);

GlrLayer *     glr_layer_new                         (GlrContext *context);
GlrLayer *     glr_layer_ref                         (GlrLayer *self);
void           glr_layer_unref                       (GlrLayer *self);

void           glr_layer_redraw                      (GlrLayer *self);
void           glr_layer_redraw_in_thread            (GlrLayer         *self,
                                                      GlrLayerDrawFunc  draw_func,
                                                      gpointer          user_data);

void           glr_layer_finish                      (GlrLayer *self);

void           glr_layer_set_transform_origin        (GlrLayer *self,
                                                      gfloat    origin_x,
                                                      gfloat    origin_y);
void           glr_layer_scale                       (GlrLayer *self,
                                                      gfloat    scale_x,
                                                      gfloat    scale_y);
void           glr_layer_rotate                      (GlrLayer *self,
                                                      gfloat    angle);
void           glr_layer_translate                   (GlrLayer *self,
                                                      gfloat    x,
                                                      gfloat    y);

GlrPaint *     glr_layer_get_default_paint           (GlrLayer *self);

void           glr_layer_clear                       (GlrLayer *self);

void           glr_layer_draw_rect                   (GlrLayer *self,
                                                      gfloat    left,
                                                      gfloat    top,
                                                      gfloat    width,
                                                      gfloat    height,
                                                      GlrPaint *paint);

void           glr_layer_draw_char                   (GlrLayer *self,
                                                      guint32   unicode_char,
                                                      gfloat    left,
                                                      gfloat    top,
                                                      GlrFont  *font,
                                                      GlrPaint *paint);

void           glr_layer_draw_rounded_rect           (GlrLayer *self,
                                                      gfloat    left,
                                                      gfloat    top,
                                                      gfloat    width,
                                                      gfloat    height,
                                                      gfloat    border_radius,
                                                      GlrPaint *paint);

/* internal API */
/* @TODO: move these to glr-layer-priv.h */
GQueue *       glr_layer_get_commands                (GlrLayer *self);

#endif /* _GLR_LAYER_H_ */
