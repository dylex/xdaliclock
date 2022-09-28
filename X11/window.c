/* xdaliclock, Copyright © 1991-2022 Jamie Zawinski.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

/* Wishlist:

   - Bifurcated transparency.  On macOS, we can render the text color with 100%
     opacity and background with less, down to 0, meaning the digits are fully
     opaque while the background is partially transparent.  It looks great.
     This is also probably impossible with GTK.  Setting alpha on glClearColor
     has no effect.

     The Xlib version used the SHAPE extension, but that was a binary mask,
     not alpha blending, and also it was unusably slow.

   - A date/time entry widget for the countdown field.  GTK doesn't have one.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "window.h"
#include "prefs.h"
#include "hsv.h"

#include <X11/Xlib.h>

#ifdef DO_SAVER
# undef GDK_MULTIHEAD_SAFE
# include <gdk/gdkx.h>
#endif


/* These aren't settable through the GUI, so no point in putting them
   in GSettings, I guess...
 */
#define MAX_FPS     30
#define AUTO_DATE   67
#define HOMESTRETCH 30


#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#ifdef DO_SAVER
# ifdef HAVE_EGL
#  include <EGL/egl.h>
#  include <EGL/eglext.h>
# else
#  include <GL/glx.h>
# endif
#endif

#define DO_DOUBLE_BUFFER

struct _XDaliClockWindow {
  GtkApplicationWindow parent;
};

typedef struct XDaliClockWindowPrivate XDaliClockWindowPrivate;

struct XDaliClockWindowPrivate {
  XDaliClock *app;
  GSettings *settings;
  dali_config *config;
  int prev_time_mode;
  time_t prev_countdown;
  double prev_opacity;
  gboolean prev_hide_titlebar_p;
  int glarea_width, glarea_height;
  gboolean drag_p;

  guint clock_timer;
  guint color_timer;
  guint date_timer;
  guint date_off_timer;
  guint countdown_home_stretch_start_timer;
  guint countdown_home_stretch_end_timer;
  rgba fg, bg;
  rgba orig_fg, orig_bg;

  GtkGLArea *glarea;
  GLuint textures[2];

# ifdef DO_SAVER
  Display *dpy;
  Window window_id;

#  ifdef HAVE_EGL
  EGLDisplay egl_display;
  EGLSurface egl_surface;
  EGLContext egl_context;
  EGLConfig  egl_config;
#  else
  GLXContext glx_context;
#  endif

# endif // DO_SAVER
};

G_DEFINE_TYPE_WITH_PRIVATE (XDaliClockWindow, xdaliclock_app_window,
                            GTK_TYPE_APPLICATION_WINDOW)


static void
log_gl_error (const char *type, GLenum err)
{
  char buf[100];
  const char *e;

# ifndef  GL_TABLE_TOO_LARGE_EXT
#  define GL_TABLE_TOO_LARGE_EXT 0x8031
# endif
# ifndef  GL_TEXTURE_TOO_LARGE_EXT
#  define GL_TEXTURE_TOO_LARGE_EXT 0x8065
# endif
# ifndef  GL_INVALID_FRAMEBUFFER_OPERATION
#  define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
# endif

  switch (err) {
    case GL_NO_ERROR:              return;
    case GL_INVALID_ENUM:          e = "invalid enum";      break;
    case GL_INVALID_VALUE:         e = "invalid value";     break;
    case GL_INVALID_OPERATION:     e = "invalid operation"; break;
    case GL_STACK_OVERFLOW:        e = "stack overflow";    break;
    case GL_STACK_UNDERFLOW:       e = "stack underflow";   break;
    case GL_OUT_OF_MEMORY:         e = "out of memory";     break;
    case GL_TABLE_TOO_LARGE_EXT:   e = "table too large";   break;
    case GL_TEXTURE_TOO_LARGE_EXT: e = "texture too large"; break;
    case GL_INVALID_FRAMEBUFFER_OPERATION: e = "invalid framebuffer op"; break;
    default:
      e = buf; sprintf (buf, "unknown GL error 0x%04x", (int) err); break;
  }

  fprintf (stderr, "%s: GL error in %s: %s\n", progname, type, e);
}


/* Log a GL error and abort. */
static void
check_gl_error (const char *type)
{
  GLenum err = glGetError();
  if (err == GL_NO_ERROR) return;
  log_gl_error (type, err);
  abort();
}



#ifdef DO_SAVER

/* Here's how XScreenSaver support *should* work: just render the GTK
   widget hierarchy onto an external X11 window, and let it run normally:

     GtkWindow *win = ...;
     GdkDisplay *gdpy = gdk_display_get_default();
     GdkWindow *gw = gdk_x11_window_foreign_new_for_display (gdpy, xwin);
     gtk_widget_set_has_window (win, TRUE);
     gdk_window_set_user_data (gw, win);
     g_signal_connect (win, "realize", G_CALLBACK(foreign_realize), gw);
     and in foreign_realize():
       gtk_widget_set_window (widget, GDK_WINDOW (user_data));

   Spoiler alert, that doesn't work.  I can find no evidence that creating
   a GtkWindow and GtkWidget hierarchy atop an X11 Window works at all.
   See the sample program in this comment on my blog for our best attempt:
   https://www.jwz.org/blog/2022/08/dali-clock-2-46-released/#comment-236849

   So instead what we do is this:

     - Create a new EGL or GLX context on the existing X11 Window;
     - Create, but do not ever realize, the GtkGLArea widget;
     - Manually run that widget's "render" function, and have it bind
       to our context instead of the context that the GtkGLArea would
       have created.

   It's nasty, and it's a lot more code (because EGL is insanely verbose),
   but it works.
 */
