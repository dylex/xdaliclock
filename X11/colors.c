/* xdaliclock - a melting digital clock
 * Copyright (c) 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1999, 2001
 *  Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xresource.h>

#if (XtSpecificationRelease > 3)
# include <X11/Xfuncs.h>
#endif

#include "xdaliclock.h"
#include "resources.h"
#include "visual.h"
#include "hsv.h"

int no_writable_cells;


void
allocate_colors (Screen *screen, Visual *visual, Colormap cmap,
                 char   *fg_name,  char   *bg_name,  char   *bd_name,
                 XColor *fg_color, XColor *bg_color, XColor *bd_color)
{
  Display *dpy = DisplayOfScreen(screen);

  if (! fg_name) fg_name = "white";
  if (! bg_name) bg_name = "black";

  no_writable_cells = 0;

#if 0
 AGAIN:
#endif

  if (do_cycle)
    {
      int vclass = visual_class (screen, visual);
      unsigned long plane_masks;
      unsigned long pixels [2];

/*      if (do_overlay)
        {
          no_writable_cells = 1;
          goto NONWRITABLE;
        }
      else*/ if (vclass == StaticGray ||
               vclass == StaticColor ||
               vclass == TrueColor)
        {
          no_writable_cells = 1;
          goto NONWRITABLE;
        }
      else if (XAllocColorCells (dpy, cmap, False,
                                 &plane_masks, 0, pixels,
                                 ((do_overlay || do_shape) ? 1 : 2)))
        {
          fg_color->pixel = pixels [0];

          XParseColor (dpy, cmap, fg_name, fg_color);
          fg_color->flags = DoRed | DoGreen | DoBlue;
          XStoreColor (dpy, cmap, fg_color);

          if (do_overlay)
            {
              bg_color->flags = DoRed | DoGreen | DoBlue;
              bg_color->red = bg_color->green = bg_color->blue = 0;
              bg_color->pixel = overlay_transparent_pixel;
            }
          else if (do_shape)
            {
              bg_color = fg_color;
            }
          else
            {
              bg_color->pixel = pixels [1];
              XParseColor (dpy, cmap, bg_name, bg_color);
              XStoreColor (dpy, cmap, bg_color);
            }
          no_writable_cells = 0;
        }
      else
        {
#if 0
          fprintf (stderr,
              "%s: couldn't allocate two read-write color cells on visual\n\t",
                   progname);
          describe_visual (stderr, dpy, visual);
          do_cycle = 0;
          goto AGAIN;
#else
          no_writable_cells = 1;
          goto NONWRITABLE;
#endif
        }
    }
  else
    {
    NONWRITABLE:
      no_writable_cells = 1;
      if (! XParseColor (dpy, cmap, fg_name, fg_color))
        {
          fprintf (stderr, "%s: can't parse color %s; using black\n",
                   progname, fg_name);
          fg_color->pixel = WhitePixelOfScreen (screen);
        }
      else if (! XAllocColor (dpy, cmap, fg_color))
        {
          fprintf (stderr,
                   "%s: couldn't allocate color \"%s\", using black\n",
                   progname, fg_name);
          fg_color->pixel = WhitePixelOfScreen (screen);
        }

      if (do_overlay)
        {
          bg_color->red = bg_color->green = bg_color->blue = 0;
          bg_color->pixel = overlay_transparent_pixel;
        }
      else if (do_shape)
        {
          bg_color = fg_color;
        }
      else if (! XParseColor (dpy, cmap, bg_name, bg_color))
        {
          fprintf (stderr, "%s: can't parse color %s; using white\n",
                   progname, bg_name);
          bg_color->pixel = BlackPixelOfScreen (screen);
        }
      else if (! XAllocColor (dpy, cmap, bg_color))
        {
          fprintf (stderr,
                   "%s: couldn't allocate color \"%s\", using white\n",
                   progname, bg_name);
          bg_color->pixel = BlackPixelOfScreen (screen);
        }

      /* kludge -rv */
      if (get_boolean_resource ("reverseVideo", "ReverseVideo"))
        {
          XColor swap;
          swap = *fg_color;
          *fg_color = *bg_color;
          *bg_color = swap;
        }
    }

  if (! bd_name)
    bd_name = fg_name;

  /* Set border color to something reasonable in the colormap */
  if (do_overlay || do_shape)
    {
      bd_color->pixel = bg_color->pixel;
    }
  else if (! XParseColor (dpy, cmap, bd_name, bd_color))
    {
      fprintf (stderr, "%s: can't parse color %s; using white\n",
               progname, bd_name);
      bd_color->pixel = WhitePixelOfScreen (screen);
    }
  else if (! XAllocColor (dpy, cmap, bd_color))
    {
      fprintf (stderr, "%s: couldn't allocate color \"%s\", using white\n",
               progname, bd_name);
      bd_color->pixel = WhitePixelOfScreen (screen);
    }
}

void
cycle_colors (Screen *screen, Colormap cmap,
              XColor *fg_color, XColor *bg_color,
              Window window, GC fg_gc, GC bg_gc)
{
  Display *dpy = DisplayOfScreen(screen);
  static int hue_tick;
  hsv_to_rgb (hue_tick,
              1.0, 1.0,
              &fg_color->red, &fg_color->green, &fg_color->blue);
  hsv_to_rgb ((hue_tick + 180) % 360,
              1.0, 1.0,
              &bg_color->red, &bg_color->green, &bg_color->blue);
  hue_tick = (hue_tick+1) % 360;

  if (! no_writable_cells)
    {
      XStoreColor (dpy, cmap, fg_color);
      if (!do_shape && !do_overlay)
        XStoreColor (dpy, cmap, bg_color);
    }
  else
    {
      XColor tmp;
      fg_color->flags = bg_color->flags = DoRed | DoGreen | DoBlue;

      tmp = *fg_color;
      if (!XAllocColor(dpy, cmap, fg_color))
        *fg_color = tmp;
      else
        {
          XFreeColors (dpy, cmap, &tmp.pixel, 1, 0);
          fg_color->red   = tmp.red;
          fg_color->green = tmp.green;
          fg_color->blue  = tmp.blue;
        }

      tmp = *bg_color;
      if (do_overlay)
        bg_color->pixel = overlay_transparent_pixel;
      else if (do_shape)
        bg_color = fg_color;
      else if (!XAllocColor(dpy, cmap, bg_color))
        *bg_color = tmp;
      else
        {
          XFreeColors (dpy, cmap, &tmp.pixel, 1, 0);
          bg_color->red   = tmp.red;
          bg_color->green = tmp.green;
          bg_color->blue  = tmp.blue;
        }

      XSetForeground (dpy, fg_gc, fg_color->pixel);
      XSetBackground (dpy, fg_gc, bg_color->pixel);
      XSetForeground (dpy, bg_gc, bg_color->pixel);
      XSetBackground (dpy, bg_gc, fg_color->pixel);
      XSetWindowBackground (dpy, window,
                            do_shape ? fg_color->pixel : bg_color->pixel);
    }
}
