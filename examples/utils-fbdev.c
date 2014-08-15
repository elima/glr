#include "utils.h"

#include <EGL/egl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

extern int
nanosleep (const struct timespec *req, struct timespec *rem);

typedef struct fbdev_window
{
  unsigned short width;
  unsigned short height;
} fbdev_window;

static fbdev_window *fbdev_native_win;

static EGLDisplay egl_display;
static EGLContext egl_context;
static EGLSurface egl_surface;

static DrawCallback draw_callback = NULL;
static ResizeCallback resize_callback = NULL;
static void *callback_user_data = NULL;

#define ZOOM_FACTOR_DELTA 1.2

static float zoom_factor = 1.0;

static void
log_times_per_second (void)
{
  static struct timeval t1 = {0}, t2;
  static uint32_t num_frames = 0;

  if (t1.tv_sec == 0)
    gettimeofday (&t1, NULL);

  if (++num_frames % 100 == 0)
    {
      gettimeofday (&t2, NULL);
      float dt  =  t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6;
      printf ("FPS: %02f\n", num_frames / dt);
      num_frames = 0;
      t1 = t2;
    }
}

/* public API */

int
utils_initialize_egl (uint32_t    win_width,
                      uint32_t    win_height,
                      const char *win_title)
{
  fbdev_native_win = calloc (1, sizeof (fbdev_native_win));
  fbdev_native_win->width = win_width;
  fbdev_native_win->height = win_height;

  // EGLBoolean success = EGL_FALSE;

  // get display
  egl_display = eglGetDisplay (EGL_DEFAULT_DISPLAY);
  if (egl_display == EGL_NO_DISPLAY)
    {
      EGLint error = eglGetError ();
      printf ("eglGetError(): %i (0x%.4x)\n", (int) error, (int) error);
      printf ("No EGL Display available at %s:%i\n", __FILE__, __LINE__);
      exit (-1);
    }

  // initialize EGL
  if (eglInitialize (egl_display, NULL, NULL) != EGL_TRUE)
    {
      EGLint error = eglGetError ();
      printf ("eglGetError(): %i (0x%.4x)\n", (int) error, (int) error);
      printf ("Failed to initialize EGL at %s:%i\n", __FILE__, __LINE__);
      exit (-1);
    }

  // find and choose a config
  EGLint config_attrs[] =
    {
      EGL_ALPHA_SIZE,          8,
      EGL_RED_SIZE,            8,
      EGL_GREEN_SIZE,          8,
      EGL_BLUE_SIZE,           8,
      // EGL_BUFFER_SIZE,         32,

      // EGL_STENCIL_SIZE,        0,
      // EGL_DEPTH_SIZE,          16,

      EGL_RENDERABLE_TYPE,     EGL_OPENGL_ES2_BIT,
      EGL_SURFACE_TYPE,        EGL_WINDOW_BIT,

      EGL_NONE
    };

  EGLConfig egl_config;
  EGLint num_configs = 0;

  if (eglChooseConfig (egl_display, config_attrs, &egl_config, 1, &num_configs) != EGL_TRUE)
    {
      EGLint error = eglGetError ();
      printf ("eglGetError(): %i (0x%.4x)\n", (int) error, (int) error);
      printf ("Failed to enumerate EGL configs at %s:%i\n", __FILE__, __LINE__);
      exit (1);
    }

  printf ("EGL: num configs: %d\n", num_configs);

  if (num_configs == 0)
    {
      printf ("EGL: No suitable config found :(\n");
      exit (1);
    }

  // create a surface
  EGLint egl_window_attrs[] =
    {
      EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
      EGL_NONE
    };

  egl_surface = eglCreateWindowSurface (egl_display,
                                        egl_config,
                                         (EGLNativeWindowType) fbdev_native_win,
                                        egl_window_attrs);
  if (egl_surface == EGL_NO_SURFACE)
    {
      EGLint error = eglGetError ();
      printf ("eglGetError(): %i (0x%.4x)\n", (int) error, (int) error);
      printf ("Failed to create EGL surface at %s:%i\n", __FILE__, __LINE__);
      exit (1);
    }

  // bind to OpenGL ES API
  eglBindAPI (EGL_OPENGL_ES_API);

  // create context
  EGLint egl_context_attrs[] =
    {
      EGL_CONTEXT_CLIENT_VERSION, 3,
      EGL_NONE
    };

  egl_context = eglCreateContext (egl_display,
                                  egl_config,
                                  EGL_NO_CONTEXT,
                                  egl_context_attrs);
  if (egl_context == EGL_NO_CONTEXT)
    {
      EGLint error = eglGetError ();
      printf ("eglGetError(): %i (0x%.4x)\n", (int) error, (int) error);
      printf ("Failed to create EGL context at %s:%i\n", __FILE__, __LINE__);
      exit (1);
    }

  if (eglMakeCurrent (egl_display,
                      egl_surface,
                      egl_surface,
                      egl_context) != EGL_TRUE)
    {
      EGLint error = eglGetError ();
      printf ("eglGetError(): %i (0x%.4x)\n", (int) error, (int) error);
      printf ("Failed to make EGL context current %s:%i\n", __FILE__, __LINE__);
      exit (1);
    }

  return 0;
}

void
utils_finalize_egl (void)
{
  eglBindAPI (EGL_OPENGL_ES_API);
  eglMakeCurrent (egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_context);
  eglMakeCurrent (egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroyContext (egl_display, egl_context);
  eglDestroySurface (egl_display, egl_surface);
  eglTerminate (egl_display);
}

void
utils_main_loop (DrawCallback    draw_cb,
                 ResizeCallback  resize_cb,
                 void           *user_data)
{
  draw_callback = draw_cb;
  resize_callback = resize_cb;
  callback_user_data = user_data;

  if (resize_callback)
    resize_callback (fbdev_native_win->width,
                     fbdev_native_win->height,
                     callback_user_data);

  uint32_t frame = 0;
  int quit = 0;
  while (! quit)
    {
      frame++;

      if (draw_callback != NULL)
        draw_callback (frame, zoom_factor, callback_user_data);

      eglSwapBuffers (egl_display, egl_surface);
      log_times_per_second ();

      static float target_fps = 60.0;
      static struct timeval last_frame = { 0 };
      struct timeval t;
      gettimeofday (&t, NULL);

      if (last_frame.tv_sec > 0)
        {
          int64_t elapsed_usec = (t.tv_sec - last_frame.tv_sec) * 1000000 + (t.tv_usec - last_frame.tv_usec);
          int64_t wait_usec = (uint64_t) (round (1.0 / target_fps * 1000000.0)) - elapsed_usec;

          if (wait_usec > 0)
            {
              struct timespec wait = {0};
              wait.tv_nsec = ((long) wait_usec) * 1000;
              nanosleep (&wait, NULL);
            }
        }
      gettimeofday (&last_frame, NULL);
    }
}
