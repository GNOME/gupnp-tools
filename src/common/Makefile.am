if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(LIBGUPNP_CFLAGS) $(LIBGLADE_CFLAGS) -I$(top_srcdir) \
	    -DDATA_DIR='"$(shareddir)"' -I$(top_builddir)/src/common

noinst_LIBRARIES = libicons.a

libicons_a_SOURCES = icons.c \
		     icons.h

MAINTAINERCLEANFILES = Makefile.in
