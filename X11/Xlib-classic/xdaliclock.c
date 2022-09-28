/* xdaliclock - a melting digital clock
 * Copyright (c) 1991-2010 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * Version 1 of this program used only Xlib, not Xt.  The dynamically linked
 * sparc executable was 81k, and geometry and resource handling were kludgy
 * and broken, because I had to duplicate the obscure, undocumented things
 * that Xt does.  Version 2 uses Xt, and geometry and resource handling work
 * correctly - but the dynamically linked executable has more than tripled in
 * size, swelling to 270k.  This is what is commonly refered to as "progress."
 * (Bear in mind that the first (or second) program ever to implement this
 * algorithm ran perfectly well on a Macintosh with a 128k address space.)
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/CoreP.h>

#include "xdaliclock.h"
#include "vroot.h"
#include "version.h"
#include "resources.h"
#include "visual.h"

#ifndef _XSCREENSAVER_VROOT_H_
# error Error!  You have an old version of vroot.h!  Check -I args.
#endif /* _XSCREENSAVER_VROOT_H_ */

char *progname;
char *progclass;
char *hacked_version, *hacked_version2;
XrmDatabase db;
Bool wander_p;
Bool do_cycle;
long countdown;

int do_shape = 0;
int do_overlay = 0;
unsigned long overlay_transparent_pixel = 0;


static char *defaults[] = {
#include "XDaliClock_ad.h"
 0
};


static void usage (void);
static void hack_version (void);

static XrmOptionDescRec options [] = {
  /* global options */
  { "-root",            ".root",                XrmoptionNoArg, "True" },
  { "-onroot",          ".root",                XrmoptionNoArg, "True" },
  { "-window",          ".root",                XrmoptionNoArg, "False" },
  { "-fullscreen",      ".fullScreen",          XrmoptionNoArg, "True" },
  { "-mono",            ".mono",                XrmoptionNoArg, "True" },
  { "-visual",          ".visualID",            XrmoptionSepArg, 0 },
  { "-install",         ".installColormap",     XrmoptionNoArg, "True" },
  { "-title",           ".title",               XrmoptionSepArg, 0  }, /* R3 */
  { "-window-id",       ".windowID",            XrmoptionSepArg, 0 },
  { "-xrm",             0,                      XrmoptionResArg, 0 },

  /* options from digital.c */
  { "-12",              ".mode",                XrmoptionNoArg, "12" },
  { "-24",              ".mode",                XrmoptionNoArg, "24" },
  { "-showdate",        ".showdate",            XrmoptionNoArg, "True" },
  { "-showtime",        ".showdate",            XrmoptionNoArg, "False" },
  { "-datemode",        ".datemode",            XrmoptionSepArg, 0 },
  { "-mm/dd/yy",        ".datemode",            XrmoptionNoArg, "MM/DD/YY" },
  { "-dd/mm/yy",        ".datemode",            XrmoptionNoArg, "DD/MM/YY" },
  { "-seconds",         ".seconds",             XrmoptionNoArg, "True" },
  { "-noseconds",       ".seconds",             XrmoptionNoArg, "False" },
  { "-hex",       	".hex",             	XrmoptionNoArg, "True" },
  { "-nohex",       	".hex",             	XrmoptionNoArg, "False" },
  { "-cycle",           ".cycle",               XrmoptionNoArg, "True" },
  { "-nocycle",         ".cycle",               XrmoptionNoArg, "False" },
  { "-font",            ".font",                XrmoptionSepArg, 0 },
  { "-fn",              ".font",                XrmoptionSepArg, 0 },
  { "-builtin",         ".font",                XrmoptionNoArg, "BUILTIN"  },
  { "-builtin0",        ".font",                XrmoptionNoArg, "BUILTIN0" },
  { "-builtin1",        ".font",                XrmoptionNoArg, "BUILTIN1" },
  { "-builtin2",        ".font",                XrmoptionNoArg, "BUILTIN2" },
  { "-builtin3",        ".font",                XrmoptionNoArg, "BUILTIN3" },
  { "-builtin4",        ".font",                XrmoptionNoArg, "BUILTIN4" },
  { "-memory",          ".memory",              XrmoptionSepArg, 0 },
  { "-sleaze-level",    ".memory",              XrmoptionSepArg, 0 },
  { "-oink",            ".memory",              XrmoptionNoArg, "Medium" },
  { "-oink-oink",       ".memory",              XrmoptionNoArg, "High" },
  { "-transparent",     ".transparent",         XrmoptionNoArg, "True" },
  { "-nontransparent",  ".transparent",         XrmoptionNoArg, "False" },
  { "-countdown",       ".countdown",           XrmoptionSepArg, 0 }
};

