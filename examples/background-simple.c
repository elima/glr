#include <math.h>

#include "../glr.h"
#include "utils.h"

#define MSAA_SAMPLES 0

#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1080

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;

static uint32_t window_width, window_height;
static float zoom_factor = 1.0;

static void
draw_frame (uint32_t frame)
{
  GlrStyle style = GLR_STYLE_DEFAULT;
  uint32_t width, height;
  uint32_t rect_width, rect_height;
  uint32_t padding;
  GlrColor bg_color;
  float anim;

  padding = 50;
  anim = sin ((M_PI*2.0/180.0) * (frame % 180));
  bg_color = glr_color_from_rgba (32, 64, 64, 128);

  glr_target_get_size (target, &width, &height);
  width *= zoom_factor;
  height *= zoom_factor;
  rect_width = (width - padding * 2) / 4 - padding;
  rect_height = (height - padding * 2) / 2 - padding;

  glr_canvas_reset_transform (canvas);
  glr_canvas_set_transform_origin (canvas, rect_width / 2.0, rect_height / 2.0, 0.0);

  /* rectangles without border width */
  /* ------------------------------------------------------------------------ */
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 0.0);

  /* no border radius */
  glr_background_set_color (&(style.background), glr_color_from_rgba (255, 128, 0, 200));
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 0.0);
  glr_canvas_draw_rect (canvas, padding, padding, rect_width, rect_height, &style);

  /* border radius 50px */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 50.0 * zoom_factor);
  glr_background_set_color (&(style.background), glr_color_from_rgba (128, 128, 0, 200));
  glr_canvas_draw_rect (canvas,
                        padding*2 + rect_width*1, padding,
                        rect_width, rect_height, &style);

  /* border radius 25px and rotating */
  glr_border_set_color (&(style.border), GLR_BORDER_ALL, glr_color_from_rgba (0, 128, 0, 200));
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 25.0 * zoom_factor);
  glr_background_set_color (&(style.background), glr_color_from_rgba (128, 0, 128, 200));
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 5.0 * zoom_factor);
  glr_canvas_rotate (canvas, 0.0, 0.0, 0.0 - frame / 100.0);
  glr_canvas_draw_rect (canvas,
                        padding*3 + rect_width*2, padding,
                        rect_width, rect_height, &style);
  glr_canvas_rotate (canvas, 0.0, 0.0, 0.0);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 0.0);
  glr_border_set_color (&(style.border), GLR_BORDER_ALL, GLR_COLOR_NONE);

  /* border radius 5px and scaled */
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 5.0 * zoom_factor);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 1.0 * zoom_factor);
  glr_background_set_color (&(style.background), glr_color_from_rgba (255, 255, 0, 200));
  glr_canvas_scale (canvas, (anim + M_PI/2.0) / 2.0, (anim + M_PI/2.0) / 2.0, 1.0);
  glr_canvas_draw_rect (canvas,
                        padding*4 + rect_width*3, padding,
                        rect_width, rect_height, &style);
  glr_canvas_scale (canvas, 1.0, 1.0, 1.0);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 0.0);
  glr_border_set_color (&(style.border), GLR_BORDER_ALL, GLR_COLOR_NONE);

  /* rectangles width border and background */
  /* ------------------------------------------------------------------------ */
  /* width 1.0, no border radius */
  static float angle = 0.0;
  angle += 0.02;
  glr_background_set_linear_gradient (&(style.background),
                                      angle,
                                      glr_color_from_rgba (0, 0, 255, 255),
                                      glr_color_from_rgba (0, 255, 0, 255));
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 0.0 * zoom_factor);
  glr_canvas_draw_rect (canvas,
                        padding,
                        padding*2 + rect_height*1,
                        rect_width, rect_height, &style);

  /* width 5.0, border radius 50px */
  glr_background_set_linear_gradient (&(style.background),
                                      M_PI/3.0,
                                      glr_color_from_rgba (0, 127, 0, 255),
                                      glr_color_from_rgba (255, 255, 255, 255));
  glr_border_set_color (&(style.border), GLR_BORDER_ALL, glr_color_from_rgba (255, 255, 255, 255));
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 25.0 * zoom_factor);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 7.0 * zoom_factor);
  glr_canvas_draw_rect (canvas,
                        padding*2 + rect_width*1,
                        padding*2 + rect_height*1,
                        rect_width, rect_height, &style);
  glr_border_set_color (&(style.border), GLR_BORDER_ALL, GLR_COLOR_NONE);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 0.0);

  /* width 10.0, border radius 25px and rotating */
  glr_background_set_linear_gradient (&(style.background),
                                      M_PI/2.0 + M_PI/4.0,
                                      glr_color_from_rgba (0, 0, 0, 255),
                                      glr_color_from_rgba (255, 0, 0, 255));
  glr_canvas_rotate (canvas, 0.0, 0.0, 0.0 - frame / 100.0);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 7.0 * zoom_factor);
  glr_canvas_draw_rect (canvas,
                        padding*3 + rect_width*2,
                        padding*2 + rect_height*1,
                        rect_width, rect_height, &style);

  /* width 25px, border radius 50px and scaled */
  static float angle1 = 0.0;
  angle1 += 0.05;
  glr_background_set_linear_gradient (&(style.background),
                                      -angle1,
                                      glr_color_from_rgba (128, 128, 128, 255),
                                      glr_color_from_rgba (0, 0, 0, 0));
  glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 80.0 * zoom_factor);
  glr_canvas_rotate (canvas, 0.0, 0.0, 0.0);
  glr_canvas_scale (canvas, (anim + M_PI/2.0) / 2.0, (anim + M_PI/2.0) / 2.0, 1.0);
  glr_border_set_width (&(style.border), GLR_BORDER_ALL, 0.0 * zoom_factor);
  glr_canvas_draw_rect (canvas,
                        padding*4 + rect_width*3,
                        padding*2 + rect_height*1,
                        rect_width, rect_height, &style);
  glr_canvas_scale (canvas, 1.0, 1.0, 1.0);
}

