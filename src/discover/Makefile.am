AM_CFLAGS = $(LIBGSSDP_CFLAGS)    \
	    $(GIO_CFLAGS)         \
	    -I$(top_srcdir) \
	    $(WARN_CFLAGS)

bin_PROGRAMS = gssdp-discover

gssdp_discover_SOURCES = gssdp-discover.c

gssdp_discover_LDADD = $(LIBGSSDP_LIBS) $(GIO_LIBS)

gssdp_discover_LDFLAGS = $(WARN_LDFLAGS)

MAINTAINERCLEANFILES = Makefile.in