static void
init_external_glcontext (XDaliClockWindow *win)
{
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  XWindowAttributes xgwa;

  if (! XGetWindowAttributes (priv->dpy, priv->window_id, &xgwa)) {
    fprintf (stderr, "%s: bad window: 0x%lX\n", progname, priv->window_id);
    exit (1);
  }

# ifdef HAVE_EGL

  priv->egl_display = eglGetDisplay ((EGLNativeDisplayType) priv->dpy);
  if (!priv->egl_display) {
    fprintf (stderr, "%s: eglGetDisplay failed\n", progname);
    exit (1);
  }

  int egl_major = -1, egl_minor = -1;
  if (! eglInitialize (priv->egl_display, &egl_major, &egl_minor)) {
    /* The library already printed this, but without progname:
       "libEGL warning: DRI2: failed to create any config" */
    /* fprintf (stderr, "%s: eglInitialize failed\n", progname); */
    exit (1);
  }

  unsigned int vid = XVisualIDFromVisual (xgwa.visual);

  const EGLint conf_attrs[] = {
    EGL_RED_SIZE,   8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE,  8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
      EGL_NATIVE_VISUAL_ID, vid,
    EGL_NONE
  };
  const EGLint ctx_attrs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  int nconfig = -1;
  if (! (eglChooseConfig (priv->egl_display, conf_attrs, &priv->egl_config,
                          1, &nconfig) &&
         nconfig == 1)) {
    fprintf (stderr, "%s: no matching EGL config for X11 visual 0x%X\n",
             progname, vid);
    exit (1);
  }

  if (! eglBindAPI(EGL_OPENGL_API)) {
    fprintf (stderr, "%s: eglBindAPI failed\n", progname);
    abort();
  }

  priv->egl_context = eglCreateContext (priv->egl_display, priv->egl_config,
                                        EGL_NO_CONTEXT, ctx_attrs);
  if (!priv->egl_context) {
    fprintf (stderr, "%s: eglCreateContext failed\n", progname);
    exit (1);
  }

  priv->egl_surface =
    eglCreatePlatformWindowSurface (priv->egl_display, priv->egl_config,
                                    (void *) &priv->window_id, NULL);
  if (!priv->egl_surface) {
    fprintf (stderr, "%s: eglCreatePlatformWindowSurface failed\n",
             progname);
    exit (1);
  }

# else  // GLX

  XVisualInfo vi_in, *vi_out;
  int out_count;
  vi_in.visualid = XVisualIDFromVisual (xgwa.visual);
  vi_out = XGetVisualInfo (priv->dpy, VisualIDMask, &vi_in, &out_count);
  if (! vi_out) abort ();
  priv->glx_context = glXCreateContext (priv->dpy, vi_out, 0, GL_TRUE);
  XFree ((char *) vi_out);

# endif // GLX
}
#endif // DO_SAVER


// size, position or stacking changed.
//
gboolean
gl_resize_cb (GtkGLArea *self, gint new_width, gint new_height, gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  dali_config *config = priv->config;
  
  priv->glarea_width  = new_width;
  priv->glarea_height = new_height;

  int ow = config->width;
  int oh = config->height;

  float sscale = 2.5;   // use the next-larger bitmap

  config->width  = new_width  * sscale;
  config->height = new_height * sscale;

  render_bitmap_size (config, 
                      &config->width, &config->height,
                      &config->width2, &config->height2);

  if (config->render_state && (ow == config->width && oh == config->height))
    return FALSE;  // nothing to do

  /* When the window is resized, re-create the bitmaps for the largest
     font that will now fit in the window.
   */
  if (config->bitmap) free (config->bitmap);
  config->bitmap = 0;

  if (config->render_state)
    render_free (config);

  render_init (config);
  priv->prev_time_mode = priv->config->time_mode;

  return FALSE;
}


/* gtk_window_move() sets the origin of the window's WM decorations, but
   GTK's "configure-event" returns the root-relative origin of the window
   within the decorations, so the "configure-event" numbers are too large by
   the size of the decorations (title bar and border).  Without compensating
   for this, the window would move down and slightly to the right every time
   we saved and restored.  GDK provides no way to find those numbers, so we
   have to hack it out X11 style...

   This probably means that under Wayland, the window will keep shifting.
 */
