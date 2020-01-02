didl_dialog_data = configuration_data()
if gtksourceview.found()
    didl_dialog_data.set('TEXT_VIEW', 'GtkSourceView')
else
    didl_dialog_data.set('TEXT_VIEW', 'GtkTextView')
endif
didl_dialog_ui = configure_file(input: 'didl-lite-dialog.ui.in', output: 'didl-lite-dialog.ui', configuration : didl_dialog_data)

install_data(['xml/Dimming-scpd.xml', 'xml/network-light-desc.xml', 'xml/SwitchPower-scpd.xml'], install_dir : join_paths(shareddir, 'xml'))
install_data(
    [
        'pixmaps/media-renderer.png',
        'pixmaps/network-light-22x22.png',
        'pixmaps/network-light-off.png',
        'pixmaps/network-light-on.png',
        'pixmaps/upnp-action-arg-in.png',
        'pixmaps/upnp-action-arg-out.png',
        'pixmaps/upnp-device.png',
        'pixmaps/upnp-service.png',
        'pixmaps/upnp-state-variable.png'
    ],
    install_dir : join_paths(shareddir, 'pixmaps')
)

install_subdir('pixmaps/hicolor', install_dir : join_paths(get_option('datadir'), 'icons'))

desktop_conf = configuration_data()
desktop_conf.set('PKGDATADIR', shareddir)

podir = join_paths('..', 'po')

i18n.merge_file(input: 'gupnp-av-cp.desktop.in', output: 'gupnp-av-cp.desktop',
                po_dir : podir,
                type: 'desktop',
                install: true,
                install_dir: desktopdir)

i18n.merge_file(input: 'gupnp-universal-cp.desktop.in', output: 'gupnp-universal-cp.desktop',
                po_dir : podir,
                type: 'desktop',
                install: true,
                install_dir: desktopdir)

i18n.merge_file(input: 'gupnp-network-light.desktop.in', output: 'gupnp-network-light.desktop',
                po_dir : podir,
                type: 'desktop',
                install: true,
                install_dir: desktopdir)
