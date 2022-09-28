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

#ifndef __XDALICLOCK_PREFS_H__
#define __XDALICLOCK_PREFS_H__

#include <gtk/gtk.h>
#include "window.h"

#define XDALICLOCK_PREFS_TYPE (xdaliclock_app_prefs_get_type())
G_DECLARE_FINAL_TYPE (XDaliClockPrefs, xdaliclock_app_prefs, XDALICLOCK,
                      APP_PREFS, GtkDialog)
extern XDaliClockPrefs *xdaliclock_app_prefs_new (XDaliClockWindow *win,
                                                  dali_config *c);
extern void xdaliclock_app_prefs_load (GSettings *s, dali_config *c);

#endif /* __XDALICLOCK_PREFS_H__ */