static void
wm_decoration_origin (GtkWindow *gtkw, int *x, int *y)
{
  Display *dpy = gdk_x11_get_default_xdisplay();
  GdkWindow *gdkw = gtk_widget_get_window (GTK_WIDGET (gtkw));
  Window xw = gdk_x11_window_get_xid (gdkw);

  Window root, parent, *kids;
  unsigned int nkids;

  Atom type = None;
  int format;
  unsigned long nitems, bytesafter;
  unsigned char *data;

  static Atom swm_vroot = 0;
  XWindowAttributes xgwa;

  if (!dpy || !xw) return;
  if (! XQueryTree (dpy, xw, &root, &parent, &kids, &nkids))
    abort ();

  if (parent == root)	/* No window above us at all */
    return;

  if (! swm_vroot)
    swm_vroot = XInternAtom (dpy, "__SWM_VROOT", False);

  /* If parent is a virtual root, there is no intervening WM decoration. */
  if (XGetWindowProperty (dpy, parent, swm_vroot,
                          0, 0, False, AnyPropertyType,
                          &type, &format, &nitems, &bytesafter,
                          (unsigned char **) &data)
      == Success
      && type != None)
    return;

  /* If we have a parent, it is the WM decoration, so use its origin. */
  if (! XGetWindowAttributes (dpy, parent, &xgwa))
    abort();
  *x = xgwa.x;
  *y = xgwa.y;
}


// When the window is resized or moved, save the new geometry in GSettings.
//
gboolean
window_resize_cb (GtkWindow *window, GdkEvent *event, gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);

  int x = event->configure.x;
  int y = event->configure.y;
  wm_decoration_origin (window, &x, &y);

  char geom[100];
  sprintf (geom, "%dx%d+%d+%d", 
           event->configure.width, event->configure.height, x, y);
  g_settings_set_string (priv->settings, "geometry", geom);

  return FALSE;
}


