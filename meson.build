project('gupnp-tools', ['c'], version: '0.8.15')

gnome = import('gnome')

gssdp = dependency('gssdp-1.0', version: '>= 0.13.3')
gupnp = dependency('gupnp-1.0', version: '>= 0.20.14')
soup = dependency('libsoup-2.4', version: '>= 2.42')
gupnp_av = dependency('gupnp-av-1.0', version: '>= 0.5.5', required: false)
gtk = dependency('gtk+-3.0', version: '>= 3.10')
glib = dependency('glib-2.0', version: '>= 2.24')
gio = dependency('gio-2.0', version: '>= 2.24')
libxml = dependency('libxml-2.0', version: '>= 2.0')
gobject = dependency('gobject-2.0', version: '>= 2.24')

# Check for gtksourceview (used for XML syntax highlighting)
gtksourceview = dependency('gtksourceview-4.0', required: false)
if not gtksourceview.found()
    gtksourceview = dependency('gtksourceview-3.0', version: '>= 3.2.0', required: false)
endif

conf = configuration_data()
conf.set('VERSION', '@0@'.format(meson.project_version()))

#configure_file('config.h', configuration: conf)
#add_global_arguments('-DHAVE_CONFIG_H=1', language : 'c')

add_global_arguments(['-DVERSION=0', '-DGETTEXT_PACKAGE=""', '-DDATA_DIR=""', '-DLOCALEDIR=""'], language : 'c')

resource_data = join_paths(meson.source_root(), 'data')

subdir('src')
