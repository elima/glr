#include <glib.h>
#include <sys/time.h>
#include <math.h>

#include "../glr.h"
#include "utils.h"

#define MSAA_SAMPLES 0

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT  960

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;
static GlrLayer *layer = NULL;

static guint window_width, window_height;
static gfloat zoom_factor = 1.0;

static void
draw_layer_in_thread (GlrLayer *layer, gpointer user_data)
{
  GlrStyle style = GLR_STYLE_DEFAULT;
  guint width, height;
  guint rect_width, rect_height;
  guint padding;
  guint frame = * (guint *) user_data;
  GlrColor bg_color;
  gfloat anim;

  padding = 50;
  anim = sin ((M_PI*2.0/180.0) * (frame % 180));
  bg_color = glr_color_from_rgba (32, 64, 64, 128);

  glr_target_get_size (target, &width, &height);
  width *= zoom_factor;
  height *= zoom_factor;
  rect_width = (width - padding * 2) / 6 - padding;
  rect_height = (height - padding * 2) / 3 - padding;

  glr_layer_rotate (layer, 0.0);
  glr_layer_scale (layer, 1.0, 1.0);
  glr_layer_set_transform_origin (layer, 0.5, 0.5);

  /* rectangles without border width */
  /* ------------------------------------------------------------------------ */
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 0.0);

  /* no border radius */
  glr_background_set_color (&(style.background), bg_color);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 0.0);
  glr_layer_draw_rect (layer, padding, padding, rect_width, rect_height, &style);

  /* border radius 10px */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 10.0 * zoom_factor);
  glr_layer_draw_rect (layer,
                       padding*2 + rect_width, padding,
                       rect_width, rect_height, &style);

  /* border radius 50px */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 50.0 * zoom_factor);
  glr_layer_draw_rect (layer,
                       padding*3 + rect_width*2, padding,
                       rect_width, rect_height, &style);

  /* border radius 25px and rotating */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 25.0 * zoom_factor);
  glr_layer_rotate (layer, 0.0 - (frame*2) / 2000.0 * 180.0);
  glr_layer_draw_rect (layer,
                       padding*4 + rect_width*3, padding,
                       rect_width, rect_height, &style);
  glr_layer_rotate (layer, 0.0);

  /* border radius 5px and scaled */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 5.0 * zoom_factor);
  glr_layer_scale (layer, (anim + M_PI/2.0) / 2.0, (anim + M_PI/2.0) / 2.0);
  glr_layer_draw_rect (layer,
                       padding*5 + rect_width*4, padding,
                       rect_width, rect_height, &style);
  glr_layer_scale (layer, 1.0, 1.0);

  /* no background, no border radius and small rotation (nothing rendered) */
  glr_background_set_color (&(style.background), GLR_COLOR_NONE);
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 0.0);
  glr_layer_rotate (layer, 7.0);
  glr_layer_draw_rect (layer,
                       padding*6 + rect_width*5, padding,
                       rect_width, rect_height, &style);
  glr_layer_rotate (layer, 0.0);

  /* rectangles width border without background */
  /* ------------------------------------------------------------------------ */
  glr_background_set_color (&(style.background), GLR_COLOR_NONE);
  glr_border_set_color (&(style.border),
                        GLR_BORDER_ALL,
                        glr_color_from_rgba (64, 64, 64, 200));

  /* width 1.0, no border radius */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 0.0 * zoom_factor);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 1.0 * zoom_factor);
  glr_layer_draw_rect (layer,
                       padding,
                       padding*2 + rect_height,
                       rect_width, rect_height, &style);

  /* width 3.0, border radius 10px */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 10.0 * zoom_factor);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 3.0 * zoom_factor);
  glr_layer_draw_rect (layer,
                       padding*2 + rect_width,
                       padding*2 + rect_height,
                       rect_width, rect_height, &style);

  /* width 5.0, border radius 50px */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 50.0 * zoom_factor);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 5.0 * zoom_factor);
  glr_layer_draw_rect (layer,
                       padding*3 + rect_width*2,
                       padding*2 + rect_height,
                       rect_width, rect_height, &style);

  /* width 10.0, border radius 25px and rotating */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 25.0 * zoom_factor);
  glr_layer_rotate (layer, 0.0 - (frame*2) / 2000.0 * 180.0);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 10.0 * zoom_factor);
  glr_layer_draw_rect (layer,
                       padding*4 + rect_width*3,
                       padding*2 + rect_height,
                       rect_width, rect_height, &style);

  /* width 25px, border radius 50px and scaled */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 50.0 * zoom_factor);
  glr_layer_rotate (layer, 0.0);
  glr_layer_scale (layer, (anim + M_PI/2.0) / 2.0, (anim + M_PI/2.0) / 2.0);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 25.0 * zoom_factor);
  glr_layer_draw_rect (layer,
                       padding*5 + rect_width*4,
                       padding*2 + rect_height,
                       rect_width, rect_height, &style);
  glr_layer_scale (layer, 1.0, 1.0);

  /* no background, no border radius, and small rotation */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 0.0 * zoom_factor);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 1.0 * zoom_factor);
  glr_layer_rotate (layer, 7.0);
  glr_layer_draw_rect (layer,
                       padding*6 + rect_width*5,
                       padding*2 + rect_height,
                       rect_width, rect_height, &style);
  glr_layer_rotate (layer, 0.0);

  /* rectangles width border and background */
  /* ------------------------------------------------------------------------ */
  glr_background_set_color (&(style.background),
                            glr_color_from_rgba (255, 0, 0, 128));

  /* width 1.0, no border radius */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 0.0 * zoom_factor);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 1.0 * zoom_factor);
  glr_layer_draw_rect (layer,
                       padding,
                       padding*3 + rect_height*2,
                       rect_width, rect_height, &style);

  /* width 3.0, border radius 10px */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 10.0 * zoom_factor);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 3.0 * zoom_factor);
  glr_layer_draw_rect (layer,
                       padding*2 + rect_width,
                       padding*3 + rect_height*2,
                       rect_width, rect_height, &style);

  /* width 5.0, border radius 50px */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 50.0 * zoom_factor);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 5.0 * zoom_factor);
  glr_layer_draw_rect (layer,
                       padding*3 + rect_width*2,
                       padding*3 + rect_height*2,
                       rect_width, rect_height, &style);

  /* width 10.0, border radius 25px and rotating */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 25.0 * zoom_factor);
  glr_layer_rotate (layer, 0.0 - (frame*2) / 2000.0 * 180.0);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 10.0 * zoom_factor);
  glr_layer_draw_rect (layer,
                       padding*4 + rect_width*3,
                       padding*3 + rect_height*2,
                       rect_width, rect_height, &style);

  /* width 25px, border radius 50px and scaled */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 50.0 * zoom_factor);
  glr_layer_rotate (layer, 0.0);
  glr_layer_scale (layer, (anim + M_PI/2.0) / 2.0, (anim + M_PI/2.0) / 2.0);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 25.0 * zoom_factor);
  glr_layer_draw_rect (layer,
                       padding*5 + rect_width*4,
                       padding*3 + rect_height*2,
                       rect_width, rect_height, &style);
  glr_layer_scale (layer, 1.0, 1.0);

  /* no background, no border radius, and small rotation */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 0.0 * zoom_factor);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 1.0 * zoom_factor);
  glr_layer_rotate (layer, 7.0);
  glr_layer_draw_rect (layer,
                       padding*6 + rect_width*5,
                       padding*3 + rect_height*2,
                       rect_width, rect_height, &style);
  glr_layer_rotate (layer, 0.0);

  /* it is necessary to always call finish() on on a layer, otherwise
     glr_canvas_finish_frame() will wait forever */
  glr_layer_finish (layer);
}