int
main (int argc, char **argv)
{
  Widget toplevel = 0;
  Display *dpy;
  Screen *screen;
  unsigned int w, h;
  Window window = 0;
  Visual *visual = 0;
  Colormap cmap;
  unsigned long fg, bg, bd;
  XWindowAttributes xgwa;
  Bool root_p;
  Bool full_screen_p;
  Window on_window = 0;

#if (XtSpecificationRelease >= 4)
  int argc_copy = argc;
  char **argv_copy = (char **) malloc (argc * sizeof(*argv));
  memcpy(argv_copy, argv, argc * sizeof(*argv));
#endif

  hack_version ();
  progclass = "XDaliClock";

#if (XtSpecificationRelease >= 4)
  {
    XtAppContext app;
    XtToolkitInitialize ();
    app = XtCreateApplicationContext ();
    XtAppSetFallbackResources (app, defaults);
    dpy = XtOpenDisplay (app, 0, 0, progclass,
                         options, sizeof (options) / sizeof (options [0]),
                         &argc, argv);
    if (!dpy) exit (-1);
    XtGetApplicationNameAndClass (dpy, &progname, &progclass);
    db = XtDatabase (dpy);
  }
#else /* R3 */
  {
    char *tmp;
    int i, j;
    XrmDatabase argv_db, server_db, fallback_db;
    progname = argv [0];
# ifdef VMS
    while (tmp = (char *) strrchr (progname, ']'))
      progname = tmp + 1;
    if (tmp = (char *) strrchr (progname, '.'))
      *tmp = '\0';
# else /* !VMS */
    if (tmp = (char *) strrchr (progname, '/'))
      progname = tmp + 1;
# endif /* !VMS */

    argv_db = 0;
    XrmParseCommand (&argv_db,
                     options, sizeof (options) / sizeof (options [0]),
                     progname, &argc, argv);
    toplevel = XtInitialize (progname, progclass, options,
                             sizeof (options) / sizeof (options [0]),
                             &argc, argv);
    dpy = XtDisplay (toplevel);
    server_db = XtDatabase (dpy);
    for (i = 0, j = 0; defaults [i]; i++)
      j += strlen (defaults [i]) + 1;
    tmp = (char *) malloc (j + 1);
    for (i = 0; defaults [i]; i++)
      {
        tmp = strcat (tmp, defaults [i]);
        tmp += strlen (tmp);
        *tmp++ = '\n';
        *tmp = 0;
      }
    tmp -= j;
    fallback_db = XrmGetStringDatabase (tmp);
    free (tmp);
    db = fallback_db;
    XrmMergeDatabases (server_db, &db);
    XrmMergeDatabases (argv_db, &db);
  }
#endif /* R3 */

  if (toplevel)
    screen = XtScreen (toplevel);
  else
    screen = DefaultScreenOfDisplay(dpy);

  if (argc > 1)
    {
      int help_p = !strcmp (argv [1], "-help");
      if (!help_p)
        fprintf (stderr, "%s: unrecognised option \"%s\"\n",
                 progname, argv[1]);
      usage ();
      exit (help_p ? 0 : -1);
    }

  root_p = get_boolean_resource ("root", "Boolean");
  full_screen_p = get_boolean_resource ("fullScreen", "Boolean");
  do_cycle = get_boolean_resource ("cycle", "Cycle");

  {
    char *s = get_string_resource ("windowID", "WindowID");
    if (s && *s)
      on_window = get_integer_resource ("windowID", "WindowID");
    if (s) free (s);
  }

  countdown = 0;
  {
    char *s = get_string_resource ("countdown", "Countdown");
    if (s && *s)
      {
        time_t now = time((time_t *) 0);
        int h100 = (99*60*60) + (59*60) + 59;
        long l = 0;
        char c = 0;
        char mon[255];
        int day = 0;
        int hour = 0;
        int min = 0;
        int sec = 0;
        int year = 0;
        *mon = 0;
        if (1 == sscanf (s, " %lu %c", &l, &c))
          countdown = l;
        else if (6 == sscanf (s, " %s %d %d:%d:%d %d %c",
                              mon, &day, &hour, &min, &sec, &year, &c))
          {
            struct tm tm;
            int imonth = -1;
            const char *months[] = {"jan", "feb", "mar", "apr", "may", "jun",
                                    "jul", "aug", "sep", "oct", "nov", "dec" };

            for (l = 0; mon[l]; l++)
              if (mon[l] >= 'A' && mon[l] <= 'Z')
                mon[l] = mon[l] + ('a'-'A');

            for (l = 0; l < 12; l++)
              if (!strcmp(months[l], mon))
                {
                  imonth = l;
                  break;
                }
            if (imonth < 0)
              {
                fprintf (stderr, "%s: unparsable month: \"%s\"\n",
                         progname, mon);
                goto LOSE2;
              }
            if (year > 1900) year -= 1900;
            tm.tm_sec = sec;
            tm.tm_min = min;
            tm.tm_hour = hour;
            tm.tm_mday = day;
            tm.tm_mon = imonth;
            tm.tm_year = year;
            tm.tm_wday = 0;     /* ignored */
            tm.tm_yday = 0;     /* ignored */
            tm.tm_isdst = -1;
            countdown = mktime (&tm);
            if ((long) countdown < 0L)
              goto LOSE1;
          }
        else
          {
          LOSE1:
            fprintf (stderr, "%s: unparsable date: \"%s\"\n", progname, s);
          LOSE2:
            fprintf (stderr,
          "%s: argument to -countdown should be a time_t integer\n"
        "\t (number of seconds past \"Jan 1 00:00:00 GMT 1970\");\n"
        "\t Or, a string of the form \"Mmm DD HH:MM:SS YYYY\",\n"
        "\t for example, \"Jan 1 00:00:00 2000\".\n"
        "\t This string is interpreted in the local time zone.\n",
                     progname);
            exit (1);
          }

        if (countdown > now + h100 ||
            countdown < now - h100)
          {
            char *s = ctime ((time_t *) &countdown);
            char *r = strchr (s, '\n');
            if (r) *r = 0;
            fprintf (stderr,
                     "%s: countdown too distant: '%s' is more\n"
                     "            than 100 hours away (can't "
                     "display it on the clock face.)\n",
                     progname, s);
            exit (1);
          }
      }
    if (s) free(s);
  }

  if (on_window)
    {
      XWindowAttributes xgwa;
      window = (Window) on_window;
      XGetWindowAttributes (dpy, window, &xgwa);
      cmap = xgwa.colormap;
      visual = xgwa.visual;
      screen = xgwa.screen;
      do_shape = 0;
      do_overlay = 0;
    }
  else if (root_p)
    {
      window = RootWindowOfScreen (screen);
      XGetWindowAttributes (dpy, window, &xgwa);
      cmap = xgwa.colormap;
      visual = xgwa.visual;
      do_shape = 0;
      do_overlay = 0;
    }
  else
    {
      do_shape = get_boolean_resource ("transparent", "Transparent");
      if (do_shape)
        {
          Visual *v = get_overlay_visual (screen, &overlay_transparent_pixel);
          if (v)
            {
              do_shape = 0;
              do_overlay = 1;
              visual = v;
            }
        }

      if (!do_overlay)
        visual = get_visual_resource (screen, "visualID", "VisualID", True);

      if ((visual != DefaultVisualOfScreen (screen)) ||
          get_boolean_resource ("installColormap", "InstallColormap"))
        cmap = XCreateColormap (dpy, RootWindowOfScreen (screen),
                                visual, AllocNone);
      else
        cmap = DefaultColormapOfScreen (screen);
    }

  wander_p = (root_p || full_screen_p);

  initialize_digital (screen, visual, cmap, &fg, &bg, &bd, &w, &h);

  if (root_p || on_window)
    {
      /* `window' has already been set, above. */
      if (toplevel)
        XtDestroyWidget (toplevel);
    }
  else
    {
      int ymargin = (h / 8) + 1;
      int xmargin = ymargin;
      Dimension ow, oh, bw;
      Arg av [20];
      int ac;

      ac = 0;
      XtSetArg (av [ac], XtNinput, True); ac++;
      XtSetArg (av [ac], XtNvisual, visual); ac++;
      XtSetArg (av [ac], XtNcolormap, cmap); ac++;
      XtSetArg (av [ac], XtNdepth, visual_depth (screen, visual)); ac++;
      XtSetArg (av [ac], XtNforeground, ((Pixel) fg)); ac++;
      XtSetArg (av [ac], XtNbackground, ((Pixel) bg)); ac++;
      XtSetArg (av [ac], XtNborderColor, (Pixel) bd); ac++;
      /* This sometimes causes -geometry to be ignored... */
      /* XtSetArg (av [ac], XtNminWidth, (Dimension) w); ac++; */
      /* XtSetArg (av [ac], XtNminHeight, (Dimension) h); ac++; */

#if (XtSpecificationRelease >= 4)
      toplevel = XtAppCreateShell (progname, progclass,
                                   applicationShellWidgetClass, dpy,
                                   av, ac);
#else /* R3 */
      /* This won't work with non-default visuals; upgrade!! */
      XtSetValues (toplevel, av, ac);
#endif /* R3 */

      ac = 0;
      XtSetArg (av [ac], XtNwidth, &ow); ac++;
      XtSetArg (av [ac], XtNheight, &oh); ac++;
      XtSetArg (av [ac], XtNborderWidth, &bw); ac++;
      XtGetValues (toplevel, av, ac);

      if (full_screen_p)
        {
          w = WidthOfScreen  (screen);
          h = HeightOfScreen (screen);
        }
      if (ow <= 1 || oh <= 1)
        {
          Dimension ww = w + xmargin + xmargin;
          Dimension hh = h + ymargin + ymargin;
          ac = 0;
          XtSetArg (av [ac], XtNwidth,  ww); ac++;
          XtSetArg (av [ac], XtNheight, hh); ac++;
          XtSetValues (toplevel, av, ac);
        }
      if (full_screen_p)
        {
          /* #### Want this to set UPosition, not PPosition... */
          ac = 0;
          XtSetArg (av [ac], XtNx, -xmargin); ac++;
          XtSetArg (av [ac], XtNy, -ymargin); ac++;
          XtSetValues (toplevel, av, ac);
        }

      XtRealizeWidget (toplevel);
      window = XtWindow (toplevel);
    }

#if (XtSpecificationRelease >= 4)
  if (!root_p)
    XSetStandardProperties(dpy, window, progname, progname,
                           0, /* icon pixmap */
                           argv_copy, argc_copy,
                           NULL /* XSizeHints* */);
  free(argv_copy);
#endif

  run_digital (screen, window, (root_p || on_window));
  return (0);
}


