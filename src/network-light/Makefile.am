if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(LIBGUPNP_CFLAGS) $(LIBGLADE_CFLAGS) -I$(top_srcdir) \
	    -DDATA_DIR='"$(shareddir)"'

bin_PROGRAMS = gupnp-network-light

gupnp_network_light_SOURCES = network-light.c \
			      network-light-gui.c \
			      network-light-upnp.c \
			      network-light.h \
			      network-light-gui.h \
			      network-light-upnp.h
gupnp_network_light_LDADD = $(LIBGLADE_LIBS) $(LIBGUPNP_LIBS)
gupnp_network_light_LDFLAGS = -export-dynamic

MAINTAINERCLEANFILES = Makefile.in
