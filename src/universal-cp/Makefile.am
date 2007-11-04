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
			     devicetreeview.c \
			     detailstreeview.c \
			     eventtreeview.c \
			     actiondialog.c \
			     main.h \
			     gui.h \
			     devicetreeview.h \
			     detailstreeview.h \
			     eventtreeview.h \
			     actiondialog.h
gupnp_universal_cp_LDADD = $(LIBGLADE_LIBS) $(LIBGUPNP_LIBS) \
		    	   ../common/libicons.a
gupnp_universal_cp_LDFLAGS = -export-dynamic

MAINTAINERCLEANFILES = Makefile.in
