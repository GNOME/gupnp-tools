if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(LIBGUPNP_CFLAGS) $(LIBGLADE_CFLAGS) -I$(top_srcdir) \
	    -DDATA_DIR='"$(shareddir)"' -I../common

bin_PROGRAMS = gupnp-universal-cp

gupnp_universal_cp_SOURCES = main.c \
			     gui.c \
			     device-treeview.c \
			     details-treeview.c \
			     event-treeview.c \
			     action-dialog.c \
			     main.h \
			     gui.h \
			     device-treeview.h \
			     details-treeview.h \
			     event-treeview.h \
			     action-dialog.h
gupnp_universal_cp_LDADD = $(LIBGLADE_LIBS) $(LIBGUPNP_LIBS) \
		    	   ../common/libicons.a
gupnp_universal_cp_LDFLAGS = -export-dynamic

MAINTAINERCLEANFILES = Makefile.in
