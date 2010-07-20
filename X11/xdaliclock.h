/* xdaliclock - a melting digital clock
 * Copyright (c) 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2002
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

#ifdef __STDC__
# include <stdlib.h>
# if defined(unix) || defined(__unix) || defined(UNIX)
#  include <unistd.h>
# endif
#endif

#ifdef VMS
# include <descrip.h>
# define R_OK 4
# if __VMS_VER < 70000000
  int strcasecmp(char *s1, char *s2);
  char *strdup(const char *s1);
# endif
#endif

#ifndef isupper
# define isupper(c)  ((c) >= 'A' && (c) <= 'Z')
#endif
#ifndef _tolower
# define _tolower(c)  ((c) - 'A' + 'a')
#endif

#ifndef XtNvisual
# define XtNvisual "visual"
#endif

/* From xdaliclock.c: */
extern char *progname;
extern char *progclass;
extern char *hacked_version;
extern Bool wander_p;
extern XrmDatabase db;
extern Bool do_cycle;
extern long countdown;

/* From colors.c: */
extern void allocate_colors (Screen *screen, Visual *visual, Colormap cmap,
                             char *fg_name, char *bg_name, char *bd_name,
                             XColor *fg_color, XColor*bg_color,
                             XColor *bd_color);
extern void cycle_colors (Screen *screen, Colormap cmap,
                          XColor *fg_color, XColor *bg_color,
                          Window window, GC fg_gc, GC bg_gc);
extern int no_writable_cells;

/* From digital.c: */
extern void initialize_digital (Screen *screen, Visual *visual,
                                Colormap cmap,
                                unsigned long *fgP,
                                unsigned long *bgP,
                                unsigned long *bdP,
                                unsigned int *widthP,
                                unsigned int *heightP);
extern void run_digital (Screen *screen, Window window, Bool external_p);

extern int do_shape;
extern int do_overlay;
extern unsigned long overlay_transparent_pixel;
