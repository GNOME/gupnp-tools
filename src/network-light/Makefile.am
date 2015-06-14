if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(LIBGUPNP_CFLAGS) \
	    $(GTK_CFLAGS) \
	    $(UUID_CFLAGS) \
	    -I$(top_srcdir) \
	    -DDATA_DIR='"$(shareddir)"' \
	    -I$(top_srcdir)/src/common \
	    $(WARN_CFLAGS)

bin_PROGRAMS = gupnp-network-light

gupnp_network_light_SOURCES = main.c \
			      gui.c \
			      upnp.c \
			      main.h \
			      gui.h \
			      upnp.h
gupnp_network_light_LDADD = ../common/libutil.a \
			    $(LIBXML_LIBS) \
			    $(LIBSOUP_LIBS) \
			    $(LIBGSSDP_LIBS) \
			    $(LIBGUPNP_LIBS) \
			    $(GTK_LIBS) \
			    $(UUID_LIBS)

gupnp_network_light_LDFLAGS = -export-dynamic $(WARN_LDFLAGS)

if OS_WIN32
network-light.res: network-light.rc
	$(WINDRES) -i network-light.rc --input-format=rc -o network-light.res -O coff
gupnp_network_light_LDADD += network-light.res
BUILT_SOURCES=network-light.res
CLEAN_FILES=network-light.res
gupnp_network_light_LDFLAGS += -Wl,-subsystem,windows
endif

EXTRA_DIST=network-light.rc

MAINTAINERCLEANFILES = Makefile.in
