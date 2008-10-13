AM_CFLAGS = $(LIBGUPNP_CFLAGS)    \
	    $(LIBGUPNP_AV_CFLAGS) \
	    $(GTHREAD_CFLAGS)     \
	    $(GIO_CFLAGS)         \
	    -I$(top_srcdir)       \
	    -I$(top_srcdir)/src/common

bin_PROGRAMS = gupnp-upload

gupnp_upload_SOURCES = main.c \
		       main.h \
		       control_point.c \
		       control_point.h \
		       item-creation.c \
		       item-creation.h \
		       container-search.c \
		       container-search.h \
		       transfer.c \
		       transfer.h

gupnp_upload_LDADD = $(LIBGUPNP_LIBS)    \
		     $(LIBGUPNP_AV_LIBS) \
	    	     $(GTHREAD_LIBS)     \
	    	     $(GIO_LIBS)

gupnp_upload_LDFLAGS = -export-dynamic

MAINTAINERCLEANFILES = Makefile.in
