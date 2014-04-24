/* xdaliclock - a melting digital clock
 * Copyright (c) 1991-2006 Jamie Zawinski <jwz@jwz.org>
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xresource.h>

#if (XtSpecificationRelease > 3)
# include <X11/Xfuncs.h>
#endif

#include "xdaliclock.h"
#include "resources.h"
#include "usleep.h"
#include "vroot.h"

#ifndef _XSCREENSAVER_VROOT_H_
# error Error!  You have an old version of vroot.h!  Check -I args.
#endif /* _XSCREENSAVER_VROOT_H_ */

#define clock_usleep screenhack_usleep

#ifdef HAVE_SHAPE
# include <X11/extensions/shape.h>
#endif

static void fill_borders (Display *dpy, Window window, GC gc,
                          int x, int y, int width, int height);
static void draw_colon (Display *dpy, Drawable pixmap, Window window);

#define COLON_CHAR (hex_time ? '_' : ':')
#define SLASH_CHAR ('/')

#ifdef BUILTIN_FONTS

#define NUMS_COLON_INDEX	(hex_time ? 17 : 16)
#define NUMS_SLASH_INDEX	18

static int use_builtin_font;

struct raw_number {
  unsigned char *bits;
  int width, height;
};

#define FONT(X)								\
 static const struct raw_number numbers_ ## X [] = {			\
  { zero  ## X ## _bits,  zero ## X ## _width,  zero ## X ## _height }, \
  { one   ## X ## _bits,   one ## X ## _width,   one ## X ## _height }, \
  { two   ## X ## _bits,   two ## X ## _width,   two ## X ## _height }, \
  { three ## X ## _bits, three ## X ## _width, three ## X ## _height }, \
  { four  ## X ## _bits,  four ## X ## _width,  four ## X ## _height }, \
  { five  ## X ## _bits,  five ## X ## _width,  five ## X ## _height }, \
  { six   ## X ## _bits,   six ## X ## _width,   six ## X ## _height }, \
  { seven ## X ## _bits, seven ## X ## _width, seven ## X ## _height }, \
  { eight ## X ## _bits, eight ## X ## _width, eight ## X ## _height }, \
  { nine  ## X ## _bits,  nine ## X ## _width,  nine ## X ## _height }, \
  { ten   ## X ## _bits,   ten ## X ## _width,   ten ## X ## _height }, \
  { eleven## X ## _bits,eleven ## X ## _width,eleven ## X ## _height }, \
  { twelve## X ## _bits,twelve ## X ## _width,twelve ## X ## _height }, \
  {thirteen##X ## _bits,thirteen##X ## _width,thirteen##X ## _height }, \
  {fourteen##X ## _bits,fourteen##X ## _width,fourteen##X ## _height }, \
  {fifteen## X ## _bits,fifteen## X ## _width,fifteen## X ## _height }, \
  { colon ## X ## _bits, colon ## X ## _width, colon ## X ## _height }, \
  {underbar##X ## _bits,underbar##X ## _width,underbar##X ## _height }, \
  { slash ## X ## _bits, slash ## X ## _width, slash ## X ## _height }, \
  { 0, }								\
}

# include "zeroB.xbm"
# include "oneB.xbm"
# include "twoB.xbm"
# include "threeB.xbm"
# include "fourB.xbm"
# include "fiveB.xbm"
# include "sixB.xbm"
# include "sevenB.xbm"
# include "eightB.xbm"
# include "nineB.xbm"
# include "tenB.xbm"
# include "elevenB.xbm"
# include "twelveB.xbm"
# include "thirteenB.xbm"
# include "fourteenB.xbm"
# include "fifteenB.xbm"
# include "colonB.xbm"
# include "underbarB.xbm"
# include "slashB.xbm"
FONT(B);

# include "zeroC.xbm"
# include "oneC.xbm"
# include "twoC.xbm"
# include "threeC.xbm"
# include "fourC.xbm"
# include "fiveC.xbm"
# include "sixC.xbm"
# include "sevenC.xbm"
# include "eightC.xbm"
# include "nineC.xbm"
# include "tenC.xbm"
# include "elevenC.xbm"
# include "twelveC.xbm"
# include "thirteenC.xbm"
# include "fourteenC.xbm"
# include "fifteenC.xbm"
# include "colonC.xbm"
# include "underbarC.xbm"
# include "slashC.xbm"
FONT(C);

# include "zeroD.xbm"
# include "oneD.xbm"
# include "twoD.xbm"
# include "threeD.xbm"
# include "fourD.xbm"
# include "fiveD.xbm"
# include "sixD.xbm"
# include "sevenD.xbm"
# include "eightD.xbm"
# include "nineD.xbm"
# include "tenD.xbm"
# include "elevenD.xbm"
# include "twelveD.xbm"
# include "thirteenD.xbm"
# include "fourteenD.xbm"
# include "fifteenD.xbm"
# include "colonD.xbm"
# include "underbarD.xbm"
# include "slashD.xbm"
FONT(D);

# include "zeroE.xbm"
# include "oneE.xbm"
# include "twoE.xbm"
# include "threeE.xbm"
# include "fourE.xbm"
# include "fiveE.xbm"
# include "sixE.xbm"
# include "sevenE.xbm"
# include "eightE.xbm"
# include "nineE.xbm"
# include "tenE.xbm"
# include "elevenE.xbm"
# include "twelveE.xbm"
# include "thirteenE.xbm"
# include "fourteenE.xbm"
# include "fifteenE.xbm"
# include "colonE.xbm"
# include "underbarE.xbm"
# include "slashE.xbm"
FONT(E);

# include "zeroF.xbm"
# include "oneF.xbm"
# include "twoF.xbm"
# include "threeF.xbm"
# include "fourF.xbm"
# include "fiveF.xbm"
# include "sixF.xbm"
# include "sevenF.xbm"
# include "eightF.xbm"
# include "nineF.xbm"
# include "tenF.xbm"
# include "elevenF.xbm"
# include "twelveF.xbm"
# include "thirteenF.xbm"
# include "fourteenF.xbm"
# include "fifteenF.xbm"
# include "colonF.xbm"
# include "underbarF.xbm"
# include "slashF.xbm"
FONT(F);

static const struct raw_number * const all_numbers[] = {
 /* Ordered according to -builtin, -builtin1, etc. */
 numbers_F,
 numbers_E,
 numbers_D,
 numbers_C,
 numbers_B,
};

#endif /* BUILTIN_FONTS */

#undef countof
#define countof(x) (sizeof((x))/sizeof(*(x)))

static char *window_title;
static char *font_name;
static int twelve_hour_time;
static int be_a_pig;
static int minutes_only;
static int show_date_initally;
static enum { MMDDYY, DDMMYY, YYMMDD, YYDDMM, MMYYDD, DDYYMM } date_format;
static int hex_time;

static Pixmap pixmap;
static GC pixmap_draw_gc, pixmap_erase_gc;
static GC window_draw_gc, window_erase_gc;
static XColor fg_color, bg_color, bd_color;

static struct frame *base_frames [16];
static struct frame *orig_frames [6];
static struct frame *current_frames [6];
static struct frame *target_frames [6];
static struct frame *clear_frame;
static int character_width, character_height;
static int x_offsets [6];
static int window_offset_x, window_offset_y;
static int wander_x, wander_y, wander_delta_x, wander_delta_y;
static Pixmap colon, slash;
static int colon_char_width;
static int time_digits [6];
static int last_time_digits [6];

static enum date_state { DTime, DDateIn, DDate, DDateOut, DDateOut2,
                         DDash, DDash2 }
  display_date;

static Pixmap memory_pig_zeros [15] [10];
static Pixmap memory_pig_digits [14] [10];
static Pixmap total_oinker_digits [15] [10];

#undef MAX
#undef MIN
#define MAX(a, b) ((a)>(b)?(a):(b))
#define MIN(a, b) ((a)<(b)?(a):(b))

#define NUM_DIGITS (hex_time ? 16 : 10)
#define SECS_PER_MIN (hex_time ? 0xf : 60)

typedef short unsigned int POS;

/* Number of horizontal segments/line.  Enlarge this if you are trying
   to use a font that is too "curvy" for XDaliClock to cope with.
   This code was sent to me by Dan Wallach <c169-bg@auriga.berkeley.edu>.
   I'm highly opposed to ever using statically-sized arrays, but I don't
   really feel like hacking on this code enough to clean it up.
 */
#ifndef MAX_SEGS_PER_LINE
#define MAX_SEGS_PER_LINE 5
#endif

struct scanline {
  POS left[MAX_SEGS_PER_LINE], right[MAX_SEGS_PER_LINE];
};

struct frame {
  struct scanline scanlines [1]; /* scanlines are contiguous here */
};


static struct frame *
image_to_frame (XImage *image)
{
  register int y, x;
  struct frame *frame;
  int width = image->width;
  int height = image->height;
  POS *left, *right;
/*  int *nsegs; */
  int maxsegments = 0;

  frame = (struct frame *)
    malloc (sizeof (struct frame) +
            (sizeof (struct scanline) * (height - 1)));
/*  nsegs = (int *) malloc (sizeof (int) * height); */

  for (y = 0; y < height; y++)
    {
      int seg, end;
      x = 0;
#define getbit(x) (XGetPixel (image, (x), y))
      left = frame->scanlines[y].left;
      right = frame->scanlines[y].right;

      for (seg = 0; seg < MAX_SEGS_PER_LINE; seg++)
        left [seg] = right [seg] = width / 2;

      for (seg = 0; seg < MAX_SEGS_PER_LINE; seg++)
        {
          for (; x < width; x++)
            if (getbit (x)) break;
          if (x == width) break;
          left [seg] = x;
          for (; x < width; x++)
            if (! getbit (x)) break;
          right [seg] = x;
        }

      for (; x < width; x++)
        if (getbit (x))
          {
            fprintf (stderr,
   "%s: font is too curvy.  Increase MAX_SEGS_PER_LINE (%d) and recompile.\n",
                     progname, MAX_SEGS_PER_LINE);
            exit (-1);
          }

      /* If there were any segments on this line, then replicate the last
         one out to the end of the line.  If it's blank, leave it alone,
         meaning it will be a 0-pixel-wide line down the middle.
       */
      end = seg;
      if (end > 0)
        for (; seg < MAX_SEGS_PER_LINE; seg++)
          {
            left [seg] = left [end-1];
            right [seg] = right [end-1];
          }
      if (end > maxsegments) maxsegments = end;
/*      nsegs [y] = end; */

#undef getbit
    }
  return frame;
}


/* This is kind of gross.
 */
static char *default_fonts [] = {
  "-*-minion-bold-i-*-*-*-500-*-*-*-*-*-*",
  "-*-new century schoolbook-bold-i-*-*-*-500-*-*-*-*-*-*",
  "-*-charter-bold-i-*-*-*-500-*-*-*-*-*-*",
  "-*-lucidabright-demibold-i-*-*-*-500-*-*-*-*-*-*",
  "-*-helvetica-bold-o-*-*-*-500-*-*-*-*-*-*",
  "-*-lucida-bold-i-*-*-*-500-*-*-*-*-*-*",
  "-*-adobe garamond-bold-i-*-*-*-500-*-*-*-*-*-*",
  "-*-palatino-bold-i-*-*-*-500-*-*-*-*-*-*",
  "-*-times-bold-i-*-*-*-500-*-*-*-*-*-*",

  "-*-charter-bold-i-*-*-*-360-*-*-*-*-*-*",
  "-*-new century schoolbook-bold-i-*-*-*-360-*-*-*-*-*-*",
  "-*-lucidabright-demibold-i-*-*-*-360-*-*-*-*-*-*",
  "-*-helvetica-bold-o-*-*-*-360-*-*-*-*-*-*",
  "-*-lucida-bold-i-*-*-*-360-*-*-*-*-*-*",

  "-*-charter-bold-i-*-*-*-240-*-*-*-*-*-*",
  "-*-new century schoolbook-bold-i-*-*-*-240-*-*-*-*-*-*",
  "-*-lucidabright-demibold-i-*-*-*-240-*-*-*-*-*-*",
  "-*-helvetica-bold-o-*-*-*-240-*-*-*-*-*-*",
  "-*-lucida-bold-i-*-*-*-240-*-*-*-*-*-*",     /* too wide.  bug? */

  "-*-charter-bold-i-*-*-*-180-*-*-*-*-*-*",
  "-*-new century schoolbook-bold-i-*-*-*-180-*-*-*-*-*-*",
  "-*-lucidabright-demibold-i-*-*-*-180-*-*-*-*-*-*",
  "-*-helvetica-bold-o-*-*-*-180-*-*-*-*-*-*",  /* too wide */
  "-*-lucida-bold-i-*-*-*-180-*-*-*-*-*-*",     /* too wide */
  0
};

static void
load_font (Screen *screen, char *fn)
{
  Display *dpy = DisplayOfScreen (screen);
  int i, max_lbearing, max_rbearing, max_ascent, max_descent;
  XFontStruct *font = 0;
  Pixmap pixmap;
  XImage *image = 0;
  XGCValues gcvalues;
  GC draw_gc, erase_gc;
  char **fonts = default_fonts;
  int bad_font_name = 0;

  if (fn)
    if (! (font = XLoadQueryFont (dpy, fn)))
      {
        bad_font_name = 1;
        fprintf (stderr, "%s: Couldn't load font \"%s\";\n", progname,
                 fn);
      }
  if (! font)
    for (; fonts; fonts++)
      if ((font = XLoadQueryFont (dpy, fn = *fonts)))
        break;
  if (bad_font_name && font)
    fprintf (stderr, " using font \"%s\" instead.\n", fonts [0]);
  else if (! font)
    {
      if (bad_font_name)
        fprintf (stderr, " couldn't load any of the default fonts either.\n");
      else
        fprintf (stderr, "%s: Couldn't load any of the default fonts.\n",
                 progname);
      exit (-1);
    }

  if (font->min_char_or_byte2 > '0' || font->max_char_or_byte2 < '9' ||
      (hex_time && (font->min_char_or_byte2 > 'A' || font->max_char_or_byte2 < 'F')))
    {
      fprintf (stderr, "%s: font %s doesn't contain all needed characters.\n",
               progname, fn);
      exit (-1);
    }
  max_lbearing = font->min_bounds.lbearing;
  max_rbearing = font->min_bounds.rbearing;
  max_ascent  = font->min_bounds.ascent;
  max_descent = font->min_bounds.descent;
  for (i = '0'; i <= '9'; i++)
    {
      XCharStruct *ch = (font->per_char
                         ? &font->per_char [i - font->min_char_or_byte2]
                         : &font->min_bounds);
      max_lbearing = MAX (max_lbearing, ch->lbearing);
      max_rbearing = MAX (max_rbearing, ch->rbearing);
      max_ascent  = MAX (max_ascent,  ch->ascent);
      max_descent = MAX (max_descent, ch->descent);
      if (ch->lbearing == ch->rbearing || ch->ascent == -ch->descent)
        {
          fprintf (stderr,
              "%s: char '%c' has bbox %dx%d (%d - %d x %d + %d) in font %s\n",
                   progname,
                   i, ch->rbearing - ch->lbearing, ch->ascent + ch->descent,
                   ch->rbearing, ch->lbearing, ch->ascent, ch->descent,
                   fn);
          exit (-1);
        }
    }
  if (hex_time) for (i = 'A'; i <= 'F'; i++)
    {
      XCharStruct *ch = (font->per_char
                         ? &font->per_char [i - font->min_char_or_byte2]
                         : &font->min_bounds);
      max_lbearing = MAX (max_lbearing, ch->lbearing);
      max_rbearing = MAX (max_rbearing, ch->rbearing);
      max_ascent  = MAX (max_ascent,  ch->ascent);
      max_descent = MAX (max_descent, ch->descent);
      if (ch->lbearing == ch->rbearing || ch->ascent == -ch->descent)
        {
          fprintf (stderr,
              "%s: char '%c' has bbox %dx%d (%d - %d x %d + %d) in font %s\n",
                   progname,
                   i, ch->rbearing - ch->lbearing, ch->ascent + ch->descent,
                   ch->rbearing, ch->lbearing, ch->ascent, ch->descent,
                   fn);
          exit (-1);
        }
    }
  character_width = max_rbearing + max_lbearing + 1; /* min enclosing rect */
  character_height = max_descent + max_ascent + 1;

  /* Now we know the combined bbox of the characters we're interested in;
     for each character, write it into a pixmap; grab the bits from that
     pixmap; and fill the scanline-buffers.
   */
  pixmap = XCreatePixmap (dpy, RootWindowOfScreen (screen),
                          character_width + 1, character_height + 1, 1);
  gcvalues.font = font->fid;
  gcvalues.foreground = 1L;
  draw_gc = XCreateGC (dpy, pixmap, GCFont | GCForeground, &gcvalues);
  gcvalues.foreground = 0L;
  erase_gc = XCreateGC (dpy, pixmap, GCForeground, &gcvalues);

  for (i = 0; i < NUM_DIGITS; i++)
    {
/*      XCharStruct *ch = (font->per_char
                         ? &font->per_char [i + '0' - font->min_char_or_byte2]
                         : &font->min_bounds);*/
      char s[1];
      s[0] = i >= 10 ? i - 10 + 'A' : i + '0';
      XFillRectangle (dpy, pixmap, erase_gc, 0, 0,
                      character_width + 1, character_height + 1);
      XDrawString (dpy, pixmap, draw_gc, max_lbearing, max_ascent, s, 1);
      if (! image)
        image = XGetImage (dpy, pixmap, 0, 0,
                           character_width, character_height, 1, XYPixmap);
      else
        XGetSubImage (dpy, pixmap, 0, 0,
                      character_width, character_height, 1, XYPixmap,
                      image, 0, 0);
      base_frames [i] = image_to_frame (image);
    }

  {
    XCharStruct *ch1, *ch2;
    int maxl, maxr, w;
    char s[2] = " ";
    if (font->per_char)
      {
        ch1 = &font->per_char [COLON_CHAR - font->min_char_or_byte2];
        ch2 = &font->per_char [SLASH_CHAR - font->min_char_or_byte2];
      }
    else
      ch1 = ch2 = &font->min_bounds;

    maxl = MAX (ch1->lbearing, ch2->lbearing);
    maxr = MAX (ch1->rbearing, ch2->rbearing);
    w = maxr + maxl + 1;
    colon =
      XCreatePixmap (dpy, RootWindowOfScreen (screen),
                     w+1, character_height+1, 1);
    slash =
      XCreatePixmap (dpy, RootWindowOfScreen (screen),
                     w+1, character_height+1, 1);
    XFillRectangle (dpy, colon, erase_gc, 0, 0, w+1, character_height+1);
    XFillRectangle (dpy, slash, erase_gc, 0, 0, w+1, character_height+1);
    s[0] = COLON_CHAR; XDrawString (dpy, colon, draw_gc, maxl+1, max_ascent, s, 1);
    s[0] = SLASH_CHAR; XDrawString (dpy, slash, draw_gc, maxl+1, max_ascent, s, 1);
    colon_char_width = w;
  }

  XDestroyImage (image);
  XFreePixmap (dpy, pixmap);
  XFreeFont (dpy, font);
  XFreeGC (dpy, draw_gc);
  XFreeGC (dpy, erase_gc);
}

#ifdef BUILTIN_FONTS
static void
load_builtin_font (Screen *screen, Visual *visual, int which)
{
  Display *dpy = DisplayOfScreen (screen);
  int i;

  const struct raw_number *nums;
  XImage *image;

  if (which < 0 || which >= countof(all_numbers)) abort();
  nums = all_numbers[which];

  image =
    XCreateImage (dpy, visual,
                  1, XYBitmap, 0,       /* depth, format, offset */
                  (char *) 0,           /* data */
                  0, 0, 8, 0);          /* w, h, pad, bytes_per_line */
  /* This stuff makes me nervous, but XCreateBitmapFromData() does it too. */
  image->byte_order = LSBFirst;
  image->bitmap_bit_order = LSBFirst;

  character_width = character_height = 0;

  for (i = 0; i < NUM_DIGITS; i++)
    {
      const struct raw_number *number = &nums [i];
      character_width = MAX (character_width, number->width);
      character_height = MAX (character_height, number->height);
      image->width = number->width;
      image->height = number->height;
      image->data = (char *) number->bits;
      image->bytes_per_line = (number->width + 7) / 8;
      base_frames [i] = image_to_frame (image);
    }
  image->data = 0;
  XDestroyImage (image);

  colon_char_width = MAX (nums [NUMS_COLON_INDEX].width, nums [NUMS_SLASH_INDEX].width);
  colon = XCreateBitmapFromData (dpy, RootWindowOfScreen (screen),
                                 (char *) nums[NUMS_COLON_INDEX].bits,
                                 nums[NUMS_COLON_INDEX].width, nums[NUMS_COLON_INDEX].height);
  slash = XCreateBitmapFromData (dpy, RootWindowOfScreen (screen),
                                 (char *) nums[NUMS_SLASH_INDEX].bits,
                                 nums[NUMS_SLASH_INDEX].width, nums[NUMS_SLASH_INDEX].height);
}
#endif /* BUILTIN_FONTS */

/*  It doesn't matter especially what MAX_SEGS is -- it gets flushed
    when it might fill up -- this number is just for performance tuning
 */
#define MAX_SEGS MAX_SEGS_PER_LINE * 200
static XSegment segment_buffer [MAX_SEGS + 2];
static int segment_count = 0;

static void
flush_segment_buffer (Display *dpy, Drawable drawable, GC draw_gc)
{
  if (! segment_count) return;
  XDrawSegments (dpy, drawable, draw_gc, segment_buffer, segment_count);
  segment_count = 0;
}



static void
draw_frame (Display *dpy, struct frame *frame, Drawable drawable, 
            GC draw_gc, int x_off)
{
  register int y, x;
  for (y = 0; y < character_height; y++)
    {
      struct scanline *line = &frame->scanlines [y];
      for (x = 0; x < MAX_SEGS_PER_LINE; x++) {
        if (line->left[x] == line->right[x] ||
            (x > 0 &&
             line->left[x] == line->left[x-1] &&
             line->right[x] == line->right[x-1]))
          continue;
        segment_buffer [segment_count].x1 = x_off + line->left[x];
        segment_buffer [segment_count].x2 = x_off + line->right[x];
        segment_buffer [segment_count].y1 = y;
        segment_buffer [segment_count].y2 = y;
        segment_count++;

        if (segment_count >= MAX_SEGS)
          flush_segment_buffer (dpy, drawable, draw_gc);
      }
    }
}

static void
set_current_scanlines (struct frame *into_frame, struct frame *from_frame)
{
  register int i;
  for (i = 0; i < character_height; i++)
    into_frame->scanlines [i] = from_frame->scanlines [i];
}

static void
one_step (struct frame *orig_frame,
          struct frame *current_frame,
          struct frame *target_frame,
          int tick)
{
  struct scanline *orig   =    &orig_frame->scanlines [0];
  struct scanline *curr   = &current_frame->scanlines [0];
  struct scanline *target =  &target_frame->scanlines [0];
  int i = 0, x;
  int ticks = 10; /* #### */
  int tt = ticks+1-tick;

  if (!orig) return;

  for (i = 0; i < character_height; i++)
    {
# define STEP(field) \
         (curr->field = (orig->field \
                         + (((int) (target->field - orig->field)) \
                            * tt / ticks)))

      for (x = 0; x < MAX_SEGS_PER_LINE; x++)
        {
          STEP (left [x]);
          STEP (right[x]);
        }
      orig++;
      curr++;
      target++;
# undef STEP
    }
}

static char test_hack = 0; /* gag */

/* #define DRYRUN */


#ifdef DRYRUN
static void DUMP(Display *dpy, Window window, int tick)
{
  char file[255];
  char buf[255];
  char *s1 = "dalidump-%d%d:%d%d:%d%d:%d.rgb";
  char *s2 = ("xwd -id 0x%X | (xwdtopnm | ppmtogif > tmp.gif) 2>/dev/null"
              "; fromgif tmp.gif %s ; rm tmp.gif");
  if (last_time_digits[3] == -1) return;
  sprintf (file, s1,
           (time_digits[0] < 0 ? 0 : time_digits[0]), time_digits[1],
           time_digits[2], time_digits[3],
           time_digits[4], time_digits[5],
           tick);
  sprintf (buf, s2, window, file);
  fprintf (stderr, "%s\n", file);
  XSync (dpy, False);
  system (buf);
}
#endif /* DRYRUN */

static time_t
current_clock (void)
{
  struct timeval tv;
  struct tm *tp;
  if (gettimeofday(&tv, NULL) == -1)
    exit(-1);
  tp = localtime(&tv.tv_sec);
  tv.tv_sec += tp->tm_gmtoff;
  tv.tv_sec %= 24*60*60;
  return hex_time ? (512 * tv.tv_sec + 8 * tv.tv_usec / 15625) / 675 : tv.tv_sec;
}

static long
fill_time_digits (void)
{
#ifdef DRYRUN
  static long clock = 799995420L;
  clock++;
#else  /* !DRYRUN */
  time_t now = time(NULL);
  time_t clock = current_clock();
#endif /* !DRYRUN */
  if (test_hack)
    {
      time_digits [0] = time_digits [1] = time_digits [2] =
        time_digits [3] = time_digits [4] = time_digits [5] =
          (test_hack == '-' ? -1 : test_hack - '0');
      test_hack = 0;
    }
  else if (display_date == DTime ||
           display_date == DDash ||
           display_date == DDash2)
    {
      if (display_date == DDash)
        display_date = DDash2;
      else if (display_date == DDash2)
        display_date = DTime;
      if (hex_time)
        {
	  time_digits [5] = 0; /* not used */
	  time_digits [4] = (clock >> 0) & 0x0f;
	  time_digits [3] = (clock >> 4) & 0x0f;
	  time_digits [2] = (clock >> 8) & 0x0f;
	  time_digits [1] = (clock >> 12) & 0x0f;
	  time_digits [0] = (clock >> 16) & 0x0f; /* hopefully zero */
        }
      else
	{
	  struct tm *tm = localtime (&now);
	  if (countdown)
	    {
	      int delta = countdown - clock;
	      if (delta < 0) delta = -delta;
	      tm->tm_sec = delta % 60;
	      tm->tm_min = (delta / 60) % 60;
	      tm->tm_hour = (delta / (60 * 60)) % 100;
	      twelve_hour_time = 0;
	    }
          if (twelve_hour_time && tm->tm_hour > 12) tm->tm_hour -= 12;
          if (twelve_hour_time && tm->tm_hour == 0) tm->tm_hour = 12;
	  time_digits [0] = (tm->tm_hour - (tm->tm_hour % 10)) / 10;
	  time_digits [1] = tm->tm_hour % 10;
	  time_digits [2] = (tm->tm_min - (tm->tm_min % 10)) / 10;
	  time_digits [3] = tm->tm_min % 10;
	  time_digits [4] = (tm->tm_sec - (tm->tm_sec % 10)) / 10;
	  time_digits [5] = tm->tm_sec % 10;
        }
    }
  else
    {
      struct tm *tm = localtime (&now);
      int m0,m1,d0,d1,y0,y1;
      tm->tm_mon++; /* 0 based */
      m0 = (tm->tm_mon - (tm->tm_mon % 10)) / 10;
      m1 = tm->tm_mon % 10;
      d0 = (tm->tm_mday - (tm->tm_mday % 10)) / 10;
      d1 = tm->tm_mday % 10;
      y0 = tm->tm_year % 100;
      y0 = (y0 - (y0 % 10)) / 10;
      y1 = tm->tm_year % 10;

      if (display_date == DDateIn)
        display_date = DDate;
      else if (display_date == DDateOut)
        display_date = DDateOut2;
      else if (display_date == DDateOut2)
        display_date = DDash;
      else if (display_date == DDash)
        display_date = DDash2;

      switch (date_format)
        {
        case MMDDYY:
          time_digits [0] = m0; time_digits [1] = m1;
          time_digits [2] = d0; time_digits [3] = d1;
          time_digits [4] = y0; time_digits [5] = y1;
          break;
        case DDMMYY:
          time_digits [0] = d0; time_digits [1] = d1;
          time_digits [2] = m0; time_digits [3] = m1;
          time_digits [4] = y0; time_digits [5] = y1;
          break;
        case YYMMDD:
          time_digits [0] = y0; time_digits [1] = y1;
          time_digits [2] = m0; time_digits [3] = m1;
          time_digits [4] = d0; time_digits [5] = d1;
          break;
        case YYDDMM:
          time_digits [0] = y0; time_digits [1] = y1;
          time_digits [2] = d0; time_digits [3] = d1;
          time_digits [4] = m0; time_digits [5] = m1;
          break;
          /* These are silly, but are included for completeness... */
        case MMYYDD:
          time_digits [0] = m0; time_digits [1] = m1;
          time_digits [2] = y0; time_digits [3] = y1;
          time_digits [4] = d0; time_digits [5] = d1;
          break;
        case DDYYMM:
          time_digits [0] = d0; time_digits [1] = d1;
          time_digits [2] = y0; time_digits [3] = y1;
          time_digits [4] = m0; time_digits [5] = m1;
          break;
        }

    }

  if (twelve_hour_time && time_digits [0] == 0)
    time_digits [0] = -1;

  if (be_a_pig < 0)
    {
      int i, d[6];
      memcpy(d, time_digits, sizeof(d));
      for (i = 0; i < 6; i++)
        time_digits[i] = d[6-i-1];
    }

  return clock;
}


static void
fill_pig_cache (Display *dpy, Drawable drawable, struct frame *work_frame)
{
  int i;
  struct frame *orig_frame = 0;

  /* do `[1-9]' to `0'
     We have very little to gain by caching the `[347]' to `0' transitions,
     but what the hell.
   */
  for (i = 0; i < NUM_DIGITS-1; i++)
    {
      int tick;
      orig_frame = base_frames [0];
      set_current_scanlines (work_frame, orig_frame);
      for (tick = 9; tick >= 0; tick--)
        {
          Pixmap p = XCreatePixmap (dpy, drawable,
                                    character_width, character_height, 1);
          XFillRectangle (dpy, p, pixmap_erase_gc, 0, 0,
                          character_width, character_height);
          draw_frame (dpy, work_frame, p, pixmap_draw_gc, 0);
          flush_segment_buffer (dpy, p, pixmap_draw_gc);
          memory_pig_zeros [i] [9 - tick] = p;
          if (tick)
            one_step (orig_frame, work_frame, base_frames [i+1], tick);
        }
    }
  /* do `[1-8]' to `[2-9]' */
  for (i = 0; i < NUM_DIGITS-2; i++)
    {
      int tick;
      orig_frame = base_frames [i+1];
      set_current_scanlines (work_frame, orig_frame);
      for (tick = 9; tick >= 0; tick--)
        {
          Pixmap p = XCreatePixmap (dpy, drawable,
                                    character_width, character_height, 1);
          XFillRectangle (dpy, p, pixmap_erase_gc, 0, 0,
                          character_width, character_height);
          draw_frame (dpy, work_frame, p, pixmap_draw_gc, 0);
          flush_segment_buffer (dpy, p, pixmap_draw_gc);
          memory_pig_digits [i] [9 - tick] = p;
          if (tick)
            one_step (orig_frame, work_frame, base_frames [i+2], tick);
        }
    }
  if (be_a_pig > 1)
    /* do `[1-7]' to `[3-9]' and `9' to `1' */
    for (i = 0; i < NUM_DIGITS-1; i++)
      {
        int tick;
        if (i == NUM_DIGITS-3) continue; /* zero transitions are already done */
        orig_frame = base_frames [i+1];
        set_current_scanlines (work_frame, orig_frame);
        for (tick = 9; tick >= 0; tick--)
          {
            Pixmap p = XCreatePixmap (dpy, drawable,
                                      character_width, character_height, 1);
            XFillRectangle (dpy, p, pixmap_erase_gc, 0, 0,
                            character_width, character_height);
            draw_frame (dpy, work_frame, p, pixmap_draw_gc, 0);
            flush_segment_buffer (dpy, p, pixmap_draw_gc);
            total_oinker_digits [i] [9 - tick] = p;
            if (tick)
              one_step (orig_frame, work_frame, base_frames [(i+3)%NUM_DIGITS], tick);
          }
      }
}


static void
initialize_digits (Screen *screen, Visual *visual)
{
  int i, x;
#ifdef BUILTIN_FONTS
  if (use_builtin_font >= 0)
    load_builtin_font (screen, visual, use_builtin_font);
  else
#endif /* !BUILTIN_FONTS */
    load_font (screen, font_name);

  memset ((char *) memory_pig_zeros,  0, sizeof (memory_pig_zeros));
  memset ((char *) memory_pig_digits, 0, sizeof (memory_pig_digits));

  for (i = 0; i < 6; i++)
    current_frames [i] = (struct frame *)
      malloc (sizeof (struct frame) +
              (sizeof (struct scanline) * (character_height - 1)));

  clear_frame = (struct frame *)
    malloc (sizeof (struct frame) +
            (sizeof (struct scanline) * (character_height - 1)));
  for (i = 0; i < character_height; i++)
    for (x = 0; x < MAX_SEGS_PER_LINE; x++)
      clear_frame->scanlines [i].left [x] =
        clear_frame->scanlines [i].right [x] = character_width / 2;

  x_offsets [0] = 0;
  x_offsets [1] = x_offsets [0] + character_width;
  x_offsets [2] = x_offsets [1] + character_width + colon_char_width;
  x_offsets [3] = x_offsets [2] + character_width;
  x_offsets [4] = x_offsets [3] + character_width + colon_char_width;
  x_offsets [5] = x_offsets [4] + character_width;

  wander_x = character_width / 2;
  wander_y = character_height / 2;
  wander_delta_x = wander_delta_y = 1;
}


static void
initialize_window (Screen *screen, Window window, Bool external_p)
{
  Display *dpy = DisplayOfScreen (screen);
  XWindowAttributes xgwa;
  XSetWindowAttributes attributes;
  unsigned long attribute_mask;
  XGCValues gcvalues;
  int ndigits = minutes_only ? 4 : (hex_time ? 5 : 6);

  XGetWindowAttributes (dpy, window, &xgwa);
#ifdef HAVE_SHAPE
  {
    int shape_event_base, shape_error_base;
    if ((do_shape || do_overlay) &&
        XShapeQueryExtension (dpy, &shape_event_base, &shape_error_base))
      ;
    else if (do_shape)
      {
        fprintf (stderr, "%s: no shape extension on this display\n", progname);
        do_shape = 0;
      }
  }
#endif /* HAVE_SHAPE */

  attribute_mask = (CWBackPixel | CWEventMask | CWDontPropagate);
  if (do_shape)
    attributes.background_pixel = fg_color.pixel;
  else
    attributes.background_pixel = bg_color.pixel;
  attributes.event_mask = (xgwa.your_event_mask |
                           KeyPressMask | StructureNotifyMask | ExposureMask );
  attributes.do_not_propagate_mask = 0;

  /* Select ButtonPress and ButtonRelease events on the window only if no other
     app has already selected them (only one app can select ButtonPress at a
     time: BadAccess results.)
   */
  if (! (xgwa.all_event_masks & (ButtonPressMask | ButtonReleaseMask)))
    attributes.event_mask |= (ButtonPressMask | ButtonReleaseMask);

  XChangeWindowAttributes (dpy, window, attribute_mask, &attributes);

  /* Center stuff in window correctly when first created */
  {
    int width = x_offsets [minutes_only ? 3 : (hex_time ? 4 : 5)] + character_width;
    if (do_shape || do_overlay)
      window_offset_x = window_offset_y = 0;
    else
      {
        window_offset_x = (xgwa.width - width) / 2;
        window_offset_y = (xgwa.height - character_height) / 2;
      }
  }

  if (! external_p)
    XStoreName (dpy, window, window_title);

  pixmap = XCreatePixmap (dpy, window,
                          x_offsets [ndigits-1] + character_width + 1,
                          character_height + 1, 1);

  gcvalues.foreground = fg_color.pixel;
  gcvalues.background = bg_color.pixel;
  gcvalues.function = GXcopy;
  gcvalues.graphics_exposures = False;
  window_draw_gc = XCreateGC (dpy, window,
                              GCForeground | GCBackground | GCFunction |
                              GCGraphicsExposures,
                              &gcvalues);
  gcvalues.foreground = bg_color.pixel;
  gcvalues.background = fg_color.pixel;
  window_erase_gc = XCreateGC (dpy, window,
                               GCForeground | GCBackground | GCFunction |
                               GCGraphicsExposures,
                               &gcvalues);
  gcvalues.foreground = 1;
  gcvalues.background = 0;
  pixmap_draw_gc = XCreateGC (dpy, pixmap,
                              GCForeground | GCBackground | GCFunction |
                              GCGraphicsExposures,
                              &gcvalues);
  gcvalues.foreground = 0;
  gcvalues.background = 1;
  pixmap_erase_gc = XCreateGC (dpy, pixmap,
                               GCForeground | GCBackground | GCFunction |
                               GCGraphicsExposures,
                               &gcvalues);

#ifdef HAVE_SHAPE
  if (do_overlay)
    {
      /* If we're doing overlays, set up a rectangular shape mask anyway --
         this informs the window manager (mwm and twm, at least) to not put
         a border around the window, but to just give it a titlebar.  Since
         we only update this shape mask when the window is resized, this
         isn't a big performance problem like updating it 10x/second is.
       */
      XFillRectangle (dpy, pixmap, pixmap_draw_gc,
                      0, 0, x_offsets [ndigits-1] + character_width + 1,
                      character_height + 1);
      XShapeCombineMask (dpy, window, ShapeBounding,
                         window_offset_x, window_offset_y, pixmap, ShapeSet);
    }
#endif /* HAVE_SHAPE */      


  XFillRectangle (dpy, pixmap, pixmap_erase_gc,
                  0, 0, x_offsets [ndigits-1] + character_width + 1,
                  character_height + 1);

  /* XClearWindow() depends on us being able to set the background
     color of the window, which we can't necessarily do if it is the
     root window.  So fill instead. */
  XFillRectangle (dpy, window, window_erase_gc, 0, 0, 32767, 32767);
}


static void
get_resources (Screen *screen)
{
  char *buf;

  /* #### maybe should be elsewhere */
  window_title = get_string_resource ("title", "Title");

  show_date_initally = get_boolean_resource ("showdate", "Showdate");
  display_date = show_date_initally ? DDate : DTime;

  minutes_only = !get_boolean_resource ("seconds", "Seconds");
  hex_time = get_boolean_resource ("hex", "Hex");
  font_name = get_string_resource ("font", "Font");
#ifdef BUILTIN_FONTS
  use_builtin_font = -1;
  if (!strcasecmp (font_name, "BUILTIN0")) use_builtin_font = 0;
  if (!strcasecmp (font_name, "BUILTIN1")) use_builtin_font = 1;
  if (!strcasecmp (font_name, "BUILTIN2")) use_builtin_font = 2;
  if (!strcasecmp (font_name, "BUILTIN" )) use_builtin_font = 2;
  if (!strcasecmp (font_name, "BUILTIN3")) use_builtin_font = 3;
  if (!strcasecmp (font_name, "BUILTIN4")) use_builtin_font = 4;
#endif /* BUILTIN_FONTS */

  buf = get_string_resource ("mode", "Mode");
  if (buf == 0 || !strcmp (buf, "12"))
    twelve_hour_time = 1;
  else if (!strcmp (buf, "24"))
    twelve_hour_time = 0;
  else
    {
      fprintf (stderr, "%s: mode must be \"12\" or \"24\", not %s.\n",
               progname, buf);
      twelve_hour_time = 1;
    }
  if (buf) free (buf);

  buf = get_string_resource ("memory", "Memory");
  if (buf == 0)
    be_a_pig = 0;
  else if (!strcasecmp (buf, "high") ||
           !strcasecmp (buf, "hi") ||
           !strcasecmp (buf, "sleazy"))
    be_a_pig = 2;
  else if (!strcasecmp (buf, "medium") ||
           !strcasecmp (buf, "med"))
    be_a_pig = 1;
  else if (!strcasecmp (buf, "low") ||
           !strcasecmp (buf, "lo"))
    be_a_pig = 0;
  else
    {
      fprintf (stderr,
         "%s: memory must be \"high\", \"medium\", or \"low\", not \"%s\".\n",
               progname, buf);
      be_a_pig = 0;
    }
  if (buf) free (buf);

  buf = get_string_resource ("datemode", "DateMode");
  if (buf == 0)
    date_format = MMDDYY;
  else if (!strcasecmp (buf, "mmddyy") || !strcasecmp (buf, "mm/dd/yy") ||
           !strcasecmp (buf, "mm-dd-yy"))
    date_format = MMDDYY;
  else if (!strcasecmp (buf, "ddmmyy") || !strcasecmp (buf, "dd/mm/yy") ||
           !strcasecmp (buf, "dd-mm-yy"))
    date_format = DDMMYY;
  else if (!strcasecmp (buf, "yymmdd") || !strcasecmp (buf, "yy/mm/dd") ||
           !strcasecmp (buf, "yy-mm-dd"))
    date_format = minutes_only ? MMDDYY : YYMMDD;
  else if (!strcasecmp (buf, "yyddmm") || !strcasecmp (buf, "yy/dd/mm") ||
           !strcasecmp (buf, "yy-dd-mm"))
    date_format = minutes_only ? DDMMYY : YYDDMM;
  else if (!strcasecmp (buf, "mmyydd") || !strcasecmp (buf, "mm/yy/dd") ||
           !strcasecmp (buf, "mm-yy-dd"))
    date_format = MMYYDD;
  else if (!strcasecmp (buf, "ddyymm") || !strcasecmp (buf, "dd/yy/mm") ||
           !strcasecmp (buf, "dd-yy-mm"))
    date_format = DDYYMM;
  else
    {
      fprintf (stderr,
         "%s: DateMode must be \"MM/DD/YY\", \"DD/MM/YY\", etc; not \"%s\"\n",
               progname, buf);
      date_format = MMDDYY;
    }
  if (buf) free (buf);
}



static void
wander (void)
{
  wander_x += wander_delta_x;
  if (wander_x == character_width || wander_x == 0)
    {
      wander_delta_x = -wander_delta_x;
      wander_y += wander_delta_y;
      if (wander_y == character_height || wander_y == 0)
        wander_delta_y = -wander_delta_y;
    }
}


static void
draw_colon (Display *dpy, Drawable pixmap, Window window)
{
  Pixmap glyph = (display_date == DTime ? colon : slash);
  XCopyPlane (dpy, glyph, pixmap, pixmap_draw_gc,
              0, 0, character_width, character_height,
              x_offsets [1] + character_width, 0, 1);
  if (! minutes_only)
    XCopyPlane (dpy, glyph, pixmap, pixmap_draw_gc,
                0, 0, character_width, character_height,
                x_offsets [3] + character_width, 0, 1);
  XStoreName (dpy, window, (glyph == slash ? hacked_version : window_title));
}


#define TICK_SLEEP()    clock_usleep (80000L)           /* 8/100th second */
#define SEC_SLEEP()     clock_usleep (1000000L)         /* 1 second */

/* The above pair of routines can't be implemented using a combination of
   sleep() and usleep() if you system has them, because they interfere with
   each other: using sleep() will cause later calls to usleep() to fail.
   The select() version is more efficient anyway (fewer system calls.)
 */


static int event_loop (Screen *, Window, Bool);

void
initialize_digital (Screen *screen, Visual *visual, Colormap cmap,
                    unsigned long *fgP, unsigned long *bgP, unsigned long *bdP,
                    unsigned int *widthP, unsigned int *heightP)
{
  int ndigits;
  get_resources (screen);
  initialize_digits (screen, visual);

  allocate_colors (screen, visual, cmap,
                   get_string_resource ("foreground", "Foreground"),
                   get_string_resource ("background", "Background"),
                   get_string_resource ("borderColor", "Foreground"),
                   &fg_color, &bg_color, &bd_color);
  *fgP = fg_color.pixel;
  *bgP = bg_color.pixel;
  *bdP = bd_color.pixel;

  ndigits = (minutes_only ? 4 : (hex_time ? 5 : 6));
  *widthP = x_offsets [ndigits-1] + character_width;
  *heightP = character_height;

  tzset(); /* to get timezone */
}


void
run_digital (Screen *screen, Window window, Bool external_p)
{
  Display *dpy = DisplayOfScreen (screen);
  int i;
  int ndigits = (minutes_only ? 4 : (hex_time ? 5 : 6));
  XWindowAttributes xgwa;
  XGetWindowAttributes (dpy, window, &xgwa);
  initialize_window (screen, window, external_p);

  if (be_a_pig > 0)
    fill_pig_cache (dpy, window, current_frames [0]);

  for (i = 0; i < ndigits; i++)
    {
      last_time_digits [i] = -1;
      set_current_scanlines (current_frames [i], clear_frame);
    }

  /* wait for initial mapwindow event */
  event_loop (screen, window, external_p);

  XFillRectangle (dpy, window, window_erase_gc,
                  0, 0, x_offsets [ndigits-1] + character_width,
                  character_height + 1);
  draw_colon (dpy, pixmap, window);

  for (i = 0; i < 6; i++)
    orig_frames [i] = current_frames [i];

  while (1)
    {
      int n, tick;
      time_t clock;
      int different_minute;
      enum date_state odate = display_date;

      clock = fill_time_digits ();

      different_minute = (time_digits [3] != last_time_digits [3]);
      if (odate != display_date)
        draw_colon (dpy, pixmap, window);

      for (n = 0; n < ndigits; n++)
        if (target_frames [n])
          orig_frames [n] = target_frames [n];

      for (n = 0; n < ndigits; n++)
        if (time_digits [n] == last_time_digits [n])
          target_frames [n] = 0;
        else if (time_digits [n] < 0)
          target_frames [n] = clear_frame;
        else
          target_frames [n] = base_frames [time_digits [n]];

      n = (be_a_pig >= 0 ? 0 : ndigits-1);
      if (twelve_hour_time && target_frames [n] && time_digits [n] == 0)
	{
	  target_frames [n] = clear_frame;
	  time_digits [n] = -1;
	}
      if (time_digits [n] < 0 && last_time_digits [n] < 0)
        target_frames [n] = 0;
      if (last_time_digits [n] == -2) /* evil hack for 12<->24 mode toggle */
        target_frames [n] = clear_frame;

      for (tick = 9; tick >= 0; tick--)
        {
          int j;
          if (do_cycle)
            cycle_colors (screen, xgwa.colormap, &fg_color, &bg_color,
                          window, window_draw_gc, window_erase_gc);
          for (j = 0; j < ndigits; j++)
            {
              if (target_frames [j])
                {
                  if (be_a_pig > 0 && time_digits [j] == 0 &&
                      last_time_digits [j] > 0)
                    {
                      Pixmap p =
                        memory_pig_zeros [last_time_digits [j] - 1] [tick];
                      XCopyPlane (dpy, p, pixmap, pixmap_draw_gc, 0, 0,
                                  character_width, character_height,
                                  x_offsets [j], 0, 1);
                    }
                  else if (be_a_pig > 0 && last_time_digits [j] == 0 &&
                           time_digits [j] > 0)
                    {
                      Pixmap p =
                        memory_pig_zeros [time_digits [j] - 1] [9 - tick];
                      XCopyPlane (dpy, p, pixmap, pixmap_draw_gc, 0, 0,
                                  character_width, character_height,
                                  x_offsets [j], 0, 1);
                    }
                  else if (be_a_pig > 0 && last_time_digits [j] >= 0 &&
                           time_digits [j] == last_time_digits [j] + 1)
                    {
                      Pixmap p =
                        memory_pig_digits [last_time_digits [j] - 1] [9-tick];
                      XCopyPlane (dpy, p, pixmap, pixmap_draw_gc, 0, 0,
                                  character_width, character_height,
                                  x_offsets [j], 0, 1);
                    }
                  /* This case isn't terribly common, but we've got it cached,
                     so why not use it. */
                  else if (be_a_pig > 0 && time_digits [j] >= 0 &&
                           last_time_digits [j] == time_digits [j] + 1)
                    {
                      Pixmap p =
                        memory_pig_digits [time_digits [j] - 1] [tick];
                      XCopyPlane (dpy, p, pixmap, pixmap_draw_gc, 0, 0,
                                  character_width, character_height,
                                  x_offsets [j], 0, 1);
                    }

                  else if (be_a_pig > 1 && last_time_digits [j] >= 0 &&
                           time_digits [j] == ((last_time_digits [j] + 2)
                                               % NUM_DIGITS))
                    {
                      Pixmap p =
                        total_oinker_digits [last_time_digits[j] - 1] [9-tick];
                      XCopyPlane (dpy, p, pixmap, pixmap_draw_gc, 0, 0,
                                  character_width, character_height,
                                  x_offsets [j], 0, 1);
                    }
                  else if (be_a_pig > 1 && time_digits [j] >= 0 &&
                           last_time_digits [j] == ((time_digits [j] + 2)
                                                    % NUM_DIGITS))
                    {
                      Pixmap p =
                        total_oinker_digits [time_digits [j] - 1] [tick];
                      XCopyPlane (dpy, p, pixmap, pixmap_draw_gc, 0, 0,
                                  character_width, character_height,
                                  x_offsets [j], 0, 1);
                    }
                  else
                    {
#if 0
                      if (be_a_pig > 0 && tick == 9)
                        fprintf (stderr, "cache miss!  %d -> %d\n",
                                 last_time_digits [j], time_digits [j]);
#endif
                      /* sends 20 bytes */
                      XFillRectangle (dpy, pixmap, pixmap_erase_gc,
                                      x_offsets [j], 0, character_width + 1,
                                      character_height);
                      draw_frame (dpy, current_frames [j],
                                  pixmap, pixmap_draw_gc, x_offsets [j]);
                    }
                  if (tick)
                    one_step (orig_frames[j], current_frames[j], 
                              target_frames[j], tick);
                }
            }
          /* sends up to 1k in non-pig mode */
          flush_segment_buffer (dpy, pixmap, pixmap_draw_gc);

#ifdef HAVE_SHAPE
          if (do_shape)
            XShapeCombineMask (dpy, window, ShapeBounding,
                               window_offset_x, window_offset_y,
                               pixmap, ShapeSet);
          else
#endif /* HAVE_SHAPE */
            /* sends 28 bytes */
            XCopyPlane (dpy, pixmap, window, window_draw_gc, 0, 0,
                        x_offsets [ndigits-1] + character_width,
                        character_height,
                        window_offset_x + (wander_x - (character_width / 2)),
                        window_offset_y + (wander_y - (character_height / 2)),
                        1);
          if (do_cycle && no_writable_cells)
            fill_borders (dpy, window, window_erase_gc,
                          window_offset_x + (wander_x - (character_width / 2)),
                          window_offset_y + (wander_y - (character_height /2)),
                          x_offsets [ndigits-1] + character_width,
                          character_height);

          XFlush (dpy);
#ifdef DRYRUN
          DUMP(dpy, window, 9-tick);
#else  /* !DRYRUN */
          TICK_SLEEP ();
#endif /* !DRYRUN */
        }
      memcpy (last_time_digits, time_digits, sizeof (time_digits));

      /* sends up to 1k in non-pig mode */
      flush_segment_buffer (dpy, pixmap, pixmap_draw_gc);

#ifdef HAVE_SHAPE
      if (do_shape)
        XShapeCombineMask (dpy, window, ShapeBounding,
                           window_offset_x, window_offset_y,
                           pixmap, ShapeSet);
      else
#endif /* HAVE_SHAPE */
      {
        if (wander_p && different_minute)
          {
            /* XClearWindow() depends on us being able to set the background
               color of the window, which we can't necessarily do if it is the
               root window.  So fill instead. */
            XFillRectangle (dpy, window, window_erase_gc,
                            0, 0, 32767, 32767);
            wander ();
          }
      }

      /* sends 28 bytes */
      XCopyPlane (dpy, pixmap, window, window_draw_gc, 0, 0,
                  x_offsets [ndigits-1] + character_width, character_height,
                  window_offset_x + (wander_x - (character_width / 2)),
                  window_offset_y + (wander_y - (character_height / 2)),
                  1);

      if (do_cycle && no_writable_cells)
        fill_borders (dpy, window, window_erase_gc,
                      window_offset_x + (wander_x - (character_width / 2)),
                      window_offset_y + (wander_y - (character_height / 2)),
                      x_offsets [ndigits-1] + character_width,
                      character_height);

#ifdef DRYRUN
      event_loop (screen, window, external_p);
#else /* !DRYRUN */
    if (minutes_only)
      {
        /* This is slightly sleazy: when in no-seconds mode, wake up
           once a second to cycle colors and poll for events, until the
           minute has expired.  We could do this with an interrupt timer
           or by select()ing on the display file descriptor, but that
           would be more work.
         */
        time_t now = current_clock();
        time_t target = clock + SECS_PER_MIN - clock%SECS_PER_MIN;
        while (now < target && now > clock) /* hack to allow midnight roll-overs */
          {
            /* if event_loop returns true, we need to go and repaint stuff
               right now, instead of waiting for the minute to elapse.
               */
            if (event_loop (screen, window, external_p))
              break;

            if (now >= target-1)        /* the home stretch; sync up */
              TICK_SLEEP ();
            else
              {
                if (do_cycle)
                  cycle_colors (screen, xgwa.colormap, &fg_color, &bg_color,
                                window, window_draw_gc, window_erase_gc);
                SEC_SLEEP ();
              }
	    now = current_clock();
          }
      }
    else
      {
        /* Sync to the second-strobe by repeatedly sleeping for about 1/10th
           second until time() returns a different value.  This is so that
           multiple instances of this program run in lock-step instead of
           being randomly staggered by up to a second, depending on when they
           were started, and scheduling delays.  (It's the details like this
           that make the difference between a True Hack and just another app.)
         */
        while (current_clock() == clock)
          {
            TICK_SLEEP ();
            /* if event_loop returns true, we need to go and repaint stuff
               right now, instead of waiting for the second to elapse.
             */
            if (event_loop (screen, window, external_p))
              break;
          }
      }
#endif /* !DRYRUN */
    }
}


static void
fill_borders (Display *dpy, Window window, GC gc,
              int x, int y, int width, int height)
{
  XRectangle rects[4];
  int i = 0;
  if (y > 0)
    {
      rects[i].x = 0;
      rects[i].y = 0;
      rects[i].width = 0x3FFF;
      rects[i].height = y;
      i++;
      rects[i].x = 0;
      rects[i].y = y + height;
      rects[i].width = 0x3FFF;
      rects[i].height = 0x3FFF;
      i++;
    }
  if (x > 0)
    {
      rects[i].x = 0;
      rects[i].y = y;
      rects[i].width = x;
      rects[i].height = height;
      i++;
      rects[i].x = x + width;
      rects[i].y = y;
      rects[i].width = 0x3FFF;
      rects[i].height = height;
      i++;
    }
  if (i > 0)
    XFillRectangles (dpy, window, gc, rects, i);
}

static int
event_loop (Screen *screen, Window window, Bool external_p)
{
  Display *dpy = DisplayOfScreen (screen);
  static int mapped_p = 0;
  int redraw_p = 0;
  XSync (dpy, False);

  while (XPending (dpy) || !(mapped_p || external_p))
    {
      XEvent event;
      XNextEvent (dpy, &event);

      /* Kludge to invert the sense of button clicks in -showdate mode */
      if (show_date_initally)
        {
          if (event.xany.type == ButtonPress)
            event.xany.type = ButtonRelease;
          else if (event.xany.type == ButtonRelease)
            event.xany.type = ButtonPress;
        }

      switch (event.xany.type)
        {
        case KeyPress:
          {
            KeySym keysym;
            XComposeStatus status;
            char buffer [10];
            int nbytes = XLookupString (&event.xkey, buffer, sizeof (buffer),
                                        &keysym, &status);
            if (nbytes == 0) break;
            if (nbytes != 1) buffer [0] = 0;
            switch (buffer [0]) {
            case 'q': case 'Q': case 3:
#ifdef VMS
              exit (1);
#else  /* !VMS */
              exit (0);
#endif /* !VMS */
            case ' ':
              twelve_hour_time = !twelve_hour_time;
              if (!hex_time && twelve_hour_time &&
                  time_digits [0] == 0 &&
                  time_digits [1] != 0)
                last_time_digits [0] = -2; /* evil hack */
              redraw_p = 1;
              break;
	    case 'h':
	      hex_time = !hex_time;
              redraw_p = 1;
	      break;
            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9': case '-':
              if (event.xkey.state)
                {
                  test_hack = buffer [0];
                  redraw_p = 1;
                  break;
                }
            default:
              XBell (dpy, 0);
              break;
            }
          }
          break;
        case ButtonPress:
          display_date = DDateIn;
          redraw_p = 1;
          draw_colon (dpy, pixmap, window);
          break;
        case ButtonRelease:
          if (display_date == DDate) /* turn off faster if already up */
            display_date = DDash;
          else if (display_date != DTime)
            display_date = DDateOut;

          redraw_p = 1;
          break;
        case ConfigureNotify:
          {
            int width = x_offsets [minutes_only ? 3 : (hex_time ? 4 : 5)] + character_width;

            window_offset_x = (event.xconfigure.width - width) / 2;
            window_offset_y = (event.xconfigure.height - character_height) / 2;

#ifdef HAVE_SHAPE
            if (do_overlay)
              {
                /* If we're doing overlays, set up a rectangular shape mask
                   anyway -- this informs the window manager (mwm and twm, at
                   least) to not put a border around the window, but to just
                   give it a titlebar.  Since we only update this shape mask
                   when the window is resized, this isn't a big performance
                   problem like updating it 10x/second is.
                 */
                int ndigits = (minutes_only ? 4 : (hex_time ? 5 : 6));
                int w = x_offsets [ndigits-1] + character_width;
                int h = character_height + 1;
                Pixmap p =
                  XCreatePixmap (dpy, RootWindowOfScreen (screen), w, h, 1);
                XFillRectangle (dpy, p, pixmap_draw_gc, 0, 0, w, h);
                XShapeCombineMask (dpy, window, ShapeBounding,
                                   window_offset_x, window_offset_y,
                                   p, ShapeSet);
                XFreePixmap (dpy, p);
              }
#endif /* !HAVE_SHAPE */


            /* XClearWindow() depends on us being able to set the background
               color of the window, which we can't necessarily do if it is the
               root window.  So fill instead. */
            XFillRectangle (dpy, window, window_erase_gc,
                            0, 0, 32767, 32767);
            redraw_p = 1;
          }
          break;
        case Expose:
        case MapNotify:
          mapped_p = 1;
          redraw_p = 1;
          break;
        case UnmapNotify:
          mapped_p = 0;
          break;
        }
    }
  return redraw_p;
}