static gboolean
gl_render_cb (GtkGLArea *area, GdkGLContext *context, gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  dali_config *config = priv->config;

  struct {
    struct { double width, height; } size;
    struct { double x, y; } origin;
  } framerect, torect;

  framerect.size.width  = priv->glarea_width;
  framerect.size.height = priv->glarea_height;

  float img_aspect = (float) config->width / (float) config->height;
  float win_aspect = framerect.size.width / (float) framerect.size.height;

  // Scale the image to fill the window without changing its aspect ratio.
  //
  if (win_aspect > img_aspect) {
    torect.size.height = framerect.size.height;
    torect.size.width  = framerect.size.height * img_aspect;
  } else {
    torect.size.width  = framerect.size.width;
    torect.size.height = framerect.size.width / img_aspect;
  }

  /**************************************************************************
     Choose the size of the texture
   **************************************************************************/

  char err[100];
  int retries = 0;

  while (1) {	// Loop trying smaller sizes

    strcpy (err, "glTexImage2D");

    // The animation slows down a lot if we use truly gigantic numbers,
    // so limit the number size in screensaver-mode.  Note that this
    // limits the size of the output quad, not the size of the texture.
    //
# ifdef DO_SAVER
    if (priv->window_id) {
      int maxh = (config->time_mode == SS ? 512 : 256);
      // maxh *= s;
      if (torect.size.height > maxh) {
        torect.size.height = maxh;
        torect.size.width  = maxh * img_aspect;
      }
    }
# endif // DO_SAVER

    // put a 10% margin between the numbers and the edge of the window.
    torect.size.width  *= 0.9;
    torect.size.height *= 0.9;

    // center it in the window
    //
    torect.origin.x = (framerect.size.width  - torect.size.width ) / 2;
    torect.origin.y = (framerect.size.height - torect.size.height) / 2;

    // Don't allow the top of the number to be off screen (iPhone 5, sec only)
    //
    if (torect.origin.x < 0) {
      float r = ((torect.size.width + torect.origin.x) / torect.size.width);
      torect.size.width  *= r;
      torect.size.height *= r;
      torect.origin.x = (framerect.size.width  - torect.size.width ) / 2;
      torect.origin.y = (framerect.size.height - torect.size.height) / 2;
    }


  /**************************************************************************
     Set the current GL context before talking to OpenGL
   **************************************************************************/

# ifdef DO_SAVER
    if (priv->window_id)
      {
#  ifdef HAVE_EGL
        if (! eglMakeCurrent (priv->egl_display, priv->egl_surface,
                              priv->egl_surface, priv->egl_context))
          abort();
#  else  // GLX
        if (! glXMakeCurrent (priv->dpy, priv->window_id, priv->glx_context))
          abort();
        glDrawBuffer (GL_BACK);
#  endif // GLX
      }
    else
# endif // DO_SAVER
      {
        // gtk_gl_area_make_current (area);   // Should already be current
      }

  /**************************************************************************
     Bind the bitmap to a texture
   **************************************************************************/

    // We alternate between two different texture IDs, only writing to the one
    // that is not currently on screen.  If we don't do this, then the window
    // flickers if it is above another window that has video playing in it.

    // The texture data we got from digital.c has all ones as the color
    // component and either all ones or all zeros as the alpha component.
    // This means that only the "ink" pixels of the digits are written into
    // the color buffer, and are multiplied by the prevailing glColor value.
    //
    // Originally we used GL_LUMINANCE_ALPHA, meaning one byte of alpha
    // plus one byte of color that is used for R, G and B; but apparently
    // GL_LUMINANCE_ALPHA has fallen out of favor with more recent versions
    // of OpenGL, so instead we can waste 4 bytes instead and use GL_RGBA.

    sprintf (err + strlen(err), " #%d %dx%d", 
             retries, config->width2, config->height2);

    if (! config->bitmap) abort();
    glBindTexture (GL_TEXTURE_2D, priv->textures[0]);

#   define FMT GL_RGBA   // GL_LUMINANCE_ALPHA
    glTexImage2D (GL_TEXTURE_2D, 0, FMT,
                  config->width2, config->height2, 0,
                  FMT, GL_UNSIGNED_BYTE, config->bitmap);
#   undef FMT

    GLenum errn = glGetError();
    if (errn == GL_NO_ERROR) {			// Texture succeeded.
      break;
    } else if (errn == GL_INVALID_VALUE) {	// Texture too large. Retry.

      log_gl_error (err, errn);
      retries++;

      // Reduce the target size of the texture.
      // If it's insanely small, abort.

      unsigned int ow = config->width2;
      unsigned int oh = config->height2;
      int toosmall = 20;
      int size_tries = 0;

      while (config->height2 > toosmall &&
             ow == config->width2 &&
             oh == config->height2) {
        config->height -= 4;
        render_bitmap_size (config, 
                            &config->width, &config->height,
                            &config->width2, &config->height2);
        if (size_tries++ > 2000) abort();  // sanity
      }

      if (config->height2 <= toosmall) abort();

      // Must re-render if size changed.
      if (config->bitmap) free (config->bitmap);
      config->bitmap = 0;
      if (config->render_state) render_free (config);
      if (config->bitmap) free (config->bitmap);
      config->bitmap = 0;
      render_init (config);
      priv->prev_time_mode = priv->config->time_mode;

    } else if (errn != GL_NO_ERROR) {		// Unknown error.
      log_gl_error (err, errn);
      abort();
    }
  }

  if (retries > 0)
    fprintf (stderr, "%s: succeded: %s\n", progname, err);


  /**************************************************************************
     Finish texture initialization
   **************************************************************************/

# ifdef DO_DOUBLE_TEXTURE
  GLuint swap = priv->textures[0];
  priv->textures[0] = priv->textures[1];
  priv->textures[1] = swap;
# endif

  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  check_gl_error ("glTexParameteri");

  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable (GL_BLEND);
  glEnable (GL_TEXTURE_2D);


  /**************************************************************************
     Set the viewport and projection matrix
   **************************************************************************/

  glViewport (0, 0, framerect.size.width, framerect.size.height);

  glMatrixMode(GL_PROJECTION);
  { // glLoadIdentity();
    // glOrtho (0, framerect.size.width, 0, framerect.size.height, -1, 1);
    GLfloat a = 2 / framerect.size.width;
    GLfloat b = 2 / framerect.size.height;
    const GLfloat m[16] = {
      a, 0, 0, 0,
      0, b, 0, 0,
      0, 0,-1, 0,
     -1,-1, 0, 1
    };
    glLoadMatrixf (m);
  }

  glMatrixMode(GL_MODELVIEW);
  { // glLoadIdentity();
    const GLfloat m[16] = {
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1
    };
    glLoadMatrixf (m);
  }


  /**************************************************************************
     Set the foreground and background colors
   **************************************************************************/

  glClearColor (priv->bg.r, priv->bg.g, priv->bg.b, priv->bg.a);
  glColor4f (priv->fg.r, priv->fg.g, priv->fg.b, priv->fg.a);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  check_gl_error ("clear");


  /**************************************************************************
     Create the quad
   **************************************************************************/

  GLfloat qx = torect.origin.x;
  GLfloat qy = torect.origin.y;
  GLfloat qw = torect.size.width;
  GLfloat qh = torect.size.height;
  GLfloat tw = (GLfloat) config->width  / config->width2;
  GLfloat th = (GLfloat) config->height / config->height2;

  if (config->left_offset != 0)
    glTranslatef (-qw * ((GLfloat) config->left_offset / config->width),
                  0, 0);

  GLfloat vertices[] = {
    qx,    qy,    0,
    qx+qw, qy,    0,
    qx,    qy+qh, 0,
    qx+qw, qy+qh, 0
  };
  GLfloat texCoords[] = {
    0,  0,
    tw, 0,
    0,  th,
    tw, th
  };
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  check_gl_error ("client state");

  glVertexPointer(3, GL_FLOAT, 0, vertices);
  glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  check_gl_error ("draw arrays");

  /**************************************************************************
     Flush bits to the screen.
   **************************************************************************/

# ifdef DO_SAVER
#  ifdef HAVE_EGL
  if (priv->window_id)
    eglSwapBuffers (priv->egl_display, priv->egl_surface);
#  else   // GLX
  if (priv->window_id)
    glXSwapBuffers (priv->dpy, priv->window_id);
#  endif  // GLX
# endif // DO_SAVER

  glFinish();

  return TRUE;
}


static gboolean
date_off_cb (gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  dali_config *config = priv->config;

  if (priv->date_off_timer) {
    g_source_remove (priv->date_off_timer);
    priv->date_off_timer = 0;
  }

  config->display_date_p = 0;
  return FALSE;
}


