AM_CFLAGS = $(LIBGSSDP_CFLAGS)    \
	    $(GIO_CFLAGS)         \
	    -I$(top_srcdir)

bin_PROGRAMS = gssdp-discover

gssdp_discover_SOURCES = gssdp-discover.c

gssdp_discover_LDADD = $(LIBGSSDP_LIBS) $(GIO_LIBS)

MAINTAINERCLEANFILES = Makefile.in
