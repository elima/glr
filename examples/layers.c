#include <glib.h>
#include <sys/time.h>

#include "../glr.h"
#include "utils.h"

#define MSAA_SAMPLES 0

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT  960

#define MAX_LAYERS 20
#define SCALE 20

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;
static GlrLayer *layers[MAX_LAYERS];

static guint window_width, window_height;
static gfloat zoom_factor = 1.0;

static void
draw_layer (GlrLayer  *layer,
            guint64    frame,
            guint      layer_index)
{
  GlrStyle style = GLR_STYLE_DEFAULT;
  gint i, j;
  guint width, height;

  glr_target_get_size (target, &width, &height);
  width *= zoom_factor;
  height *= zoom_factor;

  for (i = 0; i < SCALE; i++)
    for (j = 0; j < SCALE; j++)
      {
        gint k = layer_index;

        glr_layer_set_transform_origin (layer, 0.0, 0.0);
        glr_layer_rotate (layer, 0.0 - (frame % 1080) * ((MAX_LAYERS - k)/2.0 + 1.0));

        glr_background_set_color (&(style.background),
                                  glr_color_from_hue (layer_index * 500, 200));
        glr_border_set_radius (&(style.border), GLR_BORDER_ALL, 5.0);

        glr_layer_draw_rect (layer,
                             width/SCALE * i + k*20,
                             width/SCALE * j,
                             30 + width/SCALE/(k+1),
                             30 + height/SCALE/(k+1),
                             &style);
      }

  glr_layer_finish (layer);
}

static void
draw_func (guint frame, gfloat _zoom_factor, gpointer user_data)
{
  gint i;

  zoom_factor = _zoom_factor;

  /* notify canvas that you want to start drawing for a new frame */
  glr_canvas_start_frame (canvas);

  glr_canvas_clear (canvas, glr_color_from_rgba (127, 127, 127, 255));

  /* create and attach layers */
  for (i = MAX_LAYERS - 1; i >= 0; i--)
    {
      layers[i] = glr_layer_new (context);
      glr_canvas_attach_layer (canvas, i, layers[i]);

      draw_layer (layers[i], frame, i + 1);
    }

  /* finally, sync all layers drawings and execute
     all layers' batched commands */
  glr_canvas_finish_frame (canvas);

  /* destroy layers */
  for (i = 0; i < MAX_LAYERS; i++)
    {
      glr_layer_unref (layers[i]);
    }

  /* blit target's MSAA tex into default FBO */
  guint current_width, current_height;
  glr_target_get_size (target, &current_width, &current_height);
  glBindFramebuffer (GL_FRAMEBUFFER, 0);
  glBindFramebuffer (GL_READ_FRAMEBUFFER, glr_target_get_framebuffer (target));
  glBlitFramebuffer (0, 0,
                     current_width,
                     current_height,
                     0,
                     window_height - current_height,
                     current_width,
                     current_height,
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
  utils_initialize_x11 (WINDOW_WIDTH, WINDOW_HEIGHT, "Layers");
  utils_initialize_egl ();

  /* init glr */
  context = glr_context_new ();
  target = glr_target_new (context, WINDOW_WIDTH, WINDOW_HEIGHT, MSAA_SAMPLES);
  canvas = glr_canvas_new (context, target);

  /* start the show */
  utils_main_loop (draw_func, resize_func, NULL);

  /* clean up */
  glr_canvas_unref (canvas);
  glr_target_unref (target);
  glr_context_unref (context);

  utils_finalize_egl ();
  utils_finalize_x11 ();

  return 0;
}
