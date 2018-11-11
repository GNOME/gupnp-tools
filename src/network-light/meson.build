if host_machine.system() == 'windows'
    network_light_win_resources = win.compile_resources('av-cp.rc')
else
    network_light_win_resources = []
endif

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
           ] + network_light_resource + network_light_win_resources,
           export_dynamic: true,
           install : true,
           dependencies : [gupnp, gtk, util, gssdp, config_header],
           gui_app : true
       )
