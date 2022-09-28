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
 * The preferences window.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "prefs.h"
#include "version.h"

struct _XDaliClockPrefs {
  GtkDialog parent;
};

typedef struct _XDaliClockPrefsPrivate XDaliClockPrefsPrivate;

struct _XDaliClockPrefsPrivate {
  GSettings *settings;
  dali_config *config;

  /* The widgets for editing each preference. */
  GtkWidget
    *hhmmss_radio, *hhmm_radio, *ss_radio,
    *h12_radio, *h24_radio,
    *mmddyy_radio, *ddmmyy_radio, *yymmdd_radio,
    *cycle_adj, *opacity_adj, *foreground, *background, *hidetitlebar,
    *countdownmode, *countdowntime, *countdownseconds,
    *aboutlabel;
};

G_DEFINE_TYPE_WITH_PRIVATE (XDaliClockPrefs, xdaliclock_app_prefs,
                            GTK_TYPE_DIALOG)


static void
timemode_cb (GtkWidget *widget, gpointer data)
{
  XDaliClockPrefs *prefs = (XDaliClockPrefs *) data;
  XDaliClockPrefsPrivate *priv =
    xdaliclock_app_prefs_get_instance_private (prefs);
  const char *val = 
    (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->ss_radio))
     ? "SS" :
     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->hhmm_radio))
     ? "HHMM" : "HHMMSS");
  g_settings_set_string (priv->settings, "timemode", val);
}


static void
hourmode_cb (GtkWidget *widget, gpointer data)
{
  XDaliClockPrefs *prefs = (XDaliClockPrefs *) data;
  XDaliClockPrefsPrivate *priv =
    xdaliclock_app_prefs_get_instance_private (prefs);
  const gchar *val =
    (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->h12_radio))
     ? "12" : "24");
  g_settings_set_string (priv->settings, "hourmode", val);
}


static void
datemode_cb (GtkWidget *widget, gpointer data)
{
  XDaliClockPrefs *prefs = (XDaliClockPrefs *) data;
  XDaliClockPrefsPrivate *priv =
    xdaliclock_app_prefs_get_instance_private (prefs);
  const char *val = 
    (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->mmddyy_radio))
     ? "MMDDYY" :
     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->ddmmyy_radio))
     ? "DDMMYY" : "YYMMDD");
  g_settings_set_string (priv->settings, "datemode", val);
}


static void
foreground_cb (GtkWidget *widget, gpointer data)
{
  XDaliClockPrefs *prefs = (XDaliClockPrefs *) data;
  XDaliClockPrefsPrivate *priv =
    xdaliclock_app_prefs_get_instance_private (prefs);
  GdkRGBA rgba;
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (priv->foreground), &rgba);
  /* gchar *val = gdk_rgba_to_string (&rgba); */
  char val[100];
  sprintf (val, "#%02X%02X%02X%02X",
           (int) (0xFF * rgba.red),
           (int) (0xFF * rgba.green),
           (int) (0xFF * rgba.blue),
           (int) (0xFF * rgba.alpha));
  g_settings_set_string (priv->settings, "foreground", val);
}


static void
background_cb (GtkWidget *widget, gpointer data)
{
  XDaliClockPrefs *prefs = (XDaliClockPrefs *) data;
  XDaliClockPrefsPrivate *priv =
    xdaliclock_app_prefs_get_instance_private (prefs);
  GdkRGBA rgba;
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (priv->background), &rgba);
  /* gchar *val = gdk_rgba_to_string (&rgba); */
  char val[100];
  sprintf (val, "#%02X%02X%02X%02X",
           (int) (0xFF * rgba.red),
           (int) (0xFF * rgba.green),
           (int) (0xFF * rgba.blue),
           (int) (0xFF * rgba.alpha));
  g_settings_set_string (priv->settings, "background", val);
}


/* Parses "#FFFF" and "#FFFFFFFF" as RGBA, which gdk_rgba_parse does not do.
 */
