network_light_resource = gnome.compile_resources('network-light-resource',
    join_paths(resource_data, 'org.gupnp.Tools.Network-Light.gresource.xml'),
    source_dir : resource_data)

executable('gupnp-network-light',
           [
               'gui.c',
               'gui.h',
               'main.c',
               'main.h',
               'upnp.c',
               'upnp.h'
           ] + network_light_resource,
           export_dynamic: true,
           install : true,
           dependencies : [gupnp, gtk, util, gssdp, config_header])
