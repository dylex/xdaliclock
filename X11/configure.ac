# configure.in --- xdaliclock, Copyright © 1991-2022 Jamie Zawinski.
#

AC_PREREQ([2.69])
AC_INIT

echo "configure: current directory: `pwd`"
echo "configure: command line was: $0 $@"

AC_CONFIG_HEADERS([config.h])
AH_TEMPLATE([BUILTIN_FONTS], [Define this to use the builtin fonts.])
AH_TEMPLATE([DO_SAVER], [Define this to include XScreenSaver support.])
AH_TEMPLATE([HAVE_EGL],
  [Define this if OpenGL bindings are via EGL instead of GLX.])

DO_SAVER=yes

if ! test -z "$ac_unrecognized_opts" ; then
  echo "" >&2
  exit 2
fi

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_PATH_TOOL(pkg_config, pkg-config)
if test -z "$pkg_config" ; then
  AC_MSG_ERROR([pkg-config not found!])
  exit 1
fi

gtkv="gtk+-3.0"
giov="gio-2.0"

PKG_CHECK_MODULES([GTK], [$gtkv])
PKG_CHECK_MODULES([GIO], [$giov])
PKG_CHECK_MODULES([OPENGL], [gl])

if test "$DO_SAVER" = yes ; then
  AC_DEFINE(DO_SAVER)
  PKG_CHECK_MODULES([XLIB], [x11])
  PKG_CHECK_MODULES([EGL], [egl], [HAVE_EGL=yes], [HAVE_EGL=no])

  if test "$HAVE_EGL" = yes; then
    AC_DEFINE(HAVE_EGL)
    XLIB_CFLAGS="$XLIB_CFLAGS $EGL_CFLAGS"
    XLIB_LIBS="$XLIB_LIBS $EGL_LIBS"
  fi
fi

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AC_SUBST(OPENGL_CFLAGS)
AC_SUBST(OPENGL_LIBS)
AC_SUBST(XLIB_CFLAGS)
AC_SUBST(XLIB_LIBS)
AC_SUBST(INCLUDES)
AC_DEFINE(BUILTIN_FONTS)

AC_CACHE_CHECK([for Glib resource compiler], ac_cv_glib_res,
  [ac_cv_glib_res=`$pkg_config --variable=glib_compile_resources $giov`])
GLIB_COMPILE_RESOURCES="$ac_cv_glib_res"
AC_SUBST(GLIB_COMPILE_RESOURCES)

AC_CACHE_CHECK([for Glib schema compiler], ac_cv_glib_schema,
  [ac_cv_glib_schema=`$pkg_config --variable=glib_compile_schemas $giov`])
GLIB_COMPILE_SCHEMAS="$ac_cv_glib_schema"
AC_SUBST(GLIB_COMPILE_SCHEMAS)

if test -z "$GTK_LIBS" ; then
  AC_MSG_ERROR([GTK not found!])
  exit 1
fi

if test -z "$GTK_LIBS" ; then
  AC_MSG_ERROR([OpenGL not found!])
  exit 1
fi

if test -z "$GLIB_COMPILE_RESOURCES" -o -z "$GLIB_COMPILE_SCHEMAS" ; then
  AC_MSG_ERROR([Glib not found!])
  exit 1
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