// Single tap: briefly display date.
// Single hold: keep displaying date.
// Double tap: toggle 24 hour mode.
//
static void
mouse_cb (GtkWidget *self, GdkEventButton *event, gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  dali_config *config = priv->config;

  if (event->type == GDK_BUTTON_RELEASE)
    priv->drag_p = FALSE;

  if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
    config->display_date_p = 1;

  } else if (event->type == GDK_BUTTON_RELEASE && event->button == 1) {
    // Back to time display shortly after mouse released.
    if (config->display_date_p) {
      if (priv->date_off_timer) {
        g_source_remove (priv->date_off_timer);
        priv->date_off_timer = 0;
      }
      priv->date_off_timer = g_timeout_add (2.0 * 1000, date_off_cb, win);
    }

  } else if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {
    config->display_date_p = 0;

    // Change it only if hours currently visible;
    // In seconds-only mode, this is a mis-tap.
    if (config->time_mode != SS) {
      config->twelve_hour_p = !config->twelve_hour_p;
      g_settings_set_string (priv->settings, "hourmode",
                             config->twelve_hour_p ? "12" : "24");
    }

  } else if (event->type == GDK_BUTTON_RELEASE && event->button != 1) {
    XDaliClock *app = XDALICLOCK_APP (priv->app);
    xdaliclock_app_open_prefs (app);
  }
}


// Drags move the window, in case the title bar is hidden.
//
static void
motion_cb (GtkWidget *self, GdkEventMotion *event, gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);

  if (priv->drag_p) return;
  priv->drag_p = TRUE;

  int button = (event->state & GDK_BUTTON1_MASK	? 1 :
                event->state & GDK_BUTTON2_MASK	? 2 :
                event->state & GDK_BUTTON3_MASK	? 3 :
                event->state & GDK_BUTTON4_MASK	? 4 :
                event->state & GDK_BUTTON5_MASK	? 5 : 0);
  gtk_window_begin_move_drag (GTK_WINDOW (win), button,
                              event->x_root, event->y_root, event->time);
}


static void
keypress_cb (GtkWidget *self, GdkEventKey *event, gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  dali_config *config = priv->config;

  // Typing Ctrl-0 through Ctrl-9 and Ctrl-hyphen are a debugging hack.
  if ((event->state & GDK_CONTROL_MASK) &&
      (event->keyval == '-' ||
       (event->keyval >= '0' && event->keyval <= '9'))) {
    config->test_hack = event->keyval;

  // Up or right: increase number of digits.
  } else if ((event->keyval == GDK_KEY_Up ||
              event->keyval == GDK_KEY_Page_Up ||
              event->keyval == GDK_KEY_KP_Up ||
              event->keyval == GDK_KEY_KP_Page_Up ||
              event->keyval == GDK_KEY_uparrow ||
              event->keyval == GDK_KEY_Right ||
              event->keyval == GDK_KEY_KP_Right ||
              event->keyval == GDK_KEY_rightarrow ||
              event->keyval == '+' ||
              event->keyval == '=') &&
             config->time_mode != HHMMSS) {
    config->time_mode = (config->time_mode == SS ? HHMM : HHMMSS);
    config->width = config->height = 0;
    gl_resize_cb (priv->glarea, priv->glarea_width, priv->glarea_height, win);

  // Down or left: decrease number of digits.
  } else if ((event->keyval == GDK_KEY_Down ||
              event->keyval == GDK_KEY_Page_Down ||
              event->keyval == GDK_KEY_KP_Down ||
              event->keyval == GDK_KEY_KP_Page_Down ||
              event->keyval == GDK_KEY_downarrow ||
              event->keyval == GDK_KEY_Left ||
              event->keyval == GDK_KEY_KP_Left ||
              event->keyval == GDK_KEY_leftarrow ||
              event->keyval == '-' ||
              event->keyval == '_') &&
             config->time_mode != SS) {
    config->time_mode = (config->time_mode == HHMMSS ? HHMM : SS);
    config->width = config->height = 0;
    gl_resize_cb (priv->glarea, priv->glarea_width, priv->glarea_height, win);

  // All of these sound quitty.
  // I expected gtk_application_set_accels_for_action in xdaliclock_app_startup
  // to handle this, but apparently not.
  } else if (event->keyval == 'q' ||
             event->keyval == 'Q' ||
             event->keyval == GDK_KEY_Escape ||
             (event->keyval == 'c' && (event->state & GDK_CONTROL_MASK))) {
    g_application_quit (G_APPLICATION (priv->app));
  }
}


static void handle_countdown (XDaliClockWindow *win);