static void
usage (void)
{
  fprintf (stderr, "\n%s\n\
                  http://www.jwz.org/xdaliclock/\n\n\
usage: %s [ options ]\n\
where options include\n\
\n\
  -12                           Display twelve hour time (default).\n\
  -24                           Display twenty-four hour time.\n\
  -showtime                     Show the time initially (default).\n\
  -showdate                     Show the date initially.\n\
  -seconds                      Display seconds (default).\n\
  -noseconds                    Don't display seconds.\n\
  -cycle                        Do color-cycling (default).\n\
  -nocycle                      Don't do color-cycling.\n\
  -hex				Display hex time.\n\
  -nohex			Display decimal time (default).\n\
  -countdown <date>             Display a countdown instead of a clock.\n\
                                Run `-countdown foo' to see date syntax.\n\
  -display <host:dpy>           The display to run on.\n\
  -visual <visual-class>        The visual to use.\n\
  -install                      Allocate a private colormap.\n\
  -geometry <geometry>          Size and position of window.\n\
  -foreground or -fg <color>    Foreground color (default white).\n\
  -background or -bg <color>    Background color (default black).\n\
  -reverse or -rv               Swap foreground and background.\n\
  -borderwidth or -bw <int>     Border width.\n\
  -bd or -border <color>        Border color.\n\
  -title <string>               Window title.\n\
  -name <string>                Resource-manager name (%s).\n\
  -fullscreen                   Use a window that takes up the whole screen.\n\
  -root                         Draw on the root window instead.\n\
  -window                       Opposite of -root (default).\n\
  -datemode <MMDDYY | DDMMYY>   How to format the date.\n\
", hacked_version2, progname, progname);
#ifdef BUILTIN_FONTS
  fprintf (stderr, "\
  -font or -fn <font>           Name of an X font or built-in font to use.\n\
                                There are six builtin fonts, named\n\
                                \"BUILTIN0\", through \"BUILTIN4\", with #2\n\
                                being the default.\n\
  -builtin0                     Same as -font BUILTIN0.\n\
  -builtin1                     Same as -font BUILTIN1.\n\
  -builtin2                     Same as -font BUILTIN2.\n\
  -builtin                      Same as -font BUILTIN.\n\
  -builtin3                     Same as -font BUILTIN3.\n\
  -builtin4                     Same as -font BUILTIN4.\n\
");
#else /* !BUILTIN_FONTS */
  fprintf (stderr, "\
  -font or -fn <font>           Name of an X font to use.\n\
");
#endif /* !BUILTIN_FONTS */
  fprintf (stderr, "\
  -memory <high|medium|low>     Tune the memory versus bandwidth consumption;\n\
                                default is \"low\".\n\
  -oink                         Same as -memory medium.\n\
  -oink-oink                    Same as -memory high.\n\
  -transparent                  Make the window background be transparent,\n\
                                if possible.\n\
  -nontransparent               Don't.\n\n");
#ifndef BUILTIN_FONTS
  fprintf(stderr,
          "This version has been compiled without the builtin fonts.\n\n");
#endif /* BUILTIN_FONTS */
}

