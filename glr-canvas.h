#ifndef _GLR_CANVAS_H_
#define _GLR_CANVAS_H_

#include "glr-context.h"
#include "glr-target.h"
#include "glr-style.h"

typedef struct _GlrCanvas GlrCanvas;

GlrCanvas *         glr_canvas_new                  (GlrContext *context,
                                                     GlrTarget  *target);
GlrCanvas *         glr_canvas_ref                  (GlrCanvas *self);
void                glr_canvas_unref                (GlrCanvas *self);

GlrTarget *         glr_canvas_get_target           (GlrCanvas *self);

void                glr_canvas_flush                (GlrCanvas *self);

void                glr_canvas_clear                (GlrCanvas *self,
                                                     GlrColor   color);

void                glr_canvas_translate            (GlrCanvas *self,
                                                     float      x,
                                                     float      y,
                                                     float      z);
void                glr_canvas_rotate               (GlrCanvas *self,
                                                     float      angle_x,
                                                     float      angle_y,
                                                     float      angle_z);
void                glr_canvas_set_transform_origin (GlrCanvas *self,
                                                     float      origin_x,
                                                     float      origin_y,
                                                     float      origin_z);
void                glr_canvas_scale                (GlrCanvas *self,
                                                     float      scale_x,
                                                     float      scale_y,
                                                     float      scale_z);
void                glr_canvas_reset_transform      (GlrCanvas *self);

void                glr_canvas_draw_rect            (GlrCanvas *self,
                                                     float      left,
                                                     float      top,
                                                     float      width,
                                                     float      height,
                                                     GlrStyle  *style);
void                glr_canvas_draw_char            (GlrCanvas *self,
                                                     uint32_t   code_point,
                                                     float      left,
                                                     float      top,
                                                     GlrFont   *font,
                                                     GlrColor   color);
void                glr_canvas_draw_char_unicode    (GlrCanvas *self,
                                                     uint32_t   unicode_char,
                                                     float      left,
                                                     float      top,
                                                     GlrFont   *font,
                                                     GlrColor   color);

#endif /* _GLR_CANVAS_H_ */
