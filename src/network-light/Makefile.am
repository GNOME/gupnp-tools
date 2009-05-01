if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(LIBGUPNP_CFLAGS) \
	    $(GTK_CFLAGS) \
	    $(GTHREAD_CFLAGS) \
	    $(UUID_CFLAGS) \
	    -I$(top_srcdir) \
	    -DDATA_DIR='"$(shareddir)"' \
	    -I$(top_srcdir)/src/common

bin_PROGRAMS = gupnp-network-light

gupnp_network_light_SOURCES = main.c \
			      gui.c \
			      upnp.c \
			      main.h \
			      gui.h \
			      upnp.h
gupnp_network_light_LDADD = ../common/libicons.a $(LIBGUPNP_LIBS) $(GTK_LIBS) $(GTHREAD_LIBS) $(UUID_LIBS)

gupnp_network_light_LDFLAGS = -export-dynamic

MAINTAINERCLEANFILES = Makefile.in
