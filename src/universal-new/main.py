#!/usr/bin/env python3

import gi

gi.require_version('GUPnP', '1.6')
gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')

from gi.repository import GLib, Gio
from gi.repository import GObject
from gi.repository import GUPnP
from gi.repository import Gtk
from gi.repository import Adw
from gi.repository import Pango


class Device(GObject.GObject):
    def __init__(self, proxy):
        self.proxy = proxy
        super(Device, self).__init__()


class DeviceModel(GObject.GObject, Gio.ListModel):
    def __init__(self, context):
        self.context = context
        self.devices = []
        self.control_point = GUPnP.ControlPoint.new (self.context, "upnp:rootdevice")
        self.control_point.connect ("device-proxy-available", self.on_device_available)
        self.control_point.connect ("device-proxy-unavailable", self.on_device_unavailable)
        self.control_point.set_active(True)
        super(DeviceModel, self).__init__()

    def do_get_item_type (self):
        return Device

    def do_get_n_items(self):
        return len(self.devices)

    def do_get_item(self, position):
        return self.devices[position]

    def on_device_available(self, cp, proxy):
        self.devices.append(Device(proxy))
        self.items_changed(len(self.devices) - 1, 0, 1)

    def on_device_unavailable(self, cp, proxy):
        to_remove = None

        for i, j in enumerate(self.devices):
            if j.proxy == proxy:
                to_remove = i, j
                break

        if to_remove is None:
            return

        self.devices.remove(to_remove[1])
        self.items_changed(to_remove[0], 1, 0)

class DetailRow(Adw.ActionRow):
    def __init__(self, name, detail):
        super().__init__(title=name)
        l = Gtk.Label.new(detail)
        l.add_css_class("dim-label")
        l.set_ellipsize(Pango.EllipsizeMode.END)
        l.set_tooltip_text(detail)
        l.set_hexpand(False)
        l.set_natural_wrap_mode(Gtk.NaturalWrapMode.WORD)
        l.set_wrap_mode(Pango.WrapMode.WORD_CHAR)
        self.add_suffix(l)
        self.set_title_lines(1)

class DeviceInfo(Gtk.Box):
    def __init__(self, device):
        super().__init__(orientation=Gtk.Orientation.VERTICAL)
        scrolled = Gtk.ScrolledWindow.new()
        scrolled.set_hexpand(True)
        scrolled.set_vexpand(True)
        self.append(scrolled)
        self.page = Adw.PreferencesPage()
        self.page.add_css_class("boxed-list")
        scrolled.set_child(self.page)

        self.icon = Adw.PreferencesGroup()
        result = device.proxy.get_icon_url(None, -1, -1, -1, True)
        p = Gtk.Picture.new_for_file(Gio.File.new_for_uri(result[0]))
        p.set_keep_aspect_ratio(True)
        p.set_can_shrink(False)
        self.icon.add(p)
        self.page.add(self.icon)
        self.group = Adw.PreferencesGroup()
        self.page.add(self.group)
        self.group.add(DetailRow("Friendly Name", device.proxy.get_friendly_name()))
        self.group.add(DetailRow("Presentation URL", device.proxy.get_presentation_url()))

        self.group = Adw.PreferencesGroup()
        self.page.add(self.group)
        self.group.add(DetailRow("Location", device.proxy.get_location ()))
        self.group.add(DetailRow("UDN", device.proxy.get_udn()))
        self.group.add(DetailRow("Type", device.proxy.get_device_type()))
        self.group.add(DetailRow("Base URL", device.proxy.get_url_base().to_string()))

        self.group = Adw.PreferencesGroup()
        self.page.add(self.group)
        self.group.add(DetailRow("Manufacturer", device.proxy.get_manufacturer()))
        self.group.add(DetailRow("Manufacturer URL", device.proxy.get_manufacturer_url()))

        self.group = Adw.PreferencesGroup()
        self.page.add(self.group)
        self.group.add(DetailRow("Model Name", device.proxy.get_model_name()))
        self.group.add(DetailRow("Model Description", device.proxy.get_model_description()))
        self.group.add(DetailRow("Model Number", device.proxy.get_model_number()))
        self.group.add(DetailRow("Model URL", device.proxy.get_model_url()))
        self.group.add(DetailRow("Serial Number", device.proxy.get_serial_number()))


class Section(GObject.Object):
    def __init__(self, name):
        self.name = name
        super(Section, self).__init__()

