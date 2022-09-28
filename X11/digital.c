/* xdaliclock - a melting digital clock
 * Copyright © 1991-2022 Jamie Zawinski <jwz@jwz.org>
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

#include "xdaliclock.h"

#define DO_TEXTURE_RGBA	/* Do RGBA instead of LUMINANCE_ALPHA */
#define BIGENDIAN	/* Bit ordering if creating a single-bit bitmap */

typedef unsigned short POS;
typedef unsigned char BOOL;

#ifdef BUILTIN_FONTS

/* static int use_builtin_font; */

struct raw_number {
  const unsigned char *bits;
  POS width, height;
};

#endif /* BUILTIN_FONTS */

#ifdef BUILTIN_FONTS

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
  { colon ## X ## _bits, colon ## X ## _width, colon ## X ## _height }, \
  { slash ## X ## _bits, slash ## X ## _width, slash ## X ## _height }, \
  { 0, }								\
}

# include "zeroA.xbm"
# include "oneA.xbm"
# include "twoA.xbm"
# include "threeA.xbm"
# include "fourA.xbm"
# include "fiveA.xbm"
# include "sixA.xbm"
# include "sevenA.xbm"
# include "eightA.xbm"
# include "nineA.xbm"
# include "colonA.xbm"
# include "slashA.xbm"
FONT(A);

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
# include "colonB.xbm"
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
# include "colonC.xbm"
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
# include "colonD.xbm"
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
# include "colonE.xbm"
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
# include "colonF.xbm"
# include "slashF.xbm"
FONT(F);

static const struct raw_number * const all_numbers[] = {
 numbers_A,
 numbers_B,
 numbers_C,
 numbers_D,
 numbers_E,
 numbers_F,
};

#endif /* BUILTIN_FONTS */

#undef countof
#define countof(x) (sizeof((x))/sizeof(*(x)))

/* Number of horizontal segments/line.  Enlarge this if you are trying
   to use a font that is too "curvy" for XDaliClock to cope with.
   This code was sent to me by Dan Wallach <c169-bg@auriga.berkeley.edu>.
   I'm highly opposed to ever using statically-sized arrays, but I don't
   really feel like hacking on this code enough to clean it up.
 */
#ifndef MAX_SEGS_PER_LINE
# define MAX_SEGS_PER_LINE 5
#endif

struct scanline {
  POS left[MAX_SEGS_PER_LINE], right[MAX_SEGS_PER_LINE];
};

struct frame {
  struct scanline scanlines [1]; /* scanlines are contiguous here */
};


/* The runtime settings (some initialized from system prefs, but changable.)
 */
struct render_state {
  unsigned int last_secs;
  unsigned int current_msecs;
  int char_width, char_height, colon_width;
  struct frame *base_frames [12];	/* all digits */
  struct frame *orig_frames [8];	/* what was there */
  int           orig_digits [8];	/* what was there */
  struct frame *current_frames [8];	/* current intermediate animation */
  struct frame *target_frames [8];	/* where we are going */
  int           target_digits [8];	/* where we are going */
  struct frame *empty_frame;
  struct frame *empty_colon;
};


static struct frame *
make_empty_frame (int width, int height)
{
  int size = sizeof (struct frame) + (sizeof (struct scanline) * height);
  struct frame *frame;
  int x, y;

  frame = (struct frame *) calloc (size, 1);
  for (y = 0; y < height; y++)
    for (x = 0; x < MAX_SEGS_PER_LINE; x++)
      frame->scanlines[y].left [x] = frame->scanlines[y].right [x] = width / 2;
  return frame;
}


static struct frame *
copy_frame (dali_config *c, struct frame *from)
{
  struct render_state *state = c->render_state;
  int height = state->char_height;
  int size = sizeof (struct frame) + (sizeof (struct scanline) * height);
  struct frame *to = (struct frame *) calloc (size, 1);
  int y;
  for (y = 0; y < height; y++)
    to->scanlines[y] = from->scanlines[y];  /* copies the whole struct */
  return to;
}


static struct frame *
number_to_frame (const unsigned char *bits, int width, int height)
{
  int x, y;
  struct frame *frame;
  POS *left, *right;

  frame = make_empty_frame (width, height);

  for (y = 0; y < height; y++)
    {
      int seg, end;
      x = 0;
# define GETBIT(bits,x,y) \
         (!! ((bits) [((y) * ((width+7) >> 3)) + ((x) >> 3)] \
              & (1 << ((x) & 7))))

      left = frame->scanlines[y].left;
      right = frame->scanlines[y].right;

      for (seg = 0; seg < MAX_SEGS_PER_LINE; seg++)
        left [seg] = right [seg] = width / 2;

      for (seg = 0; seg < MAX_SEGS_PER_LINE; seg++)
        {
          for (; x < width; x++)
            if (GETBIT (bits, x, y)) break;
          if (x == width) break;
          left [seg] = x;
          for (; x < width; x++)
            if (! GETBIT (bits, x, y)) break;
          right [seg] = x;
        }

      for (; x < width; x++)
        if (GETBIT (bits, x, y))
          {
            fprintf (stderr, "%s: font is too curvy\n", progname);
            /* Increase MAX_SEGS_PER_LINE and recompile. */
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

# undef GETBIT
    }

  return frame;
}


static int
pick_font_size (dali_config *c, unsigned int *w_ret, unsigned int *h_ret)
{
#ifdef BUILTIN_FONTS
  int nn, cc;
  int f;
  unsigned int w, h;
  unsigned int ww = (w_ret ? *w_ret : c->width);
  unsigned int hh = (h_ret ? *h_ret : c->height);

  switch (c->time_mode)
    {
    case SS:     nn = 2; cc = 0; break;
    case HHMM:   nn = 4; cc = 1; break;
    case HHMMSS: nn = 6; cc = 2; break;
    default:   abort(); break;
    }

  for (f = 0; f < countof(all_numbers); f++)
    {
      w = ((all_numbers[f][0].width * nn) +
           (all_numbers[f][10].width * cc));
      h = all_numbers[f][0].height;
      if (w <= ww && h <= hh) break;
    }
  if (f >= countof(all_numbers))
    f = countof(all_numbers)-1;

  w += 2;  /* Leave a single pixel margin in the overall bitmap */
  h += 2;

  w = ((w + 7) / 8) * 8;  /* round up to byte */

#else  /* !BUILTIN_FONTS */
  int w = 0, h = 0, f = 0;
#endif /* !BUILTIN_FONTS */
  
  if (w_ret) *w_ret = w;
  if (h_ret) *h_ret = h;
  return f;
}


static void
init_numbers (dali_config *c)
{
  struct render_state *state = c->render_state;
  int i;
#ifdef BUILTIN_FONTS
  const struct raw_number *raw;

  int size = pick_font_size (c, NULL, NULL);
  if (size >= countof(all_numbers)) abort();
  raw = all_numbers[size];

  state->char_width  = raw[0].width;
  state->char_height = raw[0].height;
  state->colon_width = raw[10].width;

  state->empty_frame = make_empty_frame (raw[0].width,  raw[0].height);
  state->empty_colon = make_empty_frame (raw[10].width, raw[10].height);

  for (i = 0; i < countof(state->base_frames); i++)
    state->base_frames [i] =
      number_to_frame (raw[i].bits, raw[i].width, raw[i].height);
#endif /* BUILTIN_FONTS */

  memset (state->orig_frames,    0, sizeof(state->orig_frames));
  memset (state->current_frames, 0, sizeof(state->current_frames));
  memset (state->target_frames,  0, sizeof(state->target_frames));

  for (i = 0; i < countof(state->current_frames); i++) {
    int colonic_p = (i == 2 || i == 5);
    int cw = raw[colonic_p ? 10 : 0].width;
    int ch = raw[0].height;
    state->orig_frames[i]    = make_empty_frame (cw, ch);
    state->current_frames[i] = make_empty_frame (cw, ch);
    state->target_frames[i]  = make_empty_frame (cw, ch);
  }

  for (i = 0; i < countof(state->target_digits); i++)
    state->orig_digits[i] = state->target_digits[i] = -1;

  if (! c->bitmap) {
    if (c->bitmap_p) {
      c->bitmap = calloc (1, c->height2 * c->width / 8);
    } else {
#  ifdef DO_TEXTURE_RGBA
      c->bitmap = calloc (1, c->height2 * c->width2 * 4);
#  else
      c->bitmap = calloc (1, c->height2 * c->width2 * 2);
#  endif
    }
  }

  if (! c->bitmap) abort();
}


static void
free_numbers (dali_config *c)
{
  struct render_state *state = c->render_state;
  int i;
# define FREEIF(x) do { if ((x)) { free((x)); (x) = 0; } } while (0)
# define FREELOOP(x) do { \
    for (i = 0; i < countof ((x)); i++) FREEIF ((x)[i]); } while (0)

  FREELOOP (state->base_frames);
  FREELOOP (state->orig_frames);
  FREELOOP (state->current_frames);
  FREELOOP (state->target_frames);
  FREEIF (state->empty_frame);
  FREEIF (state->empty_colon);

# undef FREELOOP
# undef FREEIF
}


static void
fill_target_digits (dali_config *c, unsigned long time)
{
  struct render_state *state = c->render_state;
  struct tm *tm = localtime ((time_t *) &time);

  int i;
  int h = tm->tm_hour;
  int m = tm->tm_min;
  int s = tm->tm_sec;
  int D = tm->tm_mday;
  int M = tm->tm_mon + 1;
  int Y = tm->tm_year % 100;

  int twelve_p = c->twelve_hour_p;

  if (c->countdown)
    {
      long delta = ((unsigned long) c->countdown) - time;
      if (delta < 0) delta = -delta;
      s = delta % 60;
      m = (delta / 60) % 60;
      h = (delta / (60 * 60)) % 100;
      twelve_p = 0;
    }

  if (twelve_p) 
    {
      if (h > 12) { h -= 12; }
      else if (h == 0) { h = 12; }
    }

  for (i = 0; i < countof(state->target_digits); i++)
    state->target_digits[i] = -1;

  if (c->test_hack)
    {
      int a = (c->test_hack >= '0' && c->test_hack <= '9'
               ? c->test_hack - '0'
               : -1);
      state->target_digits [0] = a;
      state->target_digits [1] = a;
      state->target_digits [2] = 10;
      state->target_digits [3] = a;
      state->target_digits [4] = a;
      state->target_digits [5] = 10;
      state->target_digits [6] = a;
      state->target_digits [7] = a;
      c->test_hack = 0;
    }
  else if (!c->display_date_p)
    {
      switch (c->time_mode)
        {
        case SS:
          state->target_digits[0] = (s / 10);
          state->target_digits[1] = (s % 10);
          break;
        case HHMM:
          state->target_digits[0] = (h / 10);
          state->target_digits[1] = (h % 10);
          state->target_digits[2] = 10;		/* colon */
          state->target_digits[3] = (m / 10);
          state->target_digits[4] = (m % 10);
          if (twelve_p && state->target_digits[0] == 0)
            state->target_digits[0] = -1;
          break;
        case HHMMSS:
          state->target_digits[0] = (h / 10);
          state->target_digits[1] = (h % 10);
          state->target_digits[2] = 10;		/* colon */
          state->target_digits[3] = (m / 10);
          state->target_digits[4] = (m % 10);
          state->target_digits[5] = 10;		/* colon */
          state->target_digits[6] = (s / 10);
          state->target_digits[7] = (s % 10);
          if (twelve_p && state->target_digits[0] == 0)
            state->target_digits[0] = -1;
          break;
        default: 
          abort();
        }
    }
  else	/* date mode */
    {
      switch (c->date_mode) 
        {
        case MMDDYY:
          switch (c->time_mode) {
          case SS:
            state->target_digits[0] = (D / 10);
            state->target_digits[1] = (D % 10);
            break;
          case HHMM:
            state->target_digits[0] = (M / 10);
            state->target_digits[1] = (M % 10);
            state->target_digits[2] = 11;		/* dash */
            state->target_digits[3] = (D / 10);
            state->target_digits[4] = (D % 10);
            break;
          case HHMMSS:
            state->target_digits[0] = (M / 10);
            state->target_digits[1] = (M % 10);
            state->target_digits[2] = 11;		/* dash */
            state->target_digits[3] = (D / 10);
            state->target_digits[4] = (D % 10);
            state->target_digits[5] = 11;		/* dash */
            state->target_digits[6] = (Y / 10);
            state->target_digits[7] = (Y % 10);
            break;
          default:
            abort();
          }
          break;
        case DDMMYY:
          switch (c->time_mode) {
          case SS:
            state->target_digits[0] = (D / 10);
            state->target_digits[1] = (D % 10);
            break;
          case HHMM:
            state->target_digits[0] = (D / 10);
            state->target_digits[1] = (D % 10);
            state->target_digits[2] = 11;		/* dash */
            state->target_digits[3] = (M / 10);
            state->target_digits[4] = (M % 10);
            break;
          case HHMMSS:
            state->target_digits[0] = (D / 10);
            state->target_digits[1] = (D % 10);
            state->target_digits[2] = 11;		/* dash */
            state->target_digits[3] = (M / 10);
            state->target_digits[4] = (M % 10);
            state->target_digits[5] = 11;		/* dash */
            state->target_digits[6] = (Y / 10);
            state->target_digits[7] = (Y % 10);
            break;
          default:
            abort();
          }
          break;
        case YYMMDD:
          switch (c->time_mode) {
          case SS:
            state->target_digits[0] = (D / 10);
            state->target_digits[1] = (D % 10);
            break;
          case HHMM:
            state->target_digits[0] = (M / 10);
            state->target_digits[1] = (M % 10);
            state->target_digits[2] = 11;		/* dash */
            state->target_digits[3] = (D / 10);
            state->target_digits[4] = (D % 10);
            break;
          case HHMMSS:
            state->target_digits[0] = (Y / 10);
            state->target_digits[1] = (Y % 10);
            state->target_digits[2] = 11;		/* dash */
            state->target_digits[3] = (M / 10);
            state->target_digits[4] = (M % 10);
            state->target_digits[5] = 11;		/* dash */
            state->target_digits[6] = (D / 10);
            state->target_digits[7] = (D % 10);
            break;
          default:
            abort();
          }
          break;
        default:
          abort();
        }
    }
}


static void
draw_horizontal_line (dali_config *c, int x1, int x2, int y, BOOL black_p)
{
  unsigned char *scanline;
  if (x1 == x2) return;
  if (y > c->height) return;
  if (x1 > c->width) x1 = c->width;
  if (x2 > c->width) x2 = c->width;
  if (x1 > x2)
    {
      int swap = x1;
      x1 = x2;
      x2 = swap;
    }

  if (c->bitmap_p) {   				/* Single-bit bitmap */
  scanline = c->bitmap + (y * (c->width2 >> 3));

# ifdef BIGENDIAN
#  define B (7 - (x1 & 7))
# else
#  define B (x1 & 7)
# endif

  if (black_p)
    for (; x1 < x2; x1++)
      scanline[x1>>3] |= 1 << B;
  else
    for (; x1 < x2; x1++)
      scanline[x1>>3] &= ~(1 << B);

  } else {

    /* For OpenGL, instead of creating a single-bit bitmap, we create a
       pixmap with two bytes per pixel where the first byte is FF and
       the second byte is either FF or 00.  Also, the image is upside
       down, and the pixmap's dimensions are scaled up to the next
       highest power of 2.  This is natively usable as GL_LUMINANCE_ALPHA
       texture data.

       If we could use GL_INTENSITY textures, we could get away with
       1 byte per pixel instead of 2, but OpenGLES only implements
       GL_LUMINANCE_ALPHA, not GL_INTENSITY.

       But apparently GL_LUMINANCE_ALPHA has fallen out of favor with more
       recent versions of OpenGL, so maybe we need to waste 4 bytes instead,
       and do full RGBA (using only the values FFFFFFFF and FFFFFF00).
     */
#    undef NN
#    ifdef DO_TEXTURE_RGBA
#     define NN 2
#    else
#     define NN 1
#    endif
    scanline = c->bitmap + ((c->height-y-1) * c->width2 << NN) + (x1 << NN);
#    undef NN
    for (; x1 < x2; x1++)
      {
        *scanline++ = 0xFF;
#    ifdef DO_TEXTURE_RGBA
        *scanline++ = 0xFF;
        *scanline++ = 0xFF;
#    endif
        *scanline++ = black_p ? 0xFF : 0;
      }
  }
}


static int
draw_frame (dali_config *c, struct frame *frame, int x, int y, int colonic_p)
{
  struct render_state *state = c->render_state;
  int px, py;
  int cw = (colonic_p ? state->colon_width : state->char_width);

  for (py = 0; py < state->char_height; py++)
    {
      struct scanline *line = &frame->scanlines [py];
      int last_right = 0;

      for (px = 0; px < MAX_SEGS_PER_LINE; px++)
        {
          if (px > 0 &&
              (line->left[px] == line->right[px] ||
               (line->left [px] == line->left [px-1] &&
                line->right[px] == line->right[px-1])))
            continue;

          /* Erase the line between the last segment and this segment.
           */
          draw_horizontal_line (c,
                                x + last_right,
                                x + line->left [px],
                                y + py,
                                0);

          /* Draw the line of this segment.
           */
          draw_horizontal_line (c,
                                x + line->left [px],
                                x + line->right[px],
                                y + py,
                                1);

          last_right = line->right[px];
        }

      /* Erase the line between the last segment and the right edge.
       */
      draw_horizontal_line (c,
                            x + last_right,
                            x + cw,
                            y + py,
                            0);
    }
  return cw;
}


static void draw_clock (dali_config *c);

static void
start_sequence (dali_config *c, unsigned long time)
{
  struct render_state *state = c->render_state;
  int i;

  /* Move the (old) current_frames into the (new) orig_frames,
     since that's what's on the screen now. 
     Frames are freed as they expire out of orig_frames.
   */
  for (i = 0; i < countof (state->current_frames); i++)
    {
      if (state->orig_frames[i]) 
        free (state->orig_frames[i]);
      state->orig_frames[i]    = state->current_frames[i];
      state->current_frames[i] = state->target_frames[i];
      state->target_frames[i]  = 0;

      state->orig_digits[i]    = state->target_digits[i];
    }

  /* generate new target_digits */
  fill_target_digits (c, time);

  /* Fill the (new) target_frames from the (new) target_digits. */
  for (i = 0; i < countof (state->target_frames); i++)
    {
      int colonic_p = (i == 2 || i == 5);
      state->target_frames[i] =
        copy_frame (c,
                    (state->target_digits[i] == -1
                     ? (colonic_p ? state->empty_colon : state->empty_frame)
                     : state->base_frames[state->target_digits[i]]));
    }

  /* Render the current frame. */
  draw_clock (c);
}


static void
one_step (dali_config *c,
          struct frame *orig_frame,
          struct frame *current_frame,
          struct frame *target_frame,
          unsigned int msecs)
{
  struct render_state *state = c->render_state;
  struct scanline *orig   =    &orig_frame->scanlines [0];
  struct scanline *curr   = &current_frame->scanlines [0];
  struct scanline *target =  &target_frame->scanlines [0];
  int i = 0, x;

  for (i = 0; i < state->char_height; i++)
    {
# define STEP(field) \
         (curr->field = (orig->field \
                         + (((int) (target->field - orig->field)) \
                            * (int) msecs / 1000)))

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


static void
tick_sequence (dali_config *c)
{
  struct render_state *state = c->render_state;
  int i;

  struct timeval now;
  struct timezone tzp;
  gettimeofday (&now, &tzp);
  unsigned long secs = now.tv_sec;
  unsigned long msecs = now.tv_usec / 1000;

  if (!state->last_secs)
    state->last_secs = (int)secs;   /* fading in! */
  else if (secs != state->last_secs) 
    {
      /* End of the animation sequence; fill target_frames with the
         digits of the current time. */
      start_sequence (c, secs);
      state->last_secs = (unsigned int) secs;
    }

  /* Linger for about 1/10th second at the end of each cycle. */
  msecs *= 1.2;
  if (msecs > 1000) msecs = 1000;

  /* Construct current_frames by interpolating between
     orig_frames and target_frames. */
  for (i = 0; i < countof (state->current_frames); i++)
    one_step (c,
              state->orig_frames[i],
              state->current_frames[i],
              state->target_frames[i],
              (unsigned int) msecs);
  state->current_msecs = (unsigned int) msecs;
}


static void
compute_left_offset (dali_config *c)
{
  struct render_state *state = c->render_state;

  /* left_offset is so that the clock can be centered in the window
     when the leftmost digit is hidden (in 12-hour mode when the hour
     is 1-9).  When the hour rolls over from 9 to 10, or from 12 to 1,
     we animate the transition to keep the digits centered.
   */
  if (state->target_digits[0] == -1 &&		/* Fading in to no digit */
      state->orig_digits[1] == -1)
    c->left_offset = state->char_width / 2;
  else if (state->target_digits[0] != -1 &&	/* Fading in to a digit */
           state->orig_digits[1] == -1)
    c->left_offset = 0;
  else if (state->orig_digits[0] != -1 &&	/* Fading out from digit */
           state->target_digits[1] == -1)
    c->left_offset = 0;
  else if (state->orig_digits[0] != -1 &&	/* Fading out from no digit */
           state->target_digits[1] == -1)
    c->left_offset = state->char_width / 2;
  else if (state->orig_digits[0] == -1 &&	/* Anim no digit to digit. */
           state->target_digits[0] != -1)
    c->left_offset = state->char_width * (1000 - state->current_msecs) / 2000;
  else if (state->orig_digits[0] != -1 &&	/* Anim digit to no digit. */
           state->target_digits[0] == -1)
    c->left_offset = state->char_width * state->current_msecs / 2000;
  else if (state->target_digits[0] == -1)	/* No anim, no digit. */
    c->left_offset = state->char_width / 2;
  else						/* No anim, digit. */
    c->left_offset = 0;
}


static void
draw_clock (dali_config *c)
{
  struct render_state *state = c->render_state;
  int x, y, i, nn, cc;

  compute_left_offset (c);

  switch (c->time_mode)
    {
    case SS:     nn = 2; cc = 0; break;
    case HHMM:   nn = 4; cc = 1; break;
    case HHMMSS: nn = 6; cc = 2; break;
    default:     abort(); break;
    }

  x = y = 0;
  for (i = 0; i < nn+cc; i++) 
    {
      int colonic_p = (i == 2 || i == 5);
      x += draw_frame (c, state->current_frames[i], x, y, colonic_p);
    }
}


void
render_init (dali_config *c)
{
  if (c->render_state) abort();
  c->render_state = (struct render_state *)
    calloc (1, sizeof (struct render_state));
  init_numbers (c);
}

void
render_free (dali_config *c)
{
  free_numbers (c);
  if (c->render_state) free (c->render_state);
  if (c->bitmap) free (c->bitmap);
  c->render_state = 0;
  c->bitmap = 0;
}

void
render_once (dali_config *c)
{
  if (! c->render_state) abort();
  if (! c->bitmap) abort();
  tick_sequence (c);
  draw_clock (c);
}


/* return the next larger power of 2. */
static int
to_pow2 (int i)
{
  static const unsigned int pow2[] = { 
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 
    2048, 4096, 8192, 16384, 32768, 65536 };
  int j;
  for (j = 0; j < sizeof(pow2)/sizeof(*pow2); j++)
    if (pow2[j] >= i) return pow2[j];
  abort();  /* too big! */
}


void
render_bitmap_size (dali_config *c, 
                    unsigned int *w_ret, unsigned int *h_ret,
                    unsigned int *w2_ret, unsigned int *h2_ret)
{
  pick_font_size (c, w_ret, h_ret);
  if (c->bitmap_p) {
    if (w2_ret) *w2_ret = *w_ret;
    if (w2_ret) *h2_ret = *h_ret;
  } else {
    if (w2_ret) *w2_ret = to_pow2 (*w_ret);
    if (w2_ret) *h2_ret = to_pow2 (*h_ret);
  }
}
