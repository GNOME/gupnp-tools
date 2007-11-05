if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(LIBGUPNP_CFLAGS) $(LIBGUPNP_AV_CFLAGS) $(LIBGLADE_CFLAGS) \
	    -I$(top_srcdir) -DDATA_DIR='"$(shareddir)"' -I../common

bin_PROGRAMS = gupnp-av-cp

gupnp_av_cp_SOURCES = main.c \
		      gui.c \
		      playlist-treeview.c \
		      renderer-combo.c \
		      main.h \
		      gui.h \
		      playlist-treeview.h \
		      renderer-combo.h

gupnp_av_cp_LDADD = $(LIBGLADE_LIBS) \
		    $(LIBGUPNP_LIBS) \
		    $(LIBGUPNP_AV_LIBS) \
		    ../common/libicons.a
gupnp_av_cp_LDFLAGS = -export-dynamic

MAINTAINERCLEANFILES = Makefile.in
