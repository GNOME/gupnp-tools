SUBDIRS = pixmaps

uidir = $(datadir)/gupnp-tools
dist_ui_DATA = $(srcdir)/gupnp-universal-cp.glade

AM_CFLAGS = $(LIBGUPNP_CFLAGS) $(LIBGLADE_CFLAGS) -I$(top_srcdir) \
	    -DUI_DIR='"$(uidir)"'

bin_PROGRAMS = gupnp-universal-cp

gupnp_universal_cp_SOURCES = universal-cp.c \
			     universal-cp-gui.c \
			     universal-cp-devicetreeview.c \
			     universal-cp-actiondialog.c \
			     universal-cp.h \
			     universal-cp-gui.h \
			     universal-cp-devicetreeview.h \
			     universal-cp-actiondialog.h
gupnp_universal_cp_LDADD = $(LIBGLADE_LIBS) $(LIBGUPNP_LIBS)
gupnp_universal_cp_LDFLAGS = -export-dynamic

MAINTAINERCLEANFILES = Makefile.in
