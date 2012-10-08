if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(LIBGUPNP_CFLAGS) \
	    $(LIBGUPNP_AV_CFLAGS) \
	    $(GTK_CFLAGS) \
	    $(GTHREAD_CFLAGS) \
	    $(GTKSOURCEVIEW_CFLAGS) \
	    -I$(top_srcdir) -DDATA_DIR='"$(shareddir)"' \
	    -I$(top_srcdir)/src/common

bin_PROGRAMS = gupnp-av-cp

gupnp_av_cp_SOURCES = main.c \
		      gui.c \
		      playlist-treeview.c \
		      renderer-combo.c \
		      renderer-controls.c \
		      main.h \
		      gui.h \
		      playlist-treeview.h \
		      renderer-combo.h \
		      renderer-controls.h

gupnp_av_cp_LDADD = ../common/libutil.a \
		    $(LIBGSSDP_LIBS) \
		    $(LIBSOUP_LIBS) \
		    $(LIBXML_LIBS) \
		    $(LIBGUPNP_LIBS) \
		    $(LIBGUPNP_AV_LIBS) \
		    $(GTK_LIBS) \
		    $(GTHREAD_LIBS) \
		    $(GTKSOURCEVIEW_LIBS)
gupnp_av_cp_LDFLAGS = -export-dynamic

if OS_WIN32
av-cp.res: av-cp.rc
	$(WINDRES) -i av-cp.rc --input-format=rc -o av-cp.res -O coff
gupnp_av_cp_LDADD += av-cp.res
BUILT_SOURCES=av-cp.res
CLEAN_FILES=av-cp.res
gupnp_av_cp_LDFLAGS += -Wl,-subsystem,windows
endif

EXTRA_DIST=av-cp.rc

MAINTAINERCLEANFILES = Makefile.in
