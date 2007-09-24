SUBDIRS = pixmaps xml

shareddir = $(datadir)/gupnp-tools
dist_shared_DATA = $(srcdir)/gupnp-universal-cp.glade \
		   $(srcdir)/gupnp-network-light.glade \
		   $(srcdir)/gupnp-av-cp.glade

AM_CFLAGS = $(LIBGUPNP_CFLAGS) $(LIBGLADE_CFLAGS) -I$(top_srcdir) \
	    -DDATA_DIR='"$(shareddir)"'

bin_PROGRAMS = gupnp-universal-cp gupnp-network-light gupnp-av-cp

gupnp_universal_cp_SOURCES = universal-cp.c \
			     universal-cp-gui.c \
			     universal-cp-devicetreeview.c \
			     universal-cp-detailstreeview.c \
			     universal-cp-eventtreeview.c \
			     universal-cp-actiondialog.c \
			     universal-cp-icons.c \
			     universal-cp.h \
			     universal-cp-gui.h \
			     universal-cp-devicetreeview.h \
			     universal-cp-detailstreeview.h \
			     universal-cp-eventtreeview.h \
			     universal-cp-actiondialog.h \
			     universal-cp-icons.h
gupnp_universal_cp_LDADD = $(LIBGLADE_LIBS) $(LIBGUPNP_LIBS)
gupnp_universal_cp_LDFLAGS = -export-dynamic

gupnp_network_light_SOURCES = network-light.c \
			      network-light-gui.c \
			      network-light-upnp.c \
			      network-light.h \
			      network-light-gui.h \
			      network-light-upnp.h
gupnp_network_light_LDADD = $(LIBGLADE_LIBS) $(LIBGUPNP_LIBS)
gupnp_network_light_LDFLAGS = -export-dynamic

gupnp_av_cp_SOURCES = av-cp.c \
		      av-cp-gui.c \
		      av-cp-playlisttreeview.c \
		      av-cp.h \
		      av-cp-gui.h \
		      av-cp-playlisttreeview.h
gupnp_av_cp_LDADD = $(LIBGLADE_LIBS) $(LIBGUPNP_LIBS)
gupnp_av_cp_LDFLAGS = -export-dynamic

MAINTAINERCLEANFILES = Makefile.in