static void
hack_version (void)
{
  int i;
  const char *src = version + 4;
  char *dst1 = hacked_version  = (char *) malloc (strlen (src) + 1);
  char *dst2 = hacked_version2 = (char *) malloc (strlen (src) + 1);
#define digitp(x) (('0' <= (x)) && ((x) <= '9'))
  while (*src)
    if (!strncmp ("Copyright (c)", src, 13))
      {
        *dst1++ = '\251';
        for (i = 0; i < 13; i++) *dst2++ = *src++;
      }
    else if (*src == '(') *dst1++ = *dst2++ = '<', src++;
    else if (*src == ')') *dst1++ = *dst2++ = '>', src++;
    else if (digitp (src[0]) && digitp (src[1]) && digitp (src[2]) &&
             digitp (src[3]) && src[4] == ',')
      {
        src += 5;
        if (*src == ' ') src++;
      }
    else
      *dst1++ = *dst2++ = *src++;
  *dst1 = 0;
  *dst2 = 0;
}

#if defined(VMS) && (__VMS_VER < 70000000)
/* this isn't right, but it's good enough (only returns 1 or 0) */
int
strcasecmp (s1, s2)
    char *s1, *s2;
{
  int i;
  int L1 = strlen (s1);
  int L2 = strlen (s2);
  if (L1 != L2)
    return 1;
  for (i = 0; i < L1; i++)
    if ((isupper (s1[i]) ? _tolower (s1[i]) : s1[i]) !=
        (isupper (s2[i]) ? _tolower (s2[i]) : s2[i]))
      return 1;
  return 0;
}

char *
strdup (const char* s1)
{
  char *s2 = (char*) malloc (strlen (s1) + 1);
  if (s2) strcpy (s2, s1);
  return s2;
}
#endif /* VMS && __VMS_VER < 70000000 */