/* When this timer goes off, we re-generate the bitmap
   and mark the display as invalid.
*/
static gboolean
clock_tick_cb (gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  dali_config *config = priv->config;

  if (! priv->glarea) return TRUE;  // Exiting

  if (priv->clock_timer) {
    g_source_remove (priv->clock_timer);
    priv->clock_timer = 0;
  }

  if (config->render_state) {

    handle_countdown (win);

    // If the number of digits has changed, re-render.
    if (config->time_mode != priv->prev_time_mode) {
      config->width = config->height = 0;
      gl_resize_cb (priv->glarea, priv->glarea_width, priv->glarea_height,
                    win);
    }

    render_once (config);

# ifdef DO_SAVER
    if (priv->window_id)
      {
        static int tick = 0;

        // I can't figure out how to process ConfigureNotify events,
        // so let's just poll the window position about once a second.
        if (++tick >= 30) {
          tick = 0;
          XWindowAttributes xgwa;
          if (! XGetWindowAttributes (priv->dpy, priv->window_id, &xgwa))
            abort();
          if (xgwa.width  != priv->glarea_width ||
              xgwa.height != priv->glarea_height) {
            priv->glarea_width  = xgwa.width;
            priv->glarea_height = xgwa.height;
            gl_resize_cb (priv->glarea, xgwa.width, xgwa.height, win);
          }
        }

        gl_render_cb (priv->glarea, gtk_gl_area_get_context (priv->glarea),
                      data);
      }
    else
# endif  // DO_SAVER
      gtk_widget_queue_draw (GTK_WIDGET (priv->glarea));
  }

  /* Re-schedule the timer according to current fps.
     We could instead not remove it and return TRUE, so this same timer
     continues to fire, but then we'd have to notice when max_fps and
     remove and re-add the timer.  Simpler just to fire once and re-add.
   */
  if (config->max_fps <= 0) abort();
  float delay = 0.9 / config->max_fps;
  priv->clock_timer = g_timeout_add (delay * 1000, clock_tick_cb, win);

  return FALSE;
}


/* When this timer goes off, we re-pick the foreground/background colors,
   and mark the display as invalid.
 */
static gboolean
color_tick_cb (gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  dali_config *config = priv->config;

  if (! priv->glarea) return TRUE;  // Exiting

  if (priv->color_timer) {
    g_source_remove (priv->color_timer);
    priv->color_timer = 0;
  }

  // If they changed the colors in prefs, switch to those right now.
  if (config->fg.r != priv->orig_fg.r ||
      config->fg.g != priv->orig_fg.g ||
      config->fg.b != priv->orig_fg.b ||
      config->fg.a != priv->orig_fg.a ||
      config->bg.r != priv->orig_bg.r ||
      config->bg.g != priv->orig_bg.g ||
      config->bg.b != priv->orig_bg.b ||
      config->bg.a != priv->orig_bg.a) {
    priv->orig_fg = config->fg;
    priv->orig_bg = config->bg;
    priv->fg = config->fg;
    priv->bg = config->bg;
  }

  double h, s, v;
  unsigned short r, g, b;
  float tick = 1.0;   // cycle H by one degree per tick

  r = priv->fg.r * 0xFFFF;
  g = priv->fg.g * 0xFFFF;
  b = priv->fg.b * 0xFFFF;
  rgb_to_hsv (r, g, b, &h, &s, &v);
  h += tick;
  while (h > 360) h -= 360;
  hsv_to_rgb (h, s, v, &r, &g, &b);
  priv->fg.r = r / (double) 0xFFFF;
  priv->fg.g = g / (double) 0xFFFF;
  priv->fg.b = b / (double) 0xFFFF;

  r = priv->bg.r * 0xFFFF;
  g = priv->bg.g * 0xFFFF;
  b = priv->bg.b * 0xFFFF;
  rgb_to_hsv (r, g, b, &h, &s, &v);
  h += tick * 0.91;   // cycle bg slightly slower than fg, for randomosity.
  while (h > 360) h -= 360;
  hsv_to_rgb (h, s, v, &r, &g, &b);
  priv->bg.r = r / (double) 0xFFFF;
  priv->bg.g = g / (double) 0xFFFF;
  priv->bg.b = b / (double) 0xFFFF;

  priv->fg.a = priv->bg.a = 1;  // Since opacity doesn't work

  // This seems to do nothing:
  // gtk_gl_area_set_has_alpha (priv->glarea, TRUE);

  // Under window managers that include a compositor, we can set the opacity
  // of the *entire* window, but what I really want is to have the foreground
  // be solid and the background be transluscent.
  //
  if (priv->prev_opacity != config->window_opacity) {
    if (config->window_opacity > 1)    config->window_opacity = 1;
    if (config->window_opacity < 0.05) config->window_opacity = 0.05;
    gdk_window_set_opacity (gtk_widget_get_window (GTK_WIDGET (win)),
                            config->window_opacity);
    priv->prev_opacity = config->window_opacity;
  }

  if (priv->prev_hide_titlebar_p != config->hide_titlebar_p) {
    gtk_window_set_decorated (GTK_WINDOW (win), !config->hide_titlebar_p);
    priv->prev_hide_titlebar_p = config->hide_titlebar_p;
  }


  /* Re-schedule the timer according to current fps.
     We could instead not remove it and return TRUE, so this same timer
     continues to fire, but then we'd have to notice when max_cps and
     remove and re-add the timer.  Simpler just to fire once and re-add.
   */
  float delay = (config->max_cps > 0 ? 1.0 / config->max_cps : 1);

  priv->color_timer = g_timeout_add (delay * 1000, color_tick_cb, win);
  return FALSE;
}


/* When this timer goes off, we switch to "show date" mode.
 */
