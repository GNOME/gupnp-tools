SUBDIRS = pixmaps

uidir = $(datadir)/gupnp-tools
dist_ui_DATA = $(srcdir)/gupnp-universal-cp.glade \
	       $(srcdir)/gupnp-network-light.glade

AM_CFLAGS = $(LIBGUPNP_CFLAGS) $(LIBGLADE_CFLAGS) -I$(top_srcdir) \
	    -DUI_DIR='"$(uidir)"'

bin_PROGRAMS = gupnp-universal-cp gupnp-network-light

gupnp_universal_cp_SOURCES = universal-cp.c \
			     universal-cp-gui.c \
			     universal-cp-devicetreeview.c \
			     universal-cp-actiondialog.c \
			     universal-cp-icons.c \
			     universal-cp.h \
			     universal-cp-gui.h \
			     universal-cp-devicetreeview.h \
			     universal-cp-actiondialog.h \
			     universal-cp-icons.h
gupnp_universal_cp_LDADD = $(LIBGLADE_LIBS) $(LIBGUPNP_LIBS)
gupnp_universal_cp_LDFLAGS = -export-dynamic

gupnp_network_light_SOURCES = network-light.c \
			      network-light-gui.c \
			      network-light-gui.h
gupnp_network_light_LDADD = $(LIBGLADE_LIBS) $(LIBGUPNP_LIBS)
gupnp_network_light_LDFLAGS = -export-dynamic

MAINTAINERCLEANFILES = Makefile.in
