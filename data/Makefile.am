SUBDIRS = pixmaps xml

shareddir = $(datadir)/gupnp-tools

dist_shared_DATA = gupnp-universal-cp.ui \
		   gupnp-network-light.ui

desktopdir = ${datadir}/applications
desktop_in_files = gupnp-universal-cp.desktop.in \
		   gupnp-network-light.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

if BUILD_AV
shared_DATA = gupnp-av-cp.ui
desktop_in_files += gupnp-av-cp.desktop.in
endif

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_in_files:.desktop.in=.desktop.in.in) gupnp-av-cp.ui.in

CLEANFILES = $(desktop_DATA)

MAINTAINERCLEANFILES = Makefile.in
