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

#ifndef __XDALICLOCK_WINDOW_H__
#define __XDALICLOCK_WINDOW_H__

#include "app.h"

#define XDALICLOCK_APP_WINDOW_TYPE (xdaliclock_app_window_get_type())
G_DECLARE_FINAL_TYPE (XDaliClockWindow, xdaliclock_app_window,
                      XDALICLOCK, APP_WINDOW, GtkApplicationWindow)
extern XDaliClockWindow *xdaliclock_app_window_new (XDaliClock *app,
                                                    dali_config *c
# ifdef DO_SAVER
                                                    , unsigned long window_id
# endif
                                                    );

#endif /* __XDALICLOCK_WINDOW_H__ */
