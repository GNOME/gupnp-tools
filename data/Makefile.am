SUBDIRS = pixmaps xml

shareddir = $(datadir)/gupnp-tools

dist_shared_DATA = gupnp-universal-cp.ui \
		   gupnp-network-light.ui

desktopdir = ${datadir}/applications
desktop_in_files = gupnp-universal-cp.desktop.in \
		   gupnp-network-light.desktop.in

if BUILD_AV
dist_shared_DATA += gupnp-av-cp.ui
desktop_in_files += gupnp-av-cp.desktop.in
endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
desktop_in_in_files = $(desktop_in_files:.desktop.in=.desktop.in.in)

%.desktop.in: %.desktop.in.in
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

# Substitute variables in a Desktop Entry file.
%.desktop : %.desktop.in
	$(AM_V_GEN) $(SED) -e 's|@VERSION[@]|$(VERSION)|g' \
					-e 's|@PKGDATADIR[@]|$(PKGDATADIR)|g' $< > $@

EXTRA_DIST = $(desktop_in_in_files) gupnp-av-cp.ui.in $(desktop_in_files)

CLEANFILES = $(desktop_DATA)

MAINTAINERCLEANFILES = Makefile.in
