#include "../glr.h"
#include "utils.h"

#define MSAA_SAMPLES 0

#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1080

#define MAX_LAYERS 20
#define SCALE 10

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;

static uint32_t window_width, window_height;
static float zoom_factor = 1.0;

static void
draw_layer (uint32_t frame, uint32_t layer_index)
{
  GlrStyle style = GLR_STYLE_DEFAULT;
  int i, j;
  uint32_t width, height;

  glr_target_get_size (target, &width, &height);
  width *= zoom_factor;
  height *= zoom_factor;

  for (i = 0; i < SCALE; i++)
    for (j = 0; j < SCALE; j++)
      {
        int k = layer_index;

        glr_canvas_set_transform_origin (canvas, 0.0, 0.0, 0.0);
        glr_canvas_rotate (canvas,
                           0.0,
                           0.0,
                           0.0 - M_PI * frame/200.0 * ((MAX_LAYERS - k)/2.0 + 1.0));

        glr_background_set_color (&(style.background),
                                  glr_color_from_hue (layer_index * 500, 200));
        glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 5.0);

        glr_canvas_draw_rect (canvas,
                              width/SCALE * i + k*20,
                              width/SCALE * j,
                              30 + width/SCALE/(k+1),
                              30 + height/SCALE/(k+1),
                              &style);
      }
}

static void
draw_func (uint32_t frame, float _zoom_factor, void *user_data)
{
  int i;

  zoom_factor = _zoom_factor;

  glr_canvas_clear (canvas, glr_color_from_rgba (127, 127, 127, 255));
  glr_canvas_reset_transform (canvas);

  for (i = MAX_LAYERS - 1; i >= 0; i--)
    draw_layer (frame, i + 1);

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
  utils_initialize_egl (WINDOW_WIDTH, WINDOW_HEIGHT, "Layers");

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
