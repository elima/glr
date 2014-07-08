#include <glib.h>
#include <sys/time.h>
#include <math.h>

#include "../glr.h"
#include "utils.h"

#define MSAA_SAMPLES 8
#define WIDTH   960
#define HEIGHT  960

#define FONT_FILE  (FONTS_DIR "Cantarell-Regular.otf")
// #define FONT_FILE  (FONTS_DIR "DejaVuSansMono-Bold.ttf")
// #define FONT_FILE  (FONTS_DIR "Ubuntu-Title.ttf")

#define FONT_SIZE 24
#define SCALE 60

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;
static GlrLayer *layer = NULL;

static void
draw_layer (GlrLayer  *layer,
            guint64    frame,
            gpointer   user_data)
{
  gint i, j;
  gint scale = SCALE;
  GlrPaint paint;
  GlrFont font = {0};
  guint width, height;

  font.face = FONT_FILE;
  font.face_index = 0;

  glr_target_get_size (target, &width, &height);

  for (i = 0; i < scale; i++)
    for (j = 0; j < scale; j++)
      {
        glr_layer_set_transform_origin (layer, 0.0, 0.0);
        glr_layer_rotate (layer, - ((frame*2 + i + j) / 50.0 * 180.0));
        glr_paint_set_color_hue (&paint, frame + i + j + ((i + j % 2) * 3), 100);
        glr_layer_scale (layer,
                         cos ((M_PI/60.0) * ((frame + i + j/(i+1)) % 60))*2 + 1.0,
                         cos ((M_PI/60.0) * ((frame + i + j/(i+1)) % 60))*2 + 1.0);
        gint step = (j * scale + i + (j % 2)) % 2;
        if (step == 1)
          {
            glr_paint_set_style (&paint, GLR_PAINT_STYLE_FILL);
            glr_layer_draw_rect (layer,
                                 i * width/scale, j * height/scale,
                                 width/scale, height/scale,
                                 &paint);
          }
        else
          {
            glr_layer_set_transform_origin (layer, 0.0, 0.0);
            glr_layer_scale (layer, 1.0, 1.0);
            glr_paint_set_color_hue (&paint, frame + i + j + ((i + j % 2) * 4), 255);
            glr_layer_rotate (layer, (frame*2 + i + j) / 75.0 * 180.0);
            font.size = FONT_SIZE;
            glr_layer_draw_char (layer,
                                 i % scale + 46,
                                 i * width/scale - (j%3)*(font.size/2), j * height/scale,
                                 &font,
                                 &paint);
          }
      }

  /* it is necessary to always call finish() on on a layer, otherwise
     glr_canvas_finish_frame() will wait forever */
  glr_layer_finish (layer);
}

static void
draw_func (guint frame, gpointer user_data)
{
  GlrPaint paint;

  glr_canvas_start_frame (canvas);

  glr_paint_set_color (&paint, 0, 0, 0, 255);
  glr_canvas_clear (canvas, &paint);

  /* attach our layer */
  glr_canvas_attach_layer (canvas, 0, layer);

  /* invalidate layer and draw it */
  glr_layer_redraw (layer);
  draw_layer (layer, frame, NULL);

  /* finally, sync all layers and render */
  glr_canvas_finish_frame (canvas);

  /* blit target's MSAA tex into default FBO */
  guint current_width, current_height;
  glr_target_get_size (target, &current_width, &current_height);
  glBindFramebuffer (GL_FRAMEBUFFER, 0);
  glBindFramebuffer (GL_READ_FRAMEBUFFER, glr_target_get_framebuffer (target));
  glBlitFramebuffer (0, 0, current_width, current_height,
                     0, 0, current_width, current_height,
                     GL_COLOR_BUFFER_BIT,
                     GL_NEAREST);
}

static void
resize_func (guint width, guint height, gpointer user_data)
{
  glr_target_resize (target, width, height);
}

gint
main (int argc, char* argv[])
{
  /* init windowing system */
  utils_initialize_x11 (WIDTH, HEIGHT, "RectsAndText");
  utils_initialize_egl (MSAA_SAMPLES);

  /* init glr */
  context = glr_context_new ();
  target = glr_target_new (WIDTH, HEIGHT, MSAA_SAMPLES);
  canvas = glr_canvas_new (target);
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
