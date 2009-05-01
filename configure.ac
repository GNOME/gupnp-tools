AC_PREREQ(2.53)
AC_INIT(gupnp-tools, 0.8, http://www.gupnp.org/)
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(src/common/icons.h)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_LIBTOOL
AC_FUNC_MMAP

GUPNP_REQUIRED=0.13
GUPNP_AV_REQUIRED=0.5
GTK_REQUIRED=2.16
GTHREAD_REQUIRED=2.12
GIO_REQUIRED=2.12
ICON_THEME_REQUIRED=2.20

AC_MSG_CHECKING([for Win32])
case "$host" in
    *-*-mingw*)
        os_win32=yes
        ;;
    *)
        os_win32=no
        ;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, [test $os_win32 = yes])

PKG_CHECK_MODULES(LIBGUPNP, gupnp-1.0 >= $GUPNP_REQUIRED)
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_REQUIRED)
PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= $GTHREAD_REQUIRED)
PKG_CHECK_MODULES(GNOME_ICON_THEME, gnome-icon-theme >= $ICON_THEME_REQUIRED)
PKG_CHECK_MODULES(GIO, gio-2.0 >= $GIO_REQUIRED)

if test "x$os_win32" = "xno"; then
    PKG_CHECK_MODULES(UUID, uuid)
else
    AC_SUBST([UUID_LIBS], ["-lrpcrt4"])
    AC_SUBST([WIN32_LDFLAGS], ["-subsystem windows"])
fi

AC_ARG_WITH( av,
  AC_HELP_STRING([--disable-av],[Don't build AV tools]),
  try_av=$withval, try_av=yes )

HAVE_LIBGUPNP_AV=no
if test x$try_av = xyes ; then
    dnl check for gupnp-av
    PKG_CHECK_MODULES(LIBGUPNP_AV, gupnp-av-1.0 >= $GUPNP_AV_REQUIRED ,
      [
        HAVE_LIBGUPNP_AV=yes
        AC_SUBST(LIBGUPNP_AV_CFLAGS)
        AC_SUBST(LIBGUPNP_AV_LIBS)
      ],
      [
        HAVE_LIBGUPNP_AV=no
        AC_MSG_WARN([gupnp-av $GUPNP_AV_REQUIRED or greater not found.])
        AC_MSG_WARN([AV tools will not be built.])
      ])
else
    AC_MSG_NOTICE([AV tools disabled.])
fi

if test x$HAVE_LIBGUPNP_AV = xno; then
  BUILD_AV=no
else
  BUILD_AV=yes
  AC_DEFINE(HAVE_LIBGUPNP_AV, , [Build AV tools])
fi

AC_SUBST(HAVE_LIBGUPNP_AV)
AM_CONDITIONAL(BUILD_AV, test x$BUILD_AV = xyes)

AC_PATH_PROG(GUPNP_BINDING_TOOL, gupnp-binding-tool)

# glib-genmarshal
GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

CFLAGS="$CFLAGS -Wall"

# Run uninstalled
AC_ARG_ENABLE(uninstalled,
	[  --enable-uninstalled          run uninstalled],,
        enable_uninstalled=no)
if test "x$enable_uninstalled" = "xyes"; then
        AC_MSG_WARN("You want to run the tools uninstalled only? Fine!")
fi

AM_CONDITIONAL([UNINSTALLED], [test "x$enable_uninstalled" = "xyes"])

# Gettext
GETTEXT_PACKAGE=gupnp-tools
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], 
  [Define the gettext package to be used])

ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT
AM_GLIB_DEFINE_LOCALEDIR([LOCALEDIR])

AC_OUTPUT([
Makefile
src/Makefile
src/av-browser/Makefile
src/av-cp/Makefile
src/common/Makefile
src/network-light/Makefile
src/universal-cp/Makefile
src/upload/Makefile
data/Makefile
data/gupnp-universal-cp.desktop.in
data/gupnp-av-browser.desktop.in
data/gupnp-av-cp.desktop.in
data/gupnp-network-light.desktop.in
data/pixmaps/Makefile
data/xml/Makefile
])
