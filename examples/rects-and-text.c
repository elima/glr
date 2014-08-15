#include <sys/time.h>
#include <math.h>

#include "../glr.h"
#include "utils.h"

#define MSAA_SAMPLES 0

#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1080

// #define FONT_FILE  (FONTS_DIR "Cantarell-Regular.otf")
// #define FONT_FILE  (FONTS_DIR "DejaVuSansMono-Bold.ttf")
#define FONT_FILE  (FONTS_DIR "Ubuntu-Title.ttf")

#define FONT_SIZE 24
#define SCALE 31

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;

static uint32_t window_width, window_height;
float zoom_factor = 1.0;

static void
draw_frame (uint32_t frame)
{
  int i, j;
  int scale = SCALE;
  GlrStyle style = GLR_STYLE_DEFAULT;
  GlrFont font = {0};
  uint32_t width, height;

  font.face = FONT_FILE;
  font.face_index = 0;

  glr_target_get_size (target, &width, &height);
  // width *= zoom_factor;
  // height *= zoom_factor;

  for (i = 0; i < scale; i++)
    for (j = 0; j < scale; j++)
      {
        int step = (j * scale + i + (j % 2)) % 2;
        if (step == 1)
          {
            glr_canvas_set_transform_origin (canvas,
                                             (width/scale/2.0),
                                             height/scale/2.0,
                                             -50.0);
            glr_canvas_rotate (canvas,
                               0.0,
                               ((frame*0.5 + i + j) / 1500.0 * 180.0),
                               ((frame*0.5 + i + j) / 1500.0 * 180.0));
            glr_canvas_scale (canvas,
                              cos ((M_PI/60.0) * ((frame + i + j/(i+1)) % 60))*2 + 1.0,
                              cos ((M_PI/60.0) * ((frame + i + j/(i+1)) % 60))*2 + 1.0,
                              1.0);

            glr_background_set_color (&(style.background),
                                      glr_color_from_hue (frame + i + j + ((i + j % 2) * 3), 50));

            glr_canvas_draw_rect (canvas,
                                  i * width/scale, j * height/scale,
                                  width/scale, height/scale,
                                  &style);
          }
        else
          {
            glr_canvas_set_transform_origin (canvas, 0.0, 0.0, -50.0);
            glr_canvas_scale (canvas, 1.0, 1.0, 1.0);
            glr_canvas_rotate (canvas,
                               frame / 100.0,
                               (frame*0.5 + i + j) / 1500.0 * 180.0,
                               (frame*0.5 + i + j) / 1500.0 * 180.0);

            font.size = i % 4 == 0 ? FONT_SIZE + (i) * 1 : FONT_SIZE + scale - i;
            font.size *= zoom_factor;

            glr_canvas_draw_char_unicode (canvas,
                                          j % scale + 48,
                                          i * width/scale - (j%2)*(width/scale),
                                          j * height/scale,
                                          &font,
                                          glr_color_from_rgba (255, 255, 255, 255));
          }
      }

  glr_canvas_reset_transform (canvas);
  glr_canvas_set_transform_origin (canvas, width / 2.0, height / 2.0, 0.0);

  // float offset_y = sin ((M_PI*2.0/120.0) * (frame % 120)) * 150.0;
  // glr_canvas_translate (canvas, 0.0, offset_y, 0.0);

  glr_canvas_rotate (canvas, frame / 150.0, frame / 200.0, frame / 175.0);
}

static void
draw_func (uint32_t frame, float _zoom_factor, void *user_data)
{
  zoom_factor = _zoom_factor;

  glr_canvas_clear (canvas, glr_color_from_rgba (0, 0, 0, 255));

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
main (int argc, char* argv[])
{
  /* init windowing system */
  utils_initialize_egl (WINDOW_WIDTH, WINDOW_HEIGHT, "Rects and text");

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
