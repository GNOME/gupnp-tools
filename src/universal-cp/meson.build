universal_cp_resource = gnome.compile_resources('universal-cp-resource',
    join_paths(resource_data, 'org.gupnp.Tools.Universal-CP.gresource.xml'),
    source_dir : resource_data)

executable('gupnp-universal-cp',
           [
               'action-dialog.c',
               'action-dialog.h',
               'details-treeview.c',
               'details-treeview.h',
               'device-treeview.c',
               'device-treeview.h',
               'event-treeview.c',
               'event-treeview.h',
               'gui.c',
               'gui.h',
               'main.c',
               'main.h'
           ] + universal_cp_resource,
           export_dynamic: true,
           install : true,
           dependencies : [util, glib, gupnp, gtk, gssdp, config_header])