static void
draw_func (uint32_t frame, float _zoom_factor, void *user_data)
{
  zoom_factor = _zoom_factor;

  glr_canvas_clear (canvas, glr_color_from_rgba (0, 0, 0, 255));
  glr_canvas_reset_transform (canvas);

  draw_frame (frame);

  glr_canvas_reset_transform (canvas);
  glr_canvas_flush (canvas);

  // if canvas has a target, blit its framebuffer into default FBO
  if (glr_canvas_get_target (canvas) != NULL)
    {
      uint32_t target_width, target_height;
      glr_target_get_size (target, &target_width, &target_height);

      glBindFramebuffer (GL_FRAMEBUFFER, 0);
      glClearColor (1.0, 1.0, 1.0, 1.0);
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glBindFramebuffer (GL_READ_FRAMEBUFFER, glr_target_get_framebuffer (target));
      glBlitFramebuffer (0, 0,
                         target_width, target_height,
                         0, window_height - target_height,
                         target_width, window_height,
                         GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                         GL_NEAREST);
    }
}

static void
resize_func (uint32_t width, uint32_t height, void *user_data)
{
  glr_target_resize (target, width, height);
  window_width = width;
  window_height = height;

  glViewport (0, 0, width, height);
}

int
main (int argc, char* argv[])
{
  /* init windowing system */
  utils_initialize_egl (WINDOW_WIDTH, WINDOW_HEIGHT, "Backgrounds (simple)");

  /* init glr */
  context = glr_context_new ();
  target = glr_target_new (context, WINDOW_WIDTH, WINDOW_HEIGHT, MSAA_SAMPLES);
  canvas = glr_canvas_new (context, MSAA_SAMPLES > 0 ? target : NULL);

  /* start the show */
  utils_main_loop (draw_func, resize_func, NULL);

  /* clean up */
  glr_canvas_unref (canvas);
  glr_target_unref (target);
  glr_context_unref (context);

  utils_finalize_egl ();

  return 0;
}
