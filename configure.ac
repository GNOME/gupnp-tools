AC_PREREQ([2.63])
AC_INIT([gupnp-tools],
        [0.8.14],
        [https://bugzilla.gnome.org/enter_bug.cgi?product=gupnp&component=gupnp-tools],
        [gupnp-tools],
        [http://www.gupnp.org/])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR(src/common/icons.h)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 tar-ustar no-dist-gzip dist-xz foreign])
AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AX_CHECK_ENABLE_DEBUG([yes])

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC
AC_HEADER_STDC
AC_FUNC_MMAP

AX_IS_RELEASE([git-directory])
AX_COMPILER_FLAGS

# libtool
LT_PREREQ([2.2])
LT_INIT([])

GLIB_REQUIRED=2.24
GSSDP_REQUIRED=0.13.3
GUPNP_REQUIRED=0.20.14
SOUP_REQUIRED=2.42
GUPNP_AV_REQUIRED=0.5.5
GTK_REQUIRED=3.10
GTHREAD_REQUIRED=2.12
GTKSOURCEVIEW_REQUIRED=3.2.0
LIBXML_REQUIRED=2.0

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

PKG_CHECK_MODULES(LIBGSSDP, gssdp-1.0 >= $GSSDP_REQUIRED)
PKG_CHECK_MODULES(LIBGUPNP, gupnp-1.0 >= $GUPNP_REQUIRED)
PKG_CHECK_MODULES(LIBSOUP, libsoup-2.4 >= $SOUP_REQUIRED)
PKG_CHECK_MODULES(LIBXML, libxml-2.0 >= $LIBXML_REQUIRED)
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= $GTK_REQUIRED)
PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= $GTHREAD_REQUIRED)
PKG_CHECK_MODULES(GIO, glib-2.0 >= $GLIB_REQUIRED
                       gobject-2.0 >= $GLIB_REQUIRED
                       gio-2.0 >= $GLIB_REQUIRED)
PKG_CHECK_MODULES(GTKSOURCEVIEW, gtksourceview-3.0 >= $GTKSOURCEVIEW_REQUIRED,
                  AC_DEFINE([HAVE_GTK_SOURCEVIEW], [1], [Use GtkSourceView])
                  AC_SUBST([TEXT_VIEW],[GtkSourceView]),
                  AC_SUBST([TEXT_VIEW],[GtkTextView]))

if test "x$os_win32" = "xyes"; then
    AC_CHECK_TOOL(WINDRES, windres)
fi

AC_ARG_WITH( av,
  AS_HELP_STRING([--disable-av],[Don't build AV tools]),
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
AM_GNU_GETTEXT_VERSION([0.19.7])
AM_GNU_GETTEXT([external])
GETTEXT_PACKAGE=gupnp-tools
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], 
  [Define the gettext package to be used])

AS_AC_EXPAND(PKGDATADIR, "${datadir}/${PACKAGE}")

AC_SUBST([GLIB_COMPILE_RESOURCES], [`$PKG_CONFIG --variable glib_compile_resources gio-2.0`])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/av-cp/Makefile
src/common/Makefile
src/discover/Makefile
src/network-light/Makefile
src/universal-cp/Makefile
src/upload/Makefile
data/Makefile
data/didl-lite-dialog.ui
data/pixmaps/Makefile
data/xml/Makefile
po/Makefile.in
])
AC_OUTPUT
