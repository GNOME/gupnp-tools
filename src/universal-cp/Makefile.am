if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(GTK_CFLAGS) $(LIBGUPNP_CFLAGS) $(GTHREAD_CFLAGS) \
	    -I$(top_srcdir) -DDATA_DIR='"$(shareddir)"' \
	    -I$(top_srcdir)/src/common

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
gupnp_universal_cp_LDADD = ../common/libutil.a $(GTK_LIBS) $(LIBGUPNP_LIBS) $(GTHREAD_LIBS)
gupnp_universal_cp_LDFLAGS = -export-dynamic

if OS_WIN32
universal-cp.res: universal-cp.rc
	$(WINDRES) -i universal-cp.rc --input-format=rc -o universal-cp.res -O coff
gupnp_universal_cp_LDADD += universal-cp.res
BUILT_SOURCES=universal-cp.res
CLEAN_FILES=universal-cp.res
gupnp_universal_cp_LDFLAGS += -Wl,-subsystem,windows
endif

EXTRA_DIST=universal-cp.rc


MAINTAINERCLEANFILES = Makefile.in