static gboolean
xdaliclock_gdk_rgba_parse (GdkRGBA *rgba, const gchar *spec)
{
  while (*spec == '#') spec++;
# define HEX(C) \
    ((C) >= '0' && (C) <= '9' ? (C) - '0' :      \
     (C) >= 'A' && (C) <= 'F' ? (C) - 'A' + 10 : \
     (C) >= 'a' && (C) <= 'f' ? (C) - 'a' + 10 : 0)
  switch (strlen (spec)) {
  case 3:
    rgba->alpha = 1.0;
   THREE:
    rgba->red   = HEX(spec[0]) / 8.0;
    rgba->green = HEX(spec[1]) / 8.0;
    rgba->blue  = HEX(spec[2]) / 8.0;
    break;
  case 4:
    rgba->alpha = HEX(spec[3]) / 8.0;
    goto THREE;
  case 6:
    rgba->alpha = 1.0;
   SIX:
    rgba->red   = (HEX(spec[0]) << 4 | HEX(spec[1])) / 255.0;
    rgba->green = (HEX(spec[2]) << 4 | HEX(spec[3])) / 255.0;
    rgba->blue  = (HEX(spec[4]) << 4 | HEX(spec[5])) / 255.0;
    break;
  case 8:
    rgba->alpha = (HEX(spec[6]) << 4 | HEX(spec[7])) / 255.0;
    goto SIX;
  default:
    return FALSE;
  }
  return TRUE;
}


/* Completely halfassed time-string parser, since GTK does not provide a
   date-and-time-selector widget.  Truly this is the year of the Linux Desktop.
 */
static time_t
xdaliclock_time_parse (const char *val)
{
  int yyyy = 0, mm = 0, dd = 0, hh = 0, min = 0, ss = 0;
  char c;
  time_t t = time ((time_t *) 0);
  struct tm *tm = localtime (&t);
  tm->tm_isdst = -1;
  if (6 == sscanf (val,					// YYYY-MM-DD HH:MM:SS
                   " %d - %d - %d %d : %d : %d %c",
                   &yyyy, &mm, &dd, &hh, &min, &ss, &c)) {
    if (yyyy < 1900) yyyy += 1900;
    tm->tm_year = yyyy - 1900;
    tm->tm_mon  = mm - 1;
    tm->tm_mday = dd;
    tm->tm_hour = hh;
    tm->tm_min  = min;
    tm->tm_sec  = ss;
    t = mktime (tm);
  } else if (5 == sscanf (val,				   // YYYY-MM-DD HH:MM
                          " %d - %d - %d %d : %d : %c",
                          &yyyy, &mm, &dd, &hh, &min, &c)) {
    if (yyyy < 1900) yyyy += 1900;
    tm->tm_year = yyyy - 1900;
    tm->tm_mon  = mm - 1;
    tm->tm_mday = dd;
    tm->tm_hour = hh;
    tm->tm_min  = min;
    tm->tm_sec  = 0;
    t = mktime (tm);
  } else if (3 == sscanf (val, " %d - %d - %d %c",		 // YYYY-MM-DD
                          &yyyy, &mm, &dd, &c)) {
    if (yyyy < 1900) yyyy += 1900;
    tm->tm_year = yyyy - 1900;
    tm->tm_mon  = mm - 1;
    tm->tm_mday = dd;
    tm->tm_hour = 0;
    tm->tm_min  = 0;
    tm->tm_sec  = 0;
    t = mktime (tm);
  } else if (3 == sscanf (val, " %d : %d : %d %c",		   // HH:MM:SS
                          &hh, &min, &ss, &c)) {
    tm->tm_hour = hh;
    tm->tm_min  = min;
    tm->tm_sec  = ss;
    t = mktime (tm);
  } else if (2 == sscanf (val, " %d : %d : %c",			      // HH:MM
                          &hh, &min, &c)) {
    tm->tm_hour = hh;
    tm->tm_min  = min;
    tm->tm_sec  = 0;
    t = mktime (tm);
  } else {
    memset (tm, 0, sizeof(*tm));
    t = 0;
  }

# if 0
  fprintf(stderr,"## \"%s\" => %04d-%02d-%02d %02d:%02d:%02d %lu\n", val,
          tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
          tm->tm_hour, tm->tm_min, tm->tm_sec, (unsigned long)t);
# endif
  return t;
}


