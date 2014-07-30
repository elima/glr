#include <glib.h>
#include <math.h>
#include <stdio.h>

#include "../glr.h"
#include "utils.h"

#define MSAA_SAMPLES 0

#define WINDOW_WIDTH    1280
#define WINDOW_HEIGHT    960

#define SCALE 160

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;
static GlrLayer *layer1 = NULL;
static GlrLayer *layer2 = NULL;

static guint window_width, window_height;
static gfloat zoom_factor = 1.0;

static void
draw_layer_in_thread (GlrLayer *layer, gpointer user_data)
{
  gint i, j;
  gint scale = SCALE / 2;
  guint frame = * (guint *) user_data;
  guint width, height;

  glr_target_get_size (target, &width, &height);
  width *= zoom_factor;
  height *= zoom_factor;

  for (i = 0; i < scale; i++)
    for (j = 0; j < scale; j++)
      {
        GlrStyle style = GLR_STYLE_DEFAULT;
        GlrColor color;
        gdouble rotation = ((frame + i + j) / 50.0 * 180.0);
        guint scaling = (layer == layer1) ? 60 : 75;

        glr_layer_set_transform_origin (layer, -1.5, 0.5);
        glr_layer_rotate (layer, layer == layer1 ? rotation : -rotation);
        glr_layer_scale (layer,
                         cos ((M_PI/scaling) * ((frame + i + j) % scaling)) + 1,
                         cos ((M_PI/scaling) * ((frame + i + j) % scaling)) + 1);
        color = glr_color_from_hue (frame + i + j + ((i + j %2) * 3), 255);

        if (layer == layer1)
          {
            glr_background_set_color (&(style.background), color);
          }
        else
          {
            glr_border_set_width (&(style.border), GLR_BORDER_ALL, 1.0);
            glr_border_set_color (&(style.border), GLR_BORDER_ALL, color);
          }

        glr_layer_draw_rect (layer,
                             i * width/scale + (layer == layer1 ? width/scale/2 : 0),
                             j * height/scale,
                             width/scale/2,
                             height/scale/2,
                             &style);
      }

  /* it is necessary to always call finish() on on a layer, otherwise
     glr_canvas_finish_frame() will wait forever */
  glr_layer_finish (layer);
}

static void
draw_func (guint frame, gfloat _zoom_factor, gpointer user_data)
{
  zoom_factor = _zoom_factor;

  glr_canvas_start_frame (canvas);
  glr_canvas_clear (canvas, glr_color_from_rgba (255, 255, 255, 255));

  /* attach our two layers */
  glr_canvas_attach_layer (canvas, 0, layer1);
  glr_canvas_attach_layer (canvas, 0, layer2);

  /* invalidate layer1 and draw it in a thread*/
  glr_layer_redraw_in_thread (layer1, draw_layer_in_thread, &frame);

  /* invalidate layer2 and draw it in a thread*/
  glr_layer_redraw_in_thread (layer2, draw_layer_in_thread, &frame);

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

int
main (int argc, char *argv[])
{
  window_width = WINDOW_WIDTH;
  window_height = WINDOW_HEIGHT;

  /* init windowing system */
  utils_initialize_x11 (WINDOW_WIDTH, WINDOW_HEIGHT, "Rects");
  utils_initialize_egl ();

  /* init OpenGL */
  glLineWidth (1.0);

  /* init glr */
  context = glr_context_new ();
  target = glr_target_new (context, WINDOW_WIDTH, WINDOW_HEIGHT, MSAA_SAMPLES);
  canvas = glr_canvas_new (context, target);
  layer1 = glr_layer_new (context);
  layer2 = glr_layer_new (context);

  /* start the show */
  utils_main_loop (draw_func, resize_func, NULL);

  /* clean up */
  glr_layer_unref (layer1);
  glr_layer_unref (layer2);
  glr_canvas_unref (canvas);
  glr_target_unref (target);
  glr_context_unref (context);

  utils_finalize_egl ();
  utils_finalize_x11 ();

  return 0;
}
