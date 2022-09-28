/* xdaliclock, Copyright © 1991-2022 Jamie Zawinski.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * This is startup initialization and the "application" class.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define _GNU_SOURCE  // For strcasestr in <string.h>

#include "version.h"
#include "app.h"
#include "window.h"
#include "prefs.h"

const char *progname;

struct _XDaliClock {
  GtkApplication parent;
};

typedef struct _XDaliClockPrivate XDaliClockPrivate;

struct _XDaliClockPrivate {
  XDaliClockPrefs *prefs;
  dali_config config;
# ifdef DO_SAVER
  unsigned long window_id;
# endif
};


G_DEFINE_TYPE_WITH_PRIVATE (XDaliClock, xdaliclock_app, GTK_TYPE_APPLICATION)

dali_config *
xdaliclock_config (XDaliClock *app)
{
  XDaliClockPrivate *priv = xdaliclock_app_get_instance_private (app);
  return &priv->config;
}


static void
xdaliclock_app_init (XDaliClock *app)
{
  XDaliClockPrivate *priv = xdaliclock_app_get_instance_private (app);
  dali_config *c = &priv->config;
  memset (c, 0, sizeof(*c));
}


static void
prefs_closed_cb (GtkWidget *widget, gpointer data)
{
  XDaliClock *app = XDALICLOCK_APP (data);
  XDaliClockPrivate *priv = xdaliclock_app_get_instance_private (app);
  priv->prefs = NULL;
}


static void
preferences_cb (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  XDaliClock *app = XDALICLOCK_APP (data);
  XDaliClockPrivate *priv = xdaliclock_app_get_instance_private (app);
  GtkWindow *win = gtk_application_get_active_window (GTK_APPLICATION (app));
  if (!priv->prefs) {
    priv->prefs = xdaliclock_app_prefs_new (XDALICLOCK_APP_WINDOW (win),
                                            &priv->config);
    g_signal_connect (priv->prefs, "destroy", G_CALLBACK(prefs_closed_cb), app);
  }
  gtk_window_present (GTK_WINDOW (priv->prefs));
}


static void
button_cb (GtkWidget *self, GdkEventButton *event, gpointer data)
{
  if (event->type != GDK_BUTTON_RELEASE)
    return;
  if (event->button != 2 && event->button != 3)
    return;

  XDaliClock *app = XDALICLOCK_APP (data);
  preferences_cb (NULL, NULL, app);
}


// Because of event-propagation bullshit, we can't read mouse events on
// both the app and the window, so window.c calls in to here.
void
xdaliclock_app_open_prefs (XDaliClock *app)
{
  preferences_cb (NULL, NULL, app);
}


static void
quit_cb (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  g_application_quit (G_APPLICATION (app));
}


static GActionEntry app_entries[] = {
  { "preferences", preferences_cb, NULL, NULL, NULL },
  { "quit",        quit_cb,        NULL, NULL, NULL }
};


static void
xdaliclock_app_startup (GApplication *app)
{
  const gchar *quit_accels[] = { "<Ctrl>Q", "Q", "q", NULL };

  G_APPLICATION_CLASS (xdaliclock_app_parent_class)->startup (app);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "app.quit",
                                         quit_accels);
}


static void
xdaliclock_app_activate (GApplication *app)
{
  XDaliClockPrivate *priv =
    xdaliclock_app_get_instance_private (XDALICLOCK_APP (app));

  XDaliClockWindow *win = xdaliclock_app_window_new (XDALICLOCK_APP (app),
                                                     &priv->config
# ifdef DO_SAVER
                                                     , priv->window_id
# endif
                                                     );
  g_signal_connect (win, "button-release-event", G_CALLBACK (button_cb), app);

# ifdef DO_SAVER
  if (! priv->window_id)
# endif
    gtk_window_present (GTK_WINDOW (win));
}


static void
xdaliclock_app_open (GApplication *app,
                     GFile **files, gint n_files,
                     const gchar *hint)
{
  GList *windows = gtk_application_get_windows (GTK_APPLICATION (app));
  if (windows)
    gtk_window_present (GTK_WINDOW (windows->data));
  else
    xdaliclock_app_activate (app);
}


static int
opts_cb (GApplication *app, GVariantDict *opts, gpointer data)
{
# ifdef DO_SAVER
  XDaliClockPrivate *priv =
    xdaliclock_app_get_instance_private (XDALICLOCK_APP (app));
# endif

  if (g_variant_dict_contains (opts, "version")) {
    fprintf (stderr, "%s\n", version+4);
    return 0;

# ifdef DO_SAVER
  } else if (g_variant_dict_contains (opts, "root")) {
    priv->window_id = ~0L;

    const char *xss_id = getenv ("XSCREENSAVER_WINDOW");
    if (xss_id && *xss_id) {
      unsigned long id = 0;
      char c;
      if (1 == sscanf (xss_id, " 0x%lx %c", &id, &c) ||
          1 == sscanf (xss_id, " %lu %c",   &id, &c)) {
        priv->window_id = id;
      }
    }
    return -1;

  } else if (g_variant_dict_contains (opts, "window-id")) {
    priv->window_id =
      g_variant_get_int64 (
        g_variant_dict_lookup_value (opts, "window-id", G_VARIANT_TYPE_INT64));
    return -1;
# endif // DO_SAVER

  } else {
    return -1;
  }
}


static void
xdaliclock_app_class_init (XDaliClockClass *class)
{
  G_APPLICATION_CLASS (class)->startup  = xdaliclock_app_startup;
  G_APPLICATION_CLASS (class)->activate = xdaliclock_app_activate;
  G_APPLICATION_CLASS (class)->open     = xdaliclock_app_open;
}

static XDaliClock *
xdaliclock_app_new (void)
{
  XDaliClock *app = g_object_new (XDALICLOCK_APP_TYPE,
                                  "application-id", "org.jwz.xdaliclock",
                                  NULL);

  g_application_add_main_option (G_APPLICATION (app), "version", 'v', 
                                 G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
                                 "Print the version number",
                                 NULL);
# ifdef DO_SAVER
  g_application_add_main_option (G_APPLICATION (app), "root", 0, 
                                 G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
                                 "Render to the root window (XScreenSaver)",
                                 NULL);
  g_application_add_main_option (G_APPLICATION (app), "window-id", 0, 
                                 G_OPTION_FLAG_NONE, G_OPTION_ARG_INT64,
                                 "Render to the X Window ID",
                                 NULL);
# endif // DO_SAVER
  g_signal_connect (app, "handle-local-options", G_CALLBACK (opts_cb), app);

  return app;
}


static void
logger (const gchar *domain, GLogLevelFlags log_level,
        const gchar *message, gpointer data)
{
  if (log_level & G_LOG_LEVEL_DEBUG) return;
  if (log_level & G_LOG_LEVEL_INFO) return;

  fprintf (stderr, "%s: %s: %s\n", progname, domain, message);

  if (strcasestr (message, "Settings schema") &&
      strcasestr (message, "not installed"))
    fprintf (stderr,
      "\n"
      "\tThe app won't launch unless you have run 'make install' to add a\n"
      "\tfile to /usr/share/glib-2.0/schemas/ and regenerate a system-wide\n"
      "\tdatabase first.  Yes, this is absurd.  GTK is a dumpster fire.\n"
      "\n"
#  if !defined(__OPTIMIZE__)
      "\tFor debugging, set $GSETTINGS_SCHEMA_DIR to the source directory.\n"
      "\n"
#  endif
    );
}


int
main (int argc, char *argv[])
{
  progname = argv[0];
  char *s = strrchr (progname, '/');
  if (s) progname = s+1;

  g_log_set_default_handler (logger, NULL);

# ifdef DO_SAVER
  /* Allow single-dash arguments for these, which might still be used in
     someone's .xscreensaver file. */
  int i;
  for (i = 1; i < argc; i++) {
    if      (!strcmp(argv[i], "-root"))      argv[i] = "--root";
    else if (!strcmp(argv[i], "-window-id")) argv[i] = "--window-id";
  }
#endif

  return g_application_run (G_APPLICATION (xdaliclock_app_new()), argc, argv);
}