/* Flush any preferences changes into the dali_config struct that is
   shared with the renderer.
 */
static void
changed_1 (GSettings *settings, dali_config *config, char *key)
{
  if (!settings || !config) abort();
  if (!strcasecmp (key, "timemode")) {
    char *val = g_settings_get_string (settings, key);
    if      (!strcasecmp (val, "SS"))   config->time_mode = SS;
    else if (!strcasecmp (val, "HHMM")) config->time_mode = HHMM;
    else				config->time_mode = HHMMSS;
    free (val);

  } else if (!strcasecmp (key, "datemode")) {
    char *val = g_settings_get_string (settings, key);
    if      (!strcasecmp (val, "MMDDYY")) config->date_mode = MMDDYY;
    else if (!strcasecmp (val, "DDMMYY")) config->date_mode = DDMMYY;
    else				  config->date_mode = YYMMDD;
    free (val);

  } else if (!strcasecmp (key, "hourmode")) {
    char *val = g_settings_get_string (settings, key);
    config->twelve_hour_p = !strcasecmp (val, "12");
    free (val);

  } else if (!strcasecmp (key, "cyclespeed")) {
    gint val = g_settings_get_int (settings, key);
    config->max_cps = val;

  } else if (!strcasecmp (key, "windowopacity")) {
    gdouble val = g_settings_get_double (settings, key);
    config->window_opacity = val;

  } else if (!strcasecmp (key, "hidetitlebar")) {
    gboolean val = g_settings_get_boolean (settings, key);
    config->hide_titlebar_p = val;

  } else if (!strcasecmp (key, "countdownseconds")) {
    gboolean val = g_settings_get_boolean (settings, key);
    config->countdown_seconds_p = val;

  } else if (!strcasecmp (key, "countdownmode") ||
             !strcasecmp (key, "countdowntime")) {
    if (! g_settings_get_boolean (settings, "countdownmode")) {
      config->countdown = 0;
    } else {
      char *val = g_settings_get_string (settings, "countdowntime");
      config->countdown = xdaliclock_time_parse (val);
      free (val);
    }

  } else if (!strcasecmp (key, "foreground")) {
    char *val = g_settings_get_string (settings, key);
    GdkRGBA rgba;
    memset (&rgba, 0, sizeof(rgba));
    xdaliclock_gdk_rgba_parse (&rgba, val);
    config->fg.r = rgba.red;
    config->fg.g = rgba.green;
    config->fg.b = rgba.blue;
    config->fg.a = rgba.alpha;
    free (val);

  } else if (!strcasecmp (key, "background")) {
    char *val = g_settings_get_string (settings, key);
    GdkRGBA rgba;
    memset (&rgba, 0, sizeof(rgba));
    xdaliclock_gdk_rgba_parse (&rgba, val);
    config->bg.r = rgba.red;
    config->bg.g = rgba.green;
    config->bg.b = rgba.blue;
    config->bg.a = rgba.alpha;
    free (val);

  } else if (!strcasecmp (key, "geometry")) {
    // Nothing to do here.

  } else {
    fprintf (stderr, "%s: unknown preference \"%s\"\n", progname, key);
  }
}


static void
changed_cb (GSettings *settings, char *key, gpointer data)
{
  XDaliClockPrefs *prefs = (XDaliClockPrefs *) data;
  XDaliClockPrefsPrivate *priv =
    xdaliclock_app_prefs_get_instance_private (prefs);
  dali_config *config = priv->config;
  changed_1 (settings, config, key);
}



