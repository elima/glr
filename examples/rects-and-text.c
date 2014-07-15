#include <glib.h>
#include <sys/time.h>
#include <math.h>

#include "../glr.h"
#include "utils.h"

#define MSAA_SAMPLES 0

#define WINDOW_WIDTH    1280
#define WINDOW_HEIGHT    960

#define FONT_FILE  (FONTS_DIR "Cantarell-Regular.otf")
// #define FONT_FILE  (FONTS_DIR "DejaVuSansMono-Bold.ttf")
// #define FONT_FILE  (FONTS_DIR "Ubuntu-Title.ttf")

#define FONT_SIZE 6
#define SCALE 51

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;
static GlrLayer *layer = NULL;

static guint window_width, window_height;

static void
draw_layer_in_thread (GlrLayer *layer, gpointer user_data)
{
  gint i, j;
  gint scale = SCALE;
  GlrStyle style = GLR_STYLE_DEFAULT;
  GlrFont font = {0};
  guint width, height;
  guint frame = * (guint *) user_data;

  font.face = FONT_FILE;
  font.face_index = 0;

  glr_target_get_size (target, &width, &height);

  gfloat offset_y = sin ((M_PI*2.0/120.0) * (frame % 120)) * 150.0;
  glr_layer_translate (layer, 0, offset_y);

  for (i = 0; i < scale; i++)
    for (j = 0; j < scale; j++)
      {
        gint step = (j * scale + i + (j % 2)) % 2;
        if (step == 1)
          {
            glr_layer_set_transform_origin (layer, 0.5, 0.5);
            glr_layer_rotate (layer, - ((frame*2 + i + j) / 50.0 * 180.0));
            glr_background_set_color (&(style.background),
                                      glr_color_from_hue (frame + i + j + ((i + j % 2) * 3), 100));
            glr_layer_scale (layer,
                             cos ((M_PI/60.0) * ((frame + i + j/(i+1)) % 60))*2 + 1.0,
                             cos ((M_PI/60.0) * ((frame + i + j/(i+1)) % 60))*2 + 1.0);
            glr_layer_draw_rect (layer,
                                 i * width/scale, j * height/scale,
                                 width/scale, height/scale,
                                 &style);
          }
        else
          {
            glr_layer_set_transform_origin (layer, 0.5, 0.5);
            glr_layer_scale (layer, 1.0, 1.0);
            glr_layer_rotate (layer, (frame*2 + i + j) / 150.0 * 180.0);
            font.size = i % 4 == 0 ? FONT_SIZE + (i) * 1 : FONT_SIZE + scale - i;
            glr_layer_draw_char (layer,
                                 j % scale + 48,
                                 i * width/scale - (j%2)*(width/scale),
                                 j * height/scale,
                                 &font,
                                 glr_color_from_rgba (64, 64, 64, 255));
          }
      }

  /* it is necessary to always call finish() on on a layer, otherwise
     glr_canvas_finish_frame() will wait forever */
  glr_layer_finish (layer);
}

static void
draw_func (guint frame, gpointer user_data)
{
  glClearColor (1.0, 1.0, 1.0, 1.0);
  glClear (GL_COLOR_BUFFER_BIT);

  glr_canvas_start_frame (canvas);
  glr_canvas_clear (canvas, glr_color_from_rgba (255, 255, 255, 255));

  /* attach our layer */
  glr_canvas_attach_layer (canvas, 0, layer);

  /* invalidate layer and draw it */
  glr_layer_redraw_in_thread (layer, draw_layer_in_thread, &frame);
  /*
  glr_layer_redraw (layer);
  draw_layer_in_thread (layer, &frame);
  */

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
                     current_width, window_height,
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
  utils_initialize_x11 (WINDOW_WIDTH, WINDOW_HEIGHT, "RectsAndText");
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
