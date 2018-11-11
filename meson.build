project('gupnp-tools', ['c'], version: '0.8.15')

gnome = import('gnome')
i18n = import('i18n')

if host_machine.system() == 'windows'
    win = import('windows')
endif

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
gtksourceview = dependency('gtksourceview-4', required: false)
if not gtksourceview.found()
    gtksourceview = dependency('gtksourceview-3.0', version: '>= 3.2.0', required: false)
endif

resource_data = join_paths(meson.source_root(), 'data')
if get_option('uninstalled')
    shareddir = resource_data
else
    shareddir = join_paths(get_option('datadir'), meson.project_name())
endif

desktopdir = join_paths(get_option('datadir'), 'applications')

conf = configuration_data()
conf.set('VERSION', '"@0@"'.format(meson.project_version()))
conf.set('DATA_DIR', '"@0@"'.format(shareddir))
conf.set('HAVE_GTK_SOURCEVIEW', gtksourceview.found())
conf.set('GETTEXT_PACKAGE', '"@0@"'.format(meson.project_name()))
conf.set('LOCALEDIR', '"@0@"'.format(join_paths(get_option('prefix'), get_option('localedir'))))

configure_file(input: 'config.h.meson', output: 'config.h', configuration: conf)
config_header = declare_dependency(compile_args : '-DHAVE_CONFIG_H=1', include_directories : include_directories('.'))

subdir('data')
subdir('src')
subdir('po')