static gboolean
date_tick_cb (gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);

  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  dali_config *config = priv->config;

  if (priv->date_timer) {
    g_source_remove (priv->date_timer);
    priv->date_timer = 0;
  }

  float delay = AUTO_DATE;

  if (delay <= 0) return TRUE;

  gboolean was_on = config->display_date_p;

  if (config->time_mode != SS)		// don't auto-date in secs-only mode
    config->display_date_p = !was_on;

  if (!was_on) delay = 3.0;

  priv->date_timer = g_timeout_add (delay * 1000, date_tick_cb, win);

  return FALSE;
}


static gboolean
countdown_home_stretch_1 (XDaliClockWindow *win, gboolean start_p)
{
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  dali_config *config = priv->config;

  if (start_p && priv->countdown_home_stretch_start_timer) {
    g_source_remove (priv->countdown_home_stretch_start_timer);
    priv->countdown_home_stretch_start_timer = 0;
  }

  if (!start_p && priv->countdown_home_stretch_end_timer) {
    g_source_remove (priv->countdown_home_stretch_end_timer);
    priv->countdown_home_stretch_end_timer = 0;
  }

  config->time_mode = (start_p ? SS : HHMMSS);

  // Number of digits has changed, so re-render.
  config->width = config->height = 0;
  gl_resize_cb (priv->glarea, priv->glarea_width, priv->glarea_height, win);

  return FALSE;
}


static gboolean
countdown_home_stretch_start_cb (gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  return countdown_home_stretch_1 (win, TRUE);
}


static gboolean
countdown_home_stretch_end_cb (gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  return countdown_home_stretch_1 (win, FALSE);
}


static void
handle_countdown (XDaliClockWindow *win)
{
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  dali_config *config = priv->config;

  time_t n = (config->countdown_seconds_p ? config->countdown : 0);
  if (n == priv->prev_countdown)
    return;
  priv->prev_countdown = n;

  if (! priv->countdown_home_stretch_start_timer &&
      priv->countdown_home_stretch_end_timer &&
      config->time_mode == SS) {
    // Turning off countdown mode while inside the home stretch.
    config->time_mode = HHMMSS;
    // Number of digits has changed, so re-render.
    config->width = config->height = 0;
    gl_resize_cb (priv->glarea, priv->glarea_width, priv->glarea_height, win);
  }

  if (priv->countdown_home_stretch_start_timer) {
    g_source_remove (priv->countdown_home_stretch_start_timer);
    priv->countdown_home_stretch_start_timer = 0;
  }

  if (priv->countdown_home_stretch_end_timer) {
    g_source_remove (priv->countdown_home_stretch_end_timer);
    priv->countdown_home_stretch_end_timer = 0;
  }

  if (config->countdown && config->countdown_seconds_p) {
    time_t now   = time ((time_t *) 0);
    time_t start = config->countdown - HOMESTRETCH;
    time_t end   = config->countdown + HOMESTRETCH;
    double delay1 = start - now;
    double delay2 = end   - now;

    delay1 -= 1.5;  // start a little earlier

    if (delay1 > 0)
      priv->countdown_home_stretch_start_timer =
        g_timeout_add (delay1 * 1000, countdown_home_stretch_start_cb, win);

    if (delay2 > 0)
      priv->countdown_home_stretch_end_timer =
        g_timeout_add (delay2 * 1000, countdown_home_stretch_end_cb, win);
  }
}


/*
static GdkGLContext *
gl_create_context_cb (GtkGLArea *area, gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  GdkWindow *gwin = gtk_widget_get_window (GTK_WIDGET (win));
  GError *err = 0;
  GdkGLContext *ctx = gdk_window_create_gl_context (gwin, &err);
  if (err) {
    fprintf (stderr, "%s: %s\n", progname, err->message);
    exit (1);
  }
  // This doesn't seem to do anything.
  // "versions less than 3.2 are not supported"?
  gdk_gl_context_set_required_version (ctx, 3, 1);
  gtk_gl_area_set_required_version (priv->glarea, 1, 3);

  return ctx;
}
*/


static void
gl_realize_cb (GtkGLArea *area, gpointer data)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (data);
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);

# if 0
  gtk_gl_area_make_current (area);
  GdkGLContext *ctx = gtk_gl_area_get_context (area);

  gboolean fwd = gdk_gl_context_get_forward_compatible (ctx);
  gboolean es = gdk_gl_context_get_use_es (ctx);
  gboolean leg = gdk_gl_context_is_legacy (ctx);
  int maj = -1, min = -1, rmaj = -1, rmin = -1;
  gdk_gl_context_get_version (ctx, &maj, &min);
  gdk_gl_context_get_required_version (ctx, &rmaj, &rmin);
  fprintf (stderr, "%s: fwd=%d es=%d leg=%d v=%d.%d req=%d.%d\n", progname,
           fwd, es, leg, maj, min, rmaj, rmin);
# endif

  xdaliclock_app_prefs_load (priv->settings, priv->config);
  priv->config->max_fps = MAX_FPS;

  if (priv->config->window_opacity > 1)    priv->config->window_opacity = 1;
  if (priv->config->window_opacity < 0.05) priv->config->window_opacity = 0.05;
  gdk_window_set_opacity (gtk_widget_get_window (GTK_WIDGET (win)),
                          priv->config->window_opacity);

  clock_tick_cb (win);
  color_tick_cb (win);

  float delay = AUTO_DATE;
  priv->date_timer = g_timeout_add (delay * 1000, date_tick_cb, win);
}


