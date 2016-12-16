if UNINSTALLED
shareddir = $(abs_top_builddir)/data
else
shareddir = $(datadir)/gupnp-tools
endif

AM_CFLAGS = $(LIBGUPNP_CFLAGS) $(GTK_CFLAGS) -I$(top_srcdir) \
	    -DDATA_DIR='"$(shareddir)"' -I$(top_builddir)/src/common \
	    $(WARN_CFLAGS)

noinst_LIBRARIES = libutil.a

libutil_a_SOURCES = \
		    resource.c \
		    icons.c \
		    icons.h \
		    pretty-print.c \
		    pretty-print.h

MAINTAINERCLEANFILES = Makefile.in

RESOURCEFILE = $(top_srcdir)/data/org.gupnp.Tools.Common.gresource.xml

resource_depends = \
		   $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies \
		   --sourcedir=$(abs_top_srcdir)/data \
		   $(RESOURCEFILE))



resource.c: $(RESOURCEFILE) $(resource_depends)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES)\
	    --target=$@ \
	    --sourcedir=$(abs_top_srcdir)/data \
	    --generate $(RESOURCEFILE) \
	    --manual-register \
	    --c-name gupnp_tools_common
