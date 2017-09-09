if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(LIBGUPNP_CFLAGS) \
	    $(LIBGUPNP_AV_CFLAGS) \
	    $(GTK_CFLAGS) \
	    $(GTKSOURCEVIEW_CFLAGS) \
	    -I$(top_srcdir) -DDATA_DIR='"$(shareddir)"' \
	    -I$(top_srcdir)/src/common \
	    -DLOCALEDIR='"$(localedir)"' \
	    $(WARN_CFLAGS)

bin_PROGRAMS = gupnp-av-cp

RESOURCEFILE = $(top_srcdir)/data/org.gupnp.Tools.AV-CP.gresource.xml

resource_depends = \
	$(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies \
		--sourcedir=$(top_srcdir)/data --sourcedir=$(abs_top_builddir)/data \
		$(RESOURCEFILE))


BUILT_SOURCES = resource.c
CLEANFILES = resource.c
nodist_gupnp_av_cp_SOURCES = resource.c

resource.c: $(RESOURCEFILE) $(resource_depends)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(abs_top_srcdir)/data --sourcedir=$(abs_top_builddir)/data --generate $(RESOURCEFILE)


gupnp_av_cp_SOURCES = main.c \
		      gui.c \
		      playlist-treeview.c \
		      renderer-combo.c \
		      renderer-controls.c \
		      main.h \
		      gui.h \
		      playlist-treeview.h \
		      renderer-combo.h \
		      renderer-controls.h \
			  server-device.c \
			  server-device.h \
			  search-dialog.c \
			  search-dialog.h \
			  didl-dialog.h \
			  didl-dialog.c

gupnp_av_cp_LDADD = ../common/libutil.a \
		    $(LIBGSSDP_LIBS) \
		    $(LIBSOUP_LIBS) \
		    $(LIBXML_LIBS) \
		    $(LIBGUPNP_LIBS) \
		    $(LIBGUPNP_AV_LIBS) \
		    $(GTK_LIBS) \
		    $(GTKSOURCEVIEW_LIBS)
gupnp_av_cp_LDFLAGS = -export-dynamic $(WARN_LDFLAGS)

if OS_WIN32
av-cp.res: av-cp.rc
	$(WINDRES) -i av-cp.rc --input-format=rc -o av-cp.res -O coff
gupnp_av_cp_LDADD += av-cp.res
BUILT_SOURCES += av-cp.res
CLEANFILES += av-cp.res
gupnp_av_cp_LDFLAGS += -Wl,-subsystem,windows
endif

EXTRA_DIST=av-cp.rc

MAINTAINERCLEANFILES = Makefile.in
