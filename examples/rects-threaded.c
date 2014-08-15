#include <math.h>

#include "../glr.h"
#include "utils.h"

#define MSAA_SAMPLES 0

#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1080

#define SCALE 80

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;

static uint32_t window_width, window_height;
static float zoom_factor = 1.0;

static void
draw_frame (uint32_t frame)
{
  int i, j;
  int scale = SCALE / 2;
  uint32_t width, height;

  glr_target_get_size (target, &width, &height);
  width *= zoom_factor;
  height *= zoom_factor;

  for (i = 0; i < scale; i++)
    for (j = 0; j < scale; j++)
      {
        GlrStyle style = GLR_STYLE_DEFAULT;
        GlrColor color;
        float rotation = (frame + i + j) / 10.0;
        uint8_t step = (i * scale + j) % 2;
        uint32_t scaling = step == 0 ? 75 : 60;

        glr_canvas_set_transform_origin (canvas,
                                         -(width/scale/2.0),
                                         height/scale/2.0,
                                         0.0);
        glr_canvas_rotate (canvas, 0.0, 0.0, rotation);
        glr_canvas_scale (canvas,
                          cos ((M_PI/scaling) * ((frame + i + j) % scaling)) + 1,
                          cos ((M_PI/scaling) * ((frame + i + j) % scaling)) + 1,
                          1.0);

        color = glr_color_from_hue (frame + i + j + ((i + j %2) * 3), 255);
        if (step == 0)
          {
            glr_background_set_color (&(style.background), color);
          }
        else
          {
            glr_border_set_width (&(style.border),
                                  GLR_BORDER_ALL, 1.0 * zoom_factor);
            glr_border_set_color (&(style.border), GLR_BORDER_ALL, color);
          }

        glr_canvas_draw_rect (canvas,
                              i * width/scale + (step == 1 ? width/scale/2.0 : 0.0),
                              j * height/scale + (step == 1 ? height/scale/2.0 : 0.0),
                              width/scale/2.0,
                              height/scale/2.0,
                              &style);
      }

  glr_canvas_reset_transform (canvas);
  glr_canvas_rotate (canvas,
                     (M_PI*2.0 / 360.0) * (frame % 360),
                     0.0,
                     (M_PI*2.0 / 360.0) * (frame % 360));
  glr_canvas_set_transform_origin (canvas, width/2.0, height/2.0, 0.0);
}

static void
draw_func (uint32_t frame, float _zoom_factor, void *user_data)
{
  zoom_factor = _zoom_factor;

  glr_canvas_clear (canvas, glr_color_from_rgba (0, 0, 0, 255));
  glr_canvas_reset_transform (canvas);

  draw_frame (frame);

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
main (int argc, char *argv[])
{
  /* init windowing system */
  utils_initialize_egl (WINDOW_WIDTH, WINDOW_HEIGHT, "Rects (threaded)");

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
