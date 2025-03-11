project('gupnp-tools', ['c'], version: '0.12.2')

gnome = import('gnome')
i18n = import('i18n')

if host_machine.system() == 'windows'
    win = import('windows')
endif

gssdp = dependency('gssdp-1.6', version : '>= 1.6.2', default_options: ['sniffer=false', 'manpages=false', 'gtk_doc=false'], fallback: 'gssdp-1.6')
gupnp = dependency('gupnp-1.6', version: '>= 1.6.0')
soup = dependency('libsoup-3.0', version: '>= 3.0')
gupnp_av = dependency('gupnp-av-1.0', version: '>= 0.5.5', required: get_option('av-tools'))
gtk = dependency('gtk+-3.0', version: '>= 3.10')
glib_version = '2.68'
glib = dependency('glib-2.0', version: '>= ' + glib_version)
gio = dependency('gio-2.0', version: '>= ' + glib_version)
libxml = dependency('libxml-2.0', version: '>= 2.0')
gobject = dependency('gobject-2.0', version: '>= ' + glib_version)

# Check for gtksourceview (used for XML syntax highlighting)
gtksourceview = dependency('gtksourceview-4', required: false)
if not gtksourceview.found()
    gtksourceview = dependency('gtksourceview-3.0', version: '>= 3.2.0', required: false)
endif

prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))
localedir = join_paths(prefix, get_option('localedir'))

resource_data = join_paths(meson.source_root(), 'data')
if get_option('uninstalled')
    shareddir = resource_data
else
    shareddir = join_paths(datadir, meson.project_name())
endif

desktopdir = join_paths(datadir, 'applications')

conf = configuration_data()
conf.set('VERSION', '"@0@"'.format(meson.project_version()))
conf.set('DATA_DIR', '"@0@"'.format(shareddir))
conf.set('HAVE_GTK_SOURCEVIEW', gtksourceview.found())
conf.set('GETTEXT_PACKAGE', '"@0@"'.format(meson.project_name()))
conf.set('LOCALEDIR', '"@0@"'.format(localedir))
cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments('-Werror=deprecated-declarations'), language: 'c')
conf.set('GLIB_VERSION_MIN_REQUIRED', 'GLIB_VERSION_@0@'.format(glib_version.underscorify()))
conf.set('GLIB_VERSION_MAX_ALLOWED', 'GLIB_VERSION_@0@'.format(glib_version.underscorify()))


configure_file(input: 'config.h.meson', output: 'config.h', configuration: conf)
config_header = declare_dependency(include_directories : include_directories('.'))

subdir('data')
subdir('src')
subdir('po')
