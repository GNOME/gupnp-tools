if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(LIBGUPNP_CFLAGS) $(LIBGLADE_CFLAGS) -I$(top_srcdir) \
	    -DDATA_DIR='"$(shareddir)"' -I../common

bin_PROGRAMS = gupnp-av-cp

gupnp_av_cp_SOURCES = main.c \
		      gui.c \
		      playlisttreeview.c \
		      renderercombo.c \
		      main.h \
		      gui.h \
		      playlisttreeview.h \
		      renderercombo.h

gupnp_av_cp_LDADD = $(LIBGLADE_LIBS) $(LIBGUPNP_LIBS) \
		    ../common/libicons.a
gupnp_av_cp_LDFLAGS = -export-dynamic

MAINTAINERCLEANFILES = Makefile.in
