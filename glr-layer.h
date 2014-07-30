#ifndef _GLR_LAYER_H_
#define _GLR_LAYER_H_

#include <GL/gl.h>
#include <glib.h>
#include "glr-style.h"
#include "glr-context.h"

typedef struct _GlrCanvas GlrCanvas;
typedef struct _GlrLayer GlrLayer;

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

void           glr_layer_clear                       (GlrLayer *self);

void           glr_layer_clip                        (GlrLayer *self,
                                                      gfloat    top,
                                                      gfloat    left,
                                                      gfloat    width,
                                                      gfloat    height);

void           glr_layer_draw_rect                   (GlrLayer *self,
                                                      gfloat    left,
                                                      gfloat    top,
                                                      gfloat    width,
                                                      gfloat    height,
                                                      GlrStyle *style);

void           glr_layer_draw_char                   (GlrLayer *self,
                                                      guint32   code_point,
                                                      gfloat    left,
                                                      gfloat    top,
                                                      GlrFont  *font,
                                                      GlrColor  color);
void           glr_layer_draw_char_unicode           (GlrLayer *self,
                                                      guint32   unicode_char,
                                                      gfloat    left,
                                                      gfloat    top,
                                                      GlrFont  *font,
                                                      GlrColor  color);

#endif /* _GLR_LAYER_H_ */
