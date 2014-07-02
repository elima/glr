#include <GL/glfw.h>
#include <glib.h>
#include <sys/time.h>
#include <math.h>

#include "../glr.h"

#define MSAA_SAMPLES 8
#define WIDTH   960
#define HEIGHT  960

#define SCALE 120

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;
static GlrLayer *layer1 = NULL;
static GlrLayer *layer2 = NULL;

static void
log_times_per_second (const char *msg_format)
{
  static unsigned int counter = 0;
  static time_t last_time = 0;
  static int interval = 2;
  time_t cur_time;

  if (last_time == 0)
    last_time = time (NULL);

  counter++;

  cur_time = time (NULL);
  if (cur_time >= last_time + interval)
    {
      g_print (msg_format, (double) (counter / interval));
      counter = 0;
      last_time = cur_time;
    }
}

static void
draw_layer_in_thread (GlrLayer *layer, gpointer user_data)
{
  gint i, j;
  gint scale = SCALE / 2;
  GlrPaint paint;
  guint frame = * (guint *) user_data;

  for (i = 0; i < scale; i++)
    for (j = 0; j < scale; j++)
      {
        gdouble rotation = ((frame + i + j) / 25.0 * 180.0);
        guint scaling = (layer == layer1) ? 60 : 75;

        glr_layer_set_transform_origin (layer, 0.0, 0.0);
        glr_layer_rotate (layer, layer == layer1 ? rotation : -rotation);
        glr_paint_set_color_hue (&paint, frame + i + j + ((i + j %2) * 3), 255);
        glr_layer_scale (layer,
                         cos ((M_PI/scaling) * ((frame + i + j) % scaling)) + 1,
                         cos ((M_PI/scaling) * ((frame + i + j) % scaling)) + 1);

        glr_paint_set_style (&paint,
                             layer == layer1 ?
                             GLR_PAINT_STYLE_FILL : GLR_PAINT_STYLE_STROKE);
        glr_layer_draw_rect (layer,
                             i * WIDTH/scale + (layer == layer1 ? WIDTH/scale/2 : 0),
                             j * HEIGHT/scale,
                             WIDTH/scale/2,
                             HEIGHT/scale/2,
                             &paint);
      }

  /* it is necessary to always call finish() on on a layer, otherwise
     glr_canvas_finish_frame() will wait forever */
  glr_layer_finish (layer);
}

gint
main (int argc, char* argv[])
{
  if (glfwInit () == GL_FALSE)
    {
      g_printerr ("Could not initialize GLFW. Aborting.\n" );
      return -1;
    }

  if (glfwOpenWindow (WIDTH, HEIGHT, 8, 8, 8, 8, 24, 8, GLFW_WINDOW) == GL_FALSE)
    {
      g_printerr ("Could not open GLFW window. Aborting.\n");
      return -2;
    }

  glfwSetWindowTitle ("Rects - glr");

  context = glr_context_new ();
  target = glr_target_new (WIDTH, HEIGHT, MSAA_SAMPLES);
  canvas = glr_canvas_new (target);
  layer1 = glr_layer_new (context);
  layer2 = glr_layer_new (context);

  glEnable (GL_BLEND);
  glLineWidth (1.0);

  /* start the show */
  guint frame = 0;
  GlrPaint paint;

  do
    {
      frame++;

      glr_canvas_start_frame (canvas);

      glr_paint_set_color (&paint, 255, 255, 255, 255);
      glr_canvas_clear (canvas, &paint);

      /* attach our two layers */
      glr_canvas_attach_layer (canvas, 1, layer1);
      glr_canvas_attach_layer (canvas, 0, layer2);

      /* invalidate layer1 and draw it in a thread*/
      glr_layer_redraw_in_thread (layer1, draw_layer_in_thread, &frame);

      /* invalidate layer2 and draw it in a thread */
      glr_layer_redraw_in_thread (layer2, draw_layer_in_thread, &frame);

      /* finally, sync all layers and render */
      glr_canvas_finish_frame (canvas);

      /* blit target's MSAA tex into default FBO */
      glBindFramebuffer (GL_FRAMEBUFFER, 0);
      glBindFramebuffer (GL_READ_FRAMEBUFFER, glr_target_get_framebuffer (target));
      glBlitFramebuffer (0, 0, WIDTH, HEIGHT,
                         0, 0, WIDTH, HEIGHT,
                         GL_COLOR_BUFFER_BIT,
                         GL_NEAREST);

      log_times_per_second ("FPS: %04f\n");
      glfwSwapBuffers ();
    }
  while (glfwGetKey (GLFW_KEY_ESC) != GLFW_PRESS && glfwGetWindowParam (GLFW_OPENED));

  glr_layer_unref (layer1);
  glr_layer_unref (layer2);
  glr_canvas_unref (canvas);
  glr_target_unref (target);
  glr_context_unref (context);

  glfwTerminate ();
  return 0;
}
