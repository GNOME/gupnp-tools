SUBDIRS = pixmaps xml

shareddir = $(datadir)/gupnp-tools

dist_shared_DATA = \
	gupnp-universal-cp.ui \
	gupnp-network-light.ui

desktopdir = ${datadir}/applications
desktop_in_files = \
	gupnp-universal-cp.desktop.in \
	gupnp-network-light.desktop.in

if BUILD_AV
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

EXTRA_DIST = \
	$(desktop_in_in_files) \
	didl-lite-dialog.ui.in \
	gupnp-av-cp.ui \
	search-dialog-context.ui \
	search-dialog.ui \
	didl-lite-dialog.ui.in \
	$(desktop_in_files) \
	org.gupnp.Tools.AV-CP.gresource.xml \
	org.gupnp.Tools.Common.gresource.xml

CLEANFILES = $(desktop_DATA)

MAINTAINERCLEANFILES = Makefile.in