static void
draw_func (guint frame, gfloat _zoom_factor, gpointer user_data)
{
  zoom_factor = _zoom_factor;

  glClearColor (0.0, 0.0, 0.0, 0.0);
  glClear (GL_COLOR_BUFFER_BIT);

  glr_canvas_start_frame (canvas);
  glr_canvas_clear (canvas, glr_color_from_rgba (255, 255, 255, 255));

  /* attach our layer */
  glr_canvas_attach_layer (canvas, 0, layer);

  /* invalidate layer and draw it */
  glr_layer_redraw_in_thread (layer, draw_layer_in_thread, &frame);

  /* finally, sync all layers and render */
  glr_canvas_finish_frame (canvas);

  /* blit target's framebuffer into default FBO */
  guint current_width, current_height;
  glr_target_get_size (target, &current_width, &current_height);
  glBindFramebuffer (GL_FRAMEBUFFER, 0);
  glBindFramebuffer (GL_READ_FRAMEBUFFER, glr_target_get_framebuffer (target));
  glBlitFramebuffer (0, 0,
                     current_width, current_height,
                     0, window_height - current_height,
                     current_width, current_height,
                     GL_COLOR_BUFFER_BIT,
                     GL_NEAREST);
}

static void
resize_func (guint width, guint height, gpointer user_data)
{
  glr_target_resize (target, width, height);
  window_width = width;
  window_height = height;
}

gint
main (int argc, char* argv[])
{
  window_width = WINDOW_WIDTH;
  window_height = WINDOW_HEIGHT;

  /* init windowing system */
  utils_initialize_x11 (WINDOW_WIDTH, WINDOW_HEIGHT, "Solid Borders");
  utils_initialize_egl ();

  /* init glr */
  context = glr_context_new ();
  target = glr_target_new (context, WINDOW_WIDTH, WINDOW_HEIGHT, MSAA_SAMPLES);
  canvas = glr_canvas_new (context, target);
  layer = glr_layer_new (context);

  /* start the show */
  utils_main_loop (draw_func, resize_func, NULL);

  /* clean up */
  glr_layer_unref (layer);
  glr_canvas_unref (canvas);
  glr_target_unref (target);
  glr_context_unref (context);

  utils_finalize_egl ();
  utils_finalize_x11 ();

  return 0;
}