static void
xdaliclock_app_window_init (XDaliClockWindow *win)
{
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  gtk_widget_init_template (GTK_WIDGET (win));

  // This is where we die if the schema is not installed.
  priv->settings = g_settings_new ("org.jwz.xdaliclock");

  // This is necessary on Debian 11.3 with GTK 3.24.
  // It is not necessary on Raspbian 11.4 with GTK 3.24.
  // This function was added to the API in GTK 3.22.
  //
# if GTK_CHECK_VERSION(3,22,0)
  gtk_gl_area_set_use_es (priv->glarea, TRUE);
# endif

//  g_signal_connect (priv->glarea, "create_context",
//                    G_CALLBACK (gl_create_context_cb), win);

  g_signal_connect (priv->glarea, "realize", G_CALLBACK (gl_realize_cb), win);
  g_signal_connect (priv->glarea, "resize",  G_CALLBACK (gl_resize_cb),  win);
  g_signal_connect (priv->glarea, "render",  G_CALLBACK (gl_render_cb),  win);

  g_signal_connect (win, "button-press-event",  G_CALLBACK (mouse_cb),   win);
  g_signal_connect (win, "button-release-event",G_CALLBACK (mouse_cb),   win);
  g_signal_connect (win, "motion-notify-event", G_CALLBACK (motion_cb),  win);
  g_signal_connect (win, "key-press-event",     G_CALLBACK (keypress_cb),win);
  g_signal_connect (win, "configure-event", G_CALLBACK (window_resize_cb),win);
}


static void
xdaliclock_app_window_dispose (GObject *object)
{
  XDaliClockWindow *win = XDALICLOCK_APP_WINDOW (object);
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);

# define UNT(X) if (priv->X) { g_source_remove (priv->X); priv->X = 0; }
  UNT(clock_timer)
  UNT(color_timer)
  UNT(date_timer)
  UNT(date_off_timer)
  UNT(countdown_home_stretch_start_timer)
  UNT(countdown_home_stretch_end_timer)
# undef UNT

  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (xdaliclock_app_window_parent_class)->dispose (object);
}


static void
xdaliclock_app_window_class_init (XDaliClockWindowClass *class)
{
  G_OBJECT_CLASS (class)->dispose = xdaliclock_app_window_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/jwz/xdaliclock/window.ui");

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockWindow, glarea);
}


XDaliClockWindow *
xdaliclock_app_window_new (XDaliClock *app, dali_config *c
# ifdef DO_SAVER
                           , unsigned long window_id
# endif
                           )
{
  XDaliClockWindow *win =
    g_object_new (XDALICLOCK_APP_WINDOW_TYPE, "application", app, NULL);
  XDaliClockWindowPrivate *priv =
    xdaliclock_app_window_get_instance_private (win);
  priv->app = app;
  priv->config = c;

# ifdef DO_SAVER
  if (window_id)
    {
      priv->dpy = gdk_x11_get_default_xdisplay();

      /* Debian 11.4, Gtk 3.24.24, 2022: under Wayland, get_default_xdisplay
         is returning uninitialized data! */
      if (priv->dpy &&
          (ProtocolVersion (priv->dpy) != 11 ||
           ProtocolRevision (priv->dpy) != 0))
        {
          /*
          fprintf (stderr, "%s: uninitialized data in Display: "
                   "protocol version %d.%d!\n", progname,
                   ProtocolVersion(priv->dpy), ProtocolRevision(priv->dpy));
           */
          priv->dpy = NULL;
        }

      if (!priv->dpy) {
        fprintf (stderr, "%s: no GTK X11 display connection (Wayland?)\n",
                 progname);
        exit (1);
      }

      if (window_id == ~0L)   /* caller uses -1 to mean root */
        window_id = RootWindowOfScreen (DefaultScreenOfDisplay (priv->dpy));

      priv->window_id = window_id;

      XWindowAttributes xgwa;
      if (! XGetWindowAttributes (priv->dpy, priv->window_id, &xgwa)) {
        fprintf (stderr, "%s: bad window: 0x%lX\n", progname, priv->window_id);
        exit (1);
      }

      // Realize the glarea but do not map it.
      gtk_widget_realize (GTK_WIDGET (priv->glarea));
      init_external_glcontext (win);
      gl_resize_cb (priv->glarea, xgwa.width, xgwa.height, win);
    }
  else
# endif // DO_SAVER
    {
      // Restore to the previous size and position.
      char *geom = g_settings_get_string (priv->settings, "geometry");
      if (geom && *geom) {
        int w, h, x, y;
        char c;
        if (4 == sscanf (geom, "%d x %d + %d + %d %c", &w, &h, &x, &y, &c)) {
          gtk_window_set_default_size (GTK_WINDOW (win), w, h);
          gtk_window_move (GTK_WINDOW (win), x, y);
        }
      }
      if (geom) free (geom);

      gtk_window_set_decorated (GTK_WINDOW (win), !c->hide_titlebar_p);
      gtk_window_set_skip_taskbar_hint (GTK_WINDOW (win), TRUE);
    }

  return win;
}
