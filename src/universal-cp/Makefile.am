if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(LIBGUPNP_CFLAGS) $(LIBGLADE_CFLAGS) -I$(top_srcdir) \
	    -DDATA_DIR='"$(shareddir)"' -I../common

bin_PROGRAMS = gupnp-universal-cp

gupnp_universal_cp_SOURCES = universal-cp.c \
			     universal-cp-gui.c \
			     universal-cp-devicetreeview.c \
			     universal-cp-detailstreeview.c \
			     universal-cp-eventtreeview.c \
			     universal-cp-actiondialog.c \
			     universal-cp.h \
			     universal-cp-gui.h \
			     universal-cp-devicetreeview.h \
			     universal-cp-detailstreeview.h \
			     universal-cp-eventtreeview.h \
			     universal-cp-actiondialog.h
gupnp_universal_cp_LDADD = $(LIBGLADE_LIBS) $(LIBGUPNP_LIBS) \
		    	   ../common/libicons.a
gupnp_universal_cp_LDFLAGS = -export-dynamic

MAINTAINERCLEANFILES = Makefile.in
