#include "utils.h"

#include <EGL/egl.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

/* @FIXME: check where should this be defined */
#define EGL_OPENGL_ES3_BIT_KHR 0x00000040

static Display    *x_display;
static Window      win;
static EGLDisplay  egl_display;
static EGLContext  egl_context;
static EGLSurface  egl_surface;

static DrawCallback draw_callback = NULL;
static ResizeCallback resize_callback = NULL;
static gpointer callback_user_data = NULL;

static guint width, height;

static void
log_times_per_second (void)
{
  static struct timeval t1 = {0}, t2;
  static guint num_frames = 0;

  if (t1.tv_sec == 0)
    gettimeofday (&t1, NULL);

  if (++num_frames % 100 == 0)
    {
      gettimeofday (&t2, NULL);
      gfloat dt  =  t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6;
      printf ("FPS: %02f\n", num_frames / dt);
      num_frames = 0;
      t1 = t2;
    }
}

static void
toggle_fullscreen (void)
{
  static gboolean fullscreen_mode = FALSE;

  fullscreen_mode = ! fullscreen_mode;

  Atom wm_state = XInternAtom (x_display, "_NET_WM_STATE", False);
  XEvent xev = { 0 };
  memset (&xev, 0, sizeof (xev));

  xev.type                 = ClientMessage;
  xev.xclient.window       = win;
  xev.xclient.message_type = wm_state;
  xev.xclient.format       = 32;
  xev.xclient.data.l[0]    = fullscreen_mode ? 1 : 0;

  Atom fullscreen;
  fullscreen = XInternAtom (x_display,
                            "_NET_WM_STATE_FULLSCREEN",
                            False);
  xev.xclient.data.l[1] = fullscreen;

  XSendEvent (x_display,
              DefaultRootWindow (x_display),
              False,
              SubstructureNotifyMask,
              &xev);
}

/* public API */

gint
utils_initialize_x11 (guint _width, guint _height, const gchar *win_title)
{
  width = _width;
  height = _height;

  // open the standard display (the primary screen)
  x_display = XOpenDisplay (NULL);
  if (x_display == NULL)
    {
      printf ("cannot connect to X server\n");
      return 1;
    }

  // get the root window (usually the whole screen)
  Window root = DefaultRootWindow (x_display);

  XSetWindowAttributes swa;
  swa.event_mask = ExposureMask | PointerMotionMask | KeyPressMask | StructureNotifyMask;

  // create a window with the provided parameters
  win = XCreateWindow (x_display, root,
                       0, 0, width, height,  0,
                       CopyFromParent, InputOutput,
                       CopyFromParent, CWEventMask,
                       &swa);

  XSetWindowAttributes xattr;

  xattr.override_redirect = False;
  XChangeWindowAttributes (x_display, win, CWOverrideRedirect, &xattr);

  // fullscreen by-default
  if (FALSE)
    {
      Atom atom;
      atom = XInternAtom (x_display, "_NET_WM_STATE_FULLSCREEN", True);

      XChangeProperty (x_display, win,
                       XInternAtom (x_display, "_NET_WM_STATE", True),
                       XA_ATOM,  32,  PropModeReplace,
                       (unsigned char*) &atom, 1);
    }

  XWMHints hints;
  hints.input = True;
  hints.flags = InputHint;
  XSetWMHints (x_display, win, &hints);

  // make the window visible on the screen
  XMapWindow (x_display, win);

  // give the window a name
  XStoreName (x_display, win, win_title);

  return 0;
}

void
utils_finalize_x11 (void)
{
  XDestroyWindow (x_display, win);
  XCloseDisplay (x_display);
}

gint
utils_initialize_egl (void)
{
  egl_display = eglGetDisplay ((EGLNativeDisplayType) x_display);
  if (egl_display == EGL_NO_DISPLAY)
    {
      printf ("Got no EGL display.\n");
      return 1;
    }

  if (! eglInitialize (egl_display, NULL, NULL))
    {
      printf ("Unable to initialize EGL\n");
      return 1;
    }

  // some attributes to set up our egl-interface
  EGLint attr[] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_NONE
  };

  EGLConfig ecfg;
  EGLint num_config;
  if (! eglChooseConfig (egl_display, attr, &ecfg, 1, &num_config))
    {
      printf ("Failed to choose config (eglError: %u)\n", eglGetError());
      return 1;
    }

  if (num_config != 1)
    {
      printf ("Didn't get exactly one config, but %d\n", num_config);
      return 1;
    }

  egl_surface = eglCreateWindowSurface (egl_display, ecfg, win, NULL);
  if (egl_surface == EGL_NO_SURFACE)
    {
      printf ("Unable to create EGL surface (eglError: %u)\n", eglGetError());
      return 1;
    }

  // egl-contexts collect all state descriptions needed required for operation
  EGLint ctxattr[] = {
    EGL_CONTEXT_CLIENT_VERSION, 3,
    EGL_NONE
  };
  egl_context = eglCreateContext (egl_display, ecfg, EGL_NO_CONTEXT, ctxattr);
  if (egl_context == EGL_NO_CONTEXT)
    {
      printf ("Unable to create EGL context (eglError: %u)\n", eglGetError());
      return 1;
    }

  // associate the egl-context with the egl-surface
  eglMakeCurrent (egl_display, egl_surface, egl_surface, egl_context);

  return 0;
}

void
utils_finalize_egl (void)
{
  eglDestroyContext (egl_display, egl_context);
  eglDestroySurface (egl_display, egl_surface);
  eglTerminate (egl_display);
}

void
utils_main_loop (DrawCallback   draw_cb,
                 ResizeCallback resize_cb,
                 gpointer       user_data)
{
  draw_callback = draw_cb;
  resize_callback = resize_cb;
  callback_user_data = user_data;

  guint frame = 0;
  int quit = 0;
  while (! quit)
    {
      frame++;

      if (draw_callback != NULL)
        draw_callback (frame, callback_user_data);

      eglSwapBuffers (egl_display, egl_surface);
      log_times_per_second ();

      // check for events from the x-server
      while (XPending (x_display))
        {
          XEvent xev;

          XNextEvent (x_display, &xev);
          // g_print ("%d\n", xev.xkey.keycode);

          if (xev.type == KeyPress)
            {
              // 'f' to toggle fullscreen
              if (xev.xkey.keycode == 41)
                {
                  toggle_fullscreen ();
                }
              // ESC to quit
              else if (xev.xkey.keycode == 9)
                {
                  quit = 1;
                }
            }
          else if (xev.type == ConfigureNotify)
            {
              XConfigureEvent xce = xev.xconfigure;

              if (xce.width != width || xce.height != height)
                {
                  width = xce.width;
                  height = xce.height;
                  if (resize_callback != NULL)
                    resize_callback (width, height, callback_user_data);
                }
            }
        }
    }
}
