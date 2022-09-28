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

#ifndef __XDALICLOCK_APP_H__
#define __XDALICLOCK_APP_H__

#include <gtk/gtk.h>
#include "xdaliclock.h"

#define XDALICLOCK_APP_TYPE (xdaliclock_app_get_type())
G_DECLARE_FINAL_TYPE (XDaliClock, xdaliclock_app, XDALICLOCK, APP,
                      GtkApplication)
extern void xdaliclock_app_open_prefs (XDaliClock *);
extern dali_config *xdaliclock_config (XDaliClock *);

#endif /* __XDALICLOCK_APP_H__ */
