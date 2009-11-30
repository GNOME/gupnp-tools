AM_CFLAGS = $(LIBGUPNP_CFLAGS) \
	    $(LIBGUPNP_AV_CFLAGS) \
	    $(GTK_CFLAGS) \
	    $(GTHREAD_CFLAGS)

bin_PROGRAMS = gupnp-av-browser

gupnp_av_browser_SOURCES = main.c content-directory-service.h
BUILT_SOURCES = content-directory-service.h
CLEANFILES = $(BUILT_SOURCES) stamp-content-directory-service.h

gupnp_av_browser_LDADD = $(LIBGUPNP_LIBS) \
			 $(LIBGUPNP_AV_LIBS) \
			 $(GTK_LIBS) \
			 $(GTHREAD_LIBS)

MAINTAINERCLEANFILES = Makefile.in

%-service.h: stamp-%-service.h
	@true
stamp-content-directory-service.h: $(srcdir)/ContentDirectory.xml $(GUPNP_BINDING_TOOL)
	$(GUPNP_BINDING_TOOL) --prefix=cd --mode=client $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)
