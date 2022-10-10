util_resource = gnome.compile_resources('common-resource',
    join_paths(resource_data, 'org.gupnp.Tools.Common.gresource.xml'),
    c_name : 'gupnp_tools_common',
    export : true,
    source_dir : resource_data,
    extra_args : ['--manual-register']
)

libutil = static_library('util', ['icons.c', 'icons.h', 'pretty-print.c', 'pretty-print.h'] + util_resource,
    dependencies : [libxml, soup, gupnp, gtk, config_header])

util = declare_dependency(link_with: libutil, include_directories : include_directories('.'))
