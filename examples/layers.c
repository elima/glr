#include <GL/glfw.h>
#include <glib.h>
#include <sys/time.h>
#include <math.h>

#include "../glr.h"

#define MSAA_SAMPLES 8
#define WIDTH   960
#define HEIGHT  960

#define MAX_LAYERS 20
#define SCALE 10

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;
static GlrLayer *layers[MAX_LAYERS];

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
draw_layer (GlrLayer  *layer,
            guint64    frame,
            guint      layer_index)
{
  GlrPaint paint;
  gint i, j;

  for (i = 0; i < SCALE; i++)
    for (j = 0; j < SCALE; j++)
      {
        gint k = layer_index;

        glr_paint_set_color_hue (&paint, layer_index * 500, 200);
        glr_paint_set_style (&paint, GLR_PAINT_STYLE_FILL);
        glr_layer_set_transform_origin (layer, 0.0, 0.0);
        glr_layer_rotate (layer, 0.0 - (frame % 1080) * ((MAX_LAYERS - k)/2.0 + 1.0));
        glr_layer_draw_rounded_rect (layer,
                                     WIDTH/SCALE * i + k*20,
                                     WIDTH/SCALE * j,
                                     30 + WIDTH/SCALE/(k+1),
                                     30 + HEIGHT/SCALE/(k+1),
                                     5,
                                     &paint);
      }

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

  glfwSetWindowTitle ("Layers - glr");

  context = glr_context_new ();
  target = glr_target_new (WIDTH, HEIGHT, MSAA_SAMPLES);
  canvas = glr_canvas_new (target);

  glEnable (GL_BLEND);

  /* start the show */
  guint frame = 0;
  gint i;
  GlrPaint paint;

  do
    {
      frame++;

      /* notify canvas that you want to start drawing for a new frame */
      glr_canvas_start_frame (canvas);

      glr_paint_set_color (&paint, 127, 127, 127, 255);
      glr_canvas_clear (canvas, &paint);

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

      /* blit canvas' MSAA tex into default FBO */
      glBindFramebuffer (GL_FRAMEBUFFER, 0);
      glBindFramebuffer (GL_READ_FRAMEBUFFER, glr_target_get_framebuffer (target));
      glBlitFramebuffer (0, 0, WIDTH, HEIGHT,
                         0, 0, WIDTH, HEIGHT,
                         GL_COLOR_BUFFER_BIT,
                         GL_NEAREST);

      log_times_per_second ("FPS: %04f\n");
      glfwSwapBuffers ();

      /* destroy layers */
      for (i = 0; i < MAX_LAYERS; i++)
        {
          glr_layer_unref (layers[i]);
        }
    }
  while (glfwGetKey (GLFW_KEY_ESC) != GLFW_PRESS && glfwGetWindowParam (GLFW_OPENED));

  glr_canvas_unref (canvas);
  glr_target_unref (target);
  glr_context_unref (context);

  glfwTerminate ();
  return 0;
}
