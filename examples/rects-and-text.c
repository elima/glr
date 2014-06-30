#include <GL/glfw.h>
#include <glib.h>
#include <sys/time.h>
#include <math.h>

#include "../glr.h"

#define MSAA_SAMPLES 8
#define WIDTH  1920
#define HEIGHT  800

#define FONT_FILE  (FONTS_DIR "Cantarell-Regular.otf")
// #define FONT_PATH "./fonts/DejaVuSansMono-Bold.ttf"
// #define FONT_PATH "./fonts/Ubuntu-Title.ttf"

#define FONT_SIZE 24
#define SCALE 40

static GlrContext *context = NULL;
static GlrTarget *target = NULL;
static GlrCanvas *canvas = NULL;
static GlrLayer *layer = NULL;

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
            gpointer   user_data)
{
  gint i, j;
  gint scale = SCALE;
  GlrPaint paint;
  GlrFont font = {0};

  glr_paint_set_color (&paint, 0, 0, 0, 255);
  glr_canvas_clear (canvas, &paint);

  glr_layer_translate (layer, 0.0, 0.0);

  font.face = FONT_FILE;
  font.face_index = 0;

  for (i = 0; i < scale; i++)
    for (j = 0; j < scale; j++)
      {
        glr_layer_set_transform_origin (layer, 0.5, 0.5);
        glr_layer_rotate (layer, - ((frame*2 + i + j) / 50.0 * 180.0));
        glr_paint_set_color_hue (&paint, frame + i + j + ((i + j %2) * 3), 100);
        glr_layer_scale (layer,
                         cos ((M_PI/60.0) * ((frame + i + j/(i+1)) % 60))*2 + 1.0,
                         cos ((M_PI/60.0) * ((frame + i + j/(i+1)) % 60))*2 + 1.0);
        gint step = (j * scale + i + (j % 2)) % 2;
        if (step == 1)
          {
            glr_paint_set_style (&paint, GLR_PAINT_STYLE_FILL);
            glr_layer_draw_rect (layer,
                                 i * WIDTH/scale, j * HEIGHT/scale,
                                 WIDTH/scale, HEIGHT/scale,
                                 &paint);
          }
        else
          {
            glr_layer_set_transform_origin (layer, +2.5, 0.5);
            glr_layer_scale (layer, 1.0, 1.0);
            glr_paint_set_color_hue (&paint, frame + i + j + ((i + j % 2) * 4), 255);
            // glr_paint_set_color (&paint, 255, 255, 255, 255);
            glr_layer_rotate (layer, (frame*2 + i + j) / 75.0 * 180.0);
            font.size = FONT_SIZE;
            glr_layer_draw_char (layer,
                                 (i) % scale + 46,
                                 i * WIDTH/scale - (j%3)*(font.size/2), j * HEIGHT/scale,
                                 &font,
                                 &paint);
          }
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

  glfwSetWindowTitle (argv[0]);

  context = glr_context_new ();
  target = glr_target_new (WIDTH, HEIGHT, MSAA_SAMPLES);
  canvas = glr_canvas_new (target);
  layer = glr_layer_new (context);

  glEnable (GL_BLEND);

  /* start the show */
  guint frame = 0;

  do
    {
      frame++;

      glr_canvas_start_frame (canvas);

      /* attach our layer */
      glr_canvas_attach_layer (canvas, 0, layer);

      /* invalidate layer and draw it */
      glr_layer_redraw (layer);
      draw_layer (layer, frame, NULL);

      /* finally, sync all layers and render */
      glr_canvas_finish_frame (canvas);

      /* blit target's MSAA tex into default FBO */
      glBindFramebuffer (GL_FRAMEBUFFER, 0);
      glBindFramebuffer (GL_READ_FRAMEBUFFER, glr_target_get_framebuffer (target));
      glBlitFramebuffer (0, 0, WIDTH, HEIGHT,
                         0, 0, WIDTH, HEIGHT,
                         GL_COLOR_BUFFER_BIT,
                         GL_NEAREST);

      log_times_per_second ("FPS: %f\n");
      glfwSwapBuffers ();
    }
  while (glfwGetKey (GLFW_KEY_ESC) != GLFW_PRESS && glfwGetWindowParam (GLFW_OPENED));

  glr_layer_unref (layer);
  glr_canvas_unref (canvas);
  glr_target_unref (target);
  glr_context_unref (context);

  glfwTerminate ();
  return 0;
}
