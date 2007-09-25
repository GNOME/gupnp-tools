SUBDIRS = common av-cp network-light universal-cp pixmaps xml

if UNINSTALLED
shareddir = $(abs_top_builddir)/src
else
shareddir = $(datadir)/gupnp-tools
endif

dist_shared_DATA = $(srcdir)/gupnp-universal-cp.glade \
		   $(srcdir)/gupnp-network-light.glade \
		   $(srcdir)/gupnp-av-cp.glade

MAINTAINERCLEANFILES = Makefile.in
