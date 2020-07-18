if host_machine.system() == 'windows'
    av_cp_win_resources = win.compile_resources('av-cp.rc')
else
    av_cp_win_resources = []
endif

av_cp_resources = gnome.compile_resources('av-cp-resource',
    join_paths(resource_data, 'org.gupnp.Tools.AV-CP.gresource.xml'),
    source_dir : resource_data,
    dependencies: didl_dialog_ui)

executable('gupnp-av-cp',
           ['didl-dialog.c',
            'didl-dialog.h',
            'gui.c',
            'gui.h',
            'main.c',
            'main.h',
            'playlist-treeview.c',
            'playlist-treeview.h',
            'renderer-combo.c',
            'renderer-combo.h',
            'renderer-controls.c',
            'renderer-controls.h',
            'search-dialog.c',
            'search-dialog.h',
            'server-device.c',
            'server-device.h',
            'entry-completion.c',
            'entry-completion.h'
            ] + av_cp_resources + av_cp_win_resources,
           dependencies : [gupnp, gssdp, gupnp_av, gtk, util, gtksourceview, config_header],
           export_dynamic : true,
           install : true,
           gui_app : true
       )
