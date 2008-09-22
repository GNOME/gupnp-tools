SUBDIRS = pixmaps xml

shareddir = $(datadir)/gupnp-tools

dist_shared_DATA = gupnp-universal-cp.glade \
		   gupnp-network-light.glade

desktopdir = ${datadir}/applications
desktop_in_files = gupnp-universal-cp.desktop.in \
		   gupnp-network-light.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

if BUILD_AV
dist_shared_DATA += gupnp-av-cp.glade
desktop_in_files += gupnp-av-cp.desktop.in
endif

# Rule to create the .desktop files
$(desktop_DATA): $(desktop_in_files) Makefile
	@sed -e "s,\$${prefix},${prefix},g" $(@:.desktop=.desktop.in) > $@

EXTRA_DIST = $(desktop_in_files:.desktop.in=.desktop.in.in)

CLEANFILES = $(desktop_DATA)

MAINTAINERCLEANFILES = Makefile.in
