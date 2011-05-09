if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(LIBGUPNP_CFLAGS) $(GTK_CFLAGS) -I$(top_srcdir) \
	    -DDATA_DIR='"$(shareddir)"' -I$(top_builddir)/src/common

noinst_LIBRARIES = libutil.a

libutil_a_SOURCES = icons.c \
		    icons.h \
		    pretty-print.c \
		    pretty-print.h

MAINTAINERCLEANFILES = Makefile.in