/* Copy the GSettings preferences into the dali_config struct, at startup.
 */
void
xdaliclock_app_prefs_load (GSettings *settings, dali_config *config)
{
  if (!settings || !config) abort();
  changed_1 (settings, config, "timemode");
  changed_1 (settings, config, "datemode");
  changed_1 (settings, config, "hourmode");
  changed_1 (settings, config, "cyclespeed");
  changed_1 (settings, config, "windowopacity");
  changed_1 (settings, config, "hidetitlebar");
  changed_1 (settings, config, "countdownseconds");
  changed_1 (settings, config, "countdownmode");
  changed_1 (settings, config, "countdowntime");
  changed_1 (settings, config, "foreground");
  changed_1 (settings, config, "background");
}


static void
xdaliclock_app_prefs_init (XDaliClockPrefs *prefs)
{
  XDaliClockPrefsPrivate *priv =
    xdaliclock_app_prefs_get_instance_private (prefs);

  gtk_widget_init_template (GTK_WIDGET (prefs));
  priv->settings = g_settings_new ("org.jwz.xdaliclock");

  /* Apparently there's no way to link a radio group to the schema, because
     there is no property we can read from a single widget to get the value of
     the radio group as a whole.  So we have to do it manually with callbacks.
   */
  g_signal_connect (priv->hhmmss_radio, "toggled",
                    G_CALLBACK (timemode_cb), prefs);
  g_signal_connect (priv->hhmm_radio, "toggled",
                    G_CALLBACK (timemode_cb), prefs);
  g_signal_connect (priv->ss_radio, "toggled",
                    G_CALLBACK (timemode_cb), prefs);

  g_signal_connect (priv->h12_radio, "toggled",
                    G_CALLBACK (hourmode_cb), prefs);
  g_signal_connect (priv->h24_radio, "toggled",
                    G_CALLBACK (hourmode_cb), prefs);

  g_signal_connect (priv->mmddyy_radio, "toggled",
                    G_CALLBACK (datemode_cb), prefs);
  g_signal_connect (priv->ddmmyy_radio, "toggled",
                    G_CALLBACK (datemode_cb), prefs);
  g_signal_connect (priv->yymmdd_radio, "toggled",
                    G_CALLBACK (datemode_cb), prefs);

  /* Set the radio buttons according to the saved preference values.
   q*/
  char *timemode = g_settings_get_string (priv->settings, "timemode");
  char *hourmode = g_settings_get_string (priv->settings, "hourmode");
  char *datemode = g_settings_get_string (priv->settings, "datemode");

  if (!timemode || !strcasecmp (timemode, "HHMMSS"))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->hhmmss_radio), TRUE);
  else if (!strcasecmp (timemode, "HHMM"))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->hhmm_radio), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->ss_radio), TRUE);

  if (!hourmode || !strcasecmp (hourmode, "12"))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->h12_radio), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->h24_radio), TRUE);

  if (!datemode || !strcasecmp (datemode, "MMDDYY"))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->mmddyy_radio), TRUE);
  else if (!strcasecmp (datemode, "DDMMYY"))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->ddmmyy_radio), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->yymmdd_radio), TRUE);

  free (timemode);
  free (hourmode);
  free (datemode);

  /* Color selectors are also annoying.
   */
  char *fg = g_settings_get_string (priv->settings, "foreground");
  char *bg = g_settings_get_string (priv->settings, "background");
  GdkRGBA fg_rgba, bg_rgba;
  xdaliclock_gdk_rgba_parse (&fg_rgba, fg);
  xdaliclock_gdk_rgba_parse (&bg_rgba, bg);
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (priv->foreground), &fg_rgba);
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (priv->background), &bg_rgba);
  g_signal_connect (priv->foreground, "color-set",
                    G_CALLBACK (foreground_cb), prefs);
  g_signal_connect (priv->background, "color-set",
                    G_CALLBACK (background_cb), prefs);
  free (fg);
  free (bg);

  /* Other widgets can be linked directly to the schema.
   */
  g_settings_bind (priv->settings, "countdownmode", priv->countdownmode,
                   "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (priv->settings, "countdowntime", priv->countdowntime,
                   "text", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (priv->settings, "countdownseconds", priv->countdownseconds,
                   "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (priv->settings, "cyclespeed", priv->cycle_adj,
                   "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (priv->settings, "windowopacity", priv->opacity_adj,
                   "value", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (priv->settings, "hidetitlebar", priv->hidetitlebar,
                   "active", G_SETTINGS_BIND_DEFAULT);

  /* Get a notification when anything changed, to update the config struct.
   */
  g_signal_connect (priv->settings, "changed", G_CALLBACK (changed_cb), prefs);

  /* Reformat version.h for the About page. */
  {
    int L = strlen (version) + 1;
    char *s0 = strdup (version + 4);
    char *s1 = strchr (s0, ' ');
    char *s2 = strchr (s1+1, ' ');
    char *s3 = strchr (s2+1, '(') + 1;
    char *s4 = strchr (s3+1, '-') + 1;
    char *s5 = strchr (s4+1, '-') + 1;
    char *s6 = strchr (s5+1, ')');
    char *s7 = strchr (s6+1, ' ');
    char *s8 = strchr (s7+1, ' ') + 1;
    char *s9 = strchr (s8+1, '(') - 1;
    char *sA = strchr (s9+1, ')');
    const char *url = "https://www.jwz.org/xdaliclock/";
    *s2 = 0;
    *s6 = 0;
    *s9 = 0;
    *sA = 0;
    char *vers = strdup (s0);
    char *date = strdup (s3);
    char *year = strdup (s5);
    char *name = strdup (s8);
    char *mail = strdup (s9+2);
    char *v = malloc (L + strlen(url) * 2 + 100);
    sprintf (v,
             "%s\nCopyright \xc2\xa9 1991-%s %s\n\n"
             "%s\n<a href=\"%s\">%s</a>\n%s",
             vers, year, name, mail, url, url, date);
    gtk_label_set_markup (GTK_LABEL (priv->aboutlabel), v);
    free (s0);
    free (vers);
    free (date);
    free (year);
    free (name);
    free (mail);
    free (v);
  }
}


static void
xdaliclock_app_prefs_dispose (GObject *object)
{
  XDaliClockPrefsPrivate *priv =
    xdaliclock_app_prefs_get_instance_private (XDALICLOCK_APP_PREFS (object));
  g_clear_object (&priv->settings);
  G_OBJECT_CLASS (xdaliclock_app_prefs_parent_class)->dispose (object);
}


static void
xdaliclock_app_prefs_class_init (XDaliClockPrefsClass *class)
{
  G_OBJECT_CLASS (class)->dispose = xdaliclock_app_prefs_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/jwz/xdaliclock/prefs.ui");

  /* Link the widget objects to the instance object. */

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs, hhmmss_radio);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs, hhmm_radio);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs, ss_radio);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs, h12_radio);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs, h24_radio);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),

                                                XDaliClockPrefs, mmddyy_radio);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs, ddmmyy_radio);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs, yymmdd_radio);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs, cycle_adj);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs, opacity_adj);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs, hidetitlebar);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs, foreground);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs, background);

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs,
                                                countdownmode);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs,
                                                countdowntime);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs,
                                                countdownseconds);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                XDaliClockPrefs,
                                                aboutlabel);
}


XDaliClockPrefs *
xdaliclock_app_prefs_new (XDaliClockWindow *win, dali_config *c)
{
  XDaliClockPrefs *prefs =
    g_object_new (XDALICLOCK_PREFS_TYPE, "transient-for", win,
                  "use-header-bar", TRUE, NULL);
  XDaliClockPrefsPrivate *priv =
    xdaliclock_app_prefs_get_instance_private (prefs);
  priv->config = c;
  return prefs;
}