class DeviceContents(Gtk.Box):
    def __init__(self, device):
        super().__init__(orientation=Gtk.Orientation.VERTICAL)

        self.device = device
        scrolled = Gtk.ScrolledWindow.new()
        list_view = Gtk.ListView()
        list_view.add_css_class("navigation-sidebar")
        scrolled.set_child(list_view)
        model = Gtk.TreeListModel.new(self.get_device_model(device), False, True, self.get_device_model)

        selection = Gtk.SingleSelection.new(model)
        list_view.set_model(selection)
        scrolled.set_vexpand(True)
        self.append(scrolled)
        factory = Gtk.SignalListItemFactory()
        factory.connect ("setup", self.on_setup)
        factory.connect ("bind", self.on_bind)

        list_view.set_factory(factory)

    def on_setup(self, foo, item):
        expander = Gtk.TreeExpander()
        l = Gtk.Label()
        expander.set_child (l)
        item.set_child(expander)

    def on_bind(self, foo, item):
        content = item.get_item().get_item()
        print(content)
        expander = item.get_child()
        expander.set_list_row(item.get_item())
        if isinstance(content, Section):
            expander.get_child().set_label(content.name)
        elif isinstance(content, GUPnP.ServiceInfo):
            expander.get_child().set_label(content.get_id())
        elif isinstance(content, GUPnP.DeviceInfo):
            expander.get_child().set_label(content.get_friendly_name())
        elif isinstance(content, GUPnP.ServiceActionInfo):
            expander.get_child().set_label(content.name)


    def get_device_model(self, item):
        if isinstance(item, GUPnP.DeviceInfo):
            model = Gio.ListStore.new(Section)
            devices = item.list_device_types()
            if len(devices) > 0:
                model.append(Section("Devices"))

            services = item.list_service_types()
            if len(services) > 0:
                model.append(Section("Services"))

            return model

        if isinstance(item, Section):
            if item.name == "Services":
                services = self.device.list_services()

                model = Gio.ListStore.new(GUPnP.ServiceInfo)
                for service in services:
                    model.append(service)

                return model

        if isinstance(item, GUPnP.ServiceInfo):
            #model = Gio.ListStore.new(GUPnP.ServiceActionInfo)
            item.introspect_async (None, lambda source, res: self.on_introspection(source, res, None))
            return None

        return None

    def on_introspection(self, source, res, model):
        introspection = source.introspect_finish(res)

        [print(x.name) for x in introspection.list_actions()]


class AppWindow(Adw.ApplicationWindow):
    def __init__(self, app, context):
        super().__init__(application=app, title="GUPnP Universal Control Point")
        self.device_model = DeviceModel(context)
        self.set_default_size(600, 300)
        box = Gtk.Box.new(Gtk.Orientation.VERTICAL, 0)
        header = Gtk.HeaderBar.new()
        box.prepend(header)
        paned = Gtk.Paned.new(Gtk.Orientation.HORIZONTAL)
        paned.set_vexpand(True)
        box.append(paned)
        self.set_content(box)

        self.navigation_stack = Gtk.Stack.new()
        scrollable = Gtk.ScrolledWindow.new()
        lb = Gtk.ListBox()
        lb.bind_model(self.device_model, self.on_widget)
        lb.connect('row-selected', self.on_row_activated)
        scrollable.set_child(lb)
        self.navigation_stack.add_child(scrollable)
        paned.set_start_child(self.navigation_stack)

        stack = Gtk.Stack.new()
        status = Adw.StatusPage.new()
        status.set_icon_name("network-wired-symbolic")
        status.set_title("Select an UPnP device")
        status.set_description("Select device to introspect")
        stack.add_child(status)
        paned.set_end_child (stack)
        stack.set_visible_child(status)
        self.stack = stack

    def on_widget(self, item):
        row = Adw.ActionRow.new()
        row.set_title(item.proxy.get_friendly_name())
        row.set_subtitle(item.proxy.get_url_base().get_host())
        row.add_suffix(Gtk.Image.new_from_icon_name('go-next-symbolic'))

        return row

    def on_row_activated(self, list_box, row):
        device_proxy = self.device_model.get_item(row.get_index())
        details = DeviceInfo(device_proxy)
        self.stack.add_child(details)
        self.stack.set_visible_child(details)
        c = DeviceContents(device_proxy.proxy)
        self.navigation_stack.add_child(c)
        self.navigation_stack.set_visible_child(c)

def on_activate(app):
    context = GUPnP.Context (interface='wlp3s0', address_family=Gio.SocketFamily.IPV6)
    context.init()

    w = AppWindow(app, context)
    w.present()


if __name__ == "__main__":
    app = Adw.Application.new("org.gupnp.UniversalCp", Gio.ApplicationFlags.FLAGS_NONE)
    app.connect("activate", on_activate)
    app.run()
