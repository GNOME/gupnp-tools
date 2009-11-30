/*
 * Bickley - a meta data management framework, the file system spider.
 *
 * Copyright © 2008, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string.h>
#include <glib.h>

#include <gtk/gtk.h>

#include <libgupnp/gupnp-control-point.h>
#include <libgupnp-av/gupnp-av.h>

#include "content-directory-service.h"

GUPnPContext *context;
GUPnPControlPoint *cp;
GMainLoop *main_loop;

GtkWidget *window, *treeview;
GtkTreeModel *model;
GtkTextBuffer *textbuffer;

typedef enum _NodeType {
    NODE_ROOT,
    NODE_CONTAINER,
    NODE_ITEM
} NodeType;

typedef struct _NodeData {
    char *id;
    guint update_id;

    NodeType type;
    char *title;
    char *xml;

    GtkTreeRowReference *row;
} NodeData;

typedef struct _UPnPShare {
    GUPnPDeviceProxy *device;
    GUPnPServiceProxy *content_directory;

    GHashTable *id_to_node;
    GNode *filesystem;
    guint system_update_id;
} UPnPShare;

typedef struct _ParserData {
    GNode *parent;
    UPnPShare *share;
    GtkTreeIter *parent_iter;
} ParserData;

static gboolean browse_node (UPnPShare   *share,
                             GNode       *parent,
                             GtkTreeIter *parent_iter,
                             const char  *id);

static GList *shares = NULL; /* List of UPnPShare */

static void
insert_tabs (GString *buffer,
             int      depth)
{
    int i;

    for (i = 0; i < depth; i++) {
        g_string_append (buffer, "  ");
    }
}

static void
dump_node (xmlNode *node,
           GString *buffer,
           int      depth)
{
    gboolean text = TRUE;
    gboolean child_was_text = FALSE;

    if (strcmp ((char *) node->name, "text") != 0) {
        insert_tabs (buffer, depth);
        g_string_append (buffer, "<");
        if (node->ns && node->ns->prefix) {
            g_string_append (buffer, (char *) node->ns->prefix);
            g_string_append (buffer, ":");
        }
        g_string_append (buffer, (char *) node->name);

        if (node->content == NULL && node->children == NULL) {
            g_string_append (buffer, "/>\n");
            return;
        }

        if (node->properties) {
            xmlAttr *attrs;

            for (attrs = node->properties; attrs; attrs = attrs->next) {
                g_string_append (buffer, " ");
                if (attrs->ns && attrs->ns->prefix) {
                    g_string_append (buffer, (char *) attrs->ns->prefix);
                    g_string_append (buffer, ":");
                }
                g_string_append (buffer, (char *) attrs->name);
                g_string_append (buffer, "=\"");
                g_string_append (buffer, (char *) attrs->children->content);
                g_string_append (buffer, "\"");
            }
        }

        g_string_append (buffer, ">");
        text = FALSE;
    }

    if (node->content) {
        g_string_append (buffer, (char *) node->content);
    } else if (node->children) {
        xmlNodePtr children;

        for (children = node->children; children;
             children = children->next) {
            if (strcmp ((char *) children->name, "text") != 0) {
                g_string_append (buffer, "\n");
            } else {
                child_was_text = TRUE;
            }
            dump_node (children, buffer, depth + 1);
        }
    }

    if (text == FALSE) {
        if (child_was_text == FALSE) {
            g_string_append (buffer, "\n");
            insert_tabs (buffer, depth);
        }

        g_string_append (buffer, "</");
        g_string_append (buffer, (char *) node->name);
        g_string_append (buffer, ">");
    }
}

static void
didl_callback (GUPnPDIDLLiteParser *parser,
               GUPnPDIDLLiteObject *didl_object,
               gpointer             userdata)
{
    ParserData *parser_data = userdata;
    GNode *parent = parser_data->parent;
    GNode *child;
    GtkTreeIter child_iter;
    NodeData *nodedata;
    GString *buffer;
    GtkTreePath *path;

    nodedata = g_new (NodeData, 1);
    nodedata->id = g_strdup (gupnp_didl_lite_object_get_id (didl_object));
    nodedata->title = g_strdup (gupnp_didl_lite_object_get_title (didl_object));

    buffer = g_string_new ("");
    dump_node (gupnp_didl_lite_object_get_xml_node (didl_object), buffer, 0);

    nodedata->xml = buffer->str;
    g_string_free (buffer, FALSE);
    if (GUPNP_IS_DIDL_LITE_CONTAINER (didl_object)) {
        nodedata->type = NODE_CONTAINER;
    } else if (GUPNP_IS_DIDL_LITE_ITEM (didl_object)) {
        nodedata->type = NODE_ITEM;
    }

    /* Create the node */
    child = g_node_new (nodedata);
    g_hash_table_insert (parser_data->share->id_to_node, (gpointer)nodedata->id, child);

    gtk_tree_store_append (GTK_TREE_STORE (model), &child_iter,
                           parser_data->parent_iter);
    gtk_tree_store_set (GTK_TREE_STORE (model), &child_iter,
                        0, nodedata->title, 1, child, -1);

    path = gtk_tree_model_get_path (model, &child_iter);
    nodedata->row = gtk_tree_row_reference_new (model, path);
    gtk_tree_path_free (path);

    /* If it has children, parse them */
    if (nodedata->type == NODE_CONTAINER) {
        browse_node (parser_data->share, child, &child_iter, nodedata->id);
    }

    /* Add it to the parent */
    g_node_append (parent, child);
}

static gboolean
browse_node (UPnPShare   *share,
             GNode       *parent,
             GtkTreeIter *parent_iter,
             const char  *id)
{
    guint n_ret, n_match;
    char *didl_result;
    gboolean ret;
    GError *error = NULL;
    int start = 0;
    int amount_left = -1;
    ParserData parser_data;
    GUPnPDIDLLiteParser *parser;

    parser_data.share = share;
    parser_data.parent = parent;

    parser = gupnp_didl_lite_parser_new ();
    g_signal_connect (parser, "object-available", G_CALLBACK (didl_callback), &parser_data);

    parser_data.parent_iter = parent_iter;

    while (start == 0 || amount_left > 0) {
        GNode *node;
        NodeData *nodedata;

        int amount_required = amount_left < 0 ? 10 : MIN (amount_left, 10);

        node = g_hash_table_lookup (share->id_to_node, id);
        if (node == NULL) {
            g_warning ("Do not have a node for %s", id);
            g_assert (node != NULL);
        }

        nodedata = node->data;

        ret = cd_browse (share->content_directory, id, "BrowseDirectChildren",
                         "*", start, amount_required, "", &didl_result,
                         &n_ret, &n_match, &nodedata->update_id, &error);

        if (ret == FALSE) {
            g_warning ("Error browsing %s: %s", id, error->message);
            g_error_free (error);

            g_object_unref (parser);
            return FALSE;
        }

        if (start == 0 && amount_left == -1) {
            amount_left = n_match - n_ret;
        } else {
            amount_left -= n_ret;
        }

        start += n_ret;

        if (!gupnp_didl_lite_parser_parse_didl (parser, didl_result, &error)) {
                g_warning ("Error parsing DIDL: %s", error->message);
                g_error_free (error);
                return FALSE;
        }

        if (amount_left <= 0) {
            break;
        }

    }

    g_object_unref (parser);
    return TRUE;
}

static void
update_system_info (UPnPShare *share)
{
    NodeData *nodedata = share->filesystem->data;

    nodedata->xml = g_strdup_printf ("%s\nSystem Update ID: %u\n",
                                     nodedata->title, share->system_update_id);
}

static gboolean
destroy_node (GNode   *node,
              gpointer userdata)
{
    UPnPShare *share = userdata;
    NodeData *nodedata = node->data;
    GtkTreePath *path;
    GtkTreeIter iter;

    g_print ("Removing %s\n", nodedata->title);
    g_hash_table_remove (share->id_to_node, nodedata->id);

    path = gtk_tree_row_reference_get_path (nodedata->row);
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
    gtk_tree_path_free (path);
    gtk_tree_row_reference_free (nodedata->row);

    g_free (nodedata->id);
    g_free (nodedata->title);
    g_free (nodedata->xml);
    g_free (nodedata);

    /* We don't want to stop emissions so return FALSE */
    return FALSE;
}

static void
unlink_children (UPnPShare *share,
                 GNode     *parent)
{
    GNode *child;

    for (child = parent->children; child;) {
        GNode *old_child;
        g_node_traverse (child, G_POST_ORDER, G_TRAVERSE_ALL, -1,
                         destroy_node, share);

        old_child = child;
        child = old_child->next;

        g_node_destroy (old_child);
    }
}

static void
variable_update_notify_cb (GUPnPServiceProxy *proxy,
                           const char        *variable,
                           GValue            *value,
                           gpointer           userdata)
{
    UPnPShare *share = userdata;

    g_print ("Variable changed: %s\n", variable);
    if (strcmp (variable, "SystemUpdateID") == 0) {
        guint system_id = g_value_get_uint (value);

        if (system_id != share->system_update_id) {
            g_print ("System update id is now: %u\n", system_id);
            share->system_update_id = system_id;

            update_system_info (share);
        }
    } else if (strcmp (variable, "ContainerUpdateIDs") == 0) {
        const char *container_ids;
        char **updates;
        int i;

        container_ids = g_value_get_string (value);
        g_print ("ContainerUpdateIDs: %s\n", container_ids);
        updates = g_strsplit (container_ids, ",", 0);

        /* container_ids is a CSV list of {container_id, container_update_id}
           so updates[i] = container_id, updates[i + 1] = container_update_id
           Stupid upnp spec */

        /* skip moronic servers */
        if (updates[0] == NULL || updates[1] == NULL)
          return;

        for (i = 0; updates[i]; i += 2) {
            GNode *node;
            NodeData *nodedata;
            GtkTreePath *path;
            GtkTreeIter iter;

            node = g_hash_table_lookup (share->id_to_node, updates[i]);
            if (node == NULL) {
                g_warning ("Cannot find node for %s\n", updates[i]);
                continue;
            }

            nodedata = node->data;
            /* Node does need updating */
            if (nodedata->update_id == atoi (updates[i + 1])) {
                continue;
            }

            /* As upnp doesn't tell us what has changed, just that something in
               the container has changed, we need to throw away everything we
               know about the container, and rescan it all. Blah */
            unlink_children (share, node);

            g_print ("Need to update %s (%s)\n", nodedata->title, nodedata->id);

            path = gtk_tree_row_reference_get_path (nodedata->row);
            gtk_tree_model_get_iter (model, &iter, path);
            gtk_tree_path_free (path);

            browse_node (share, node, &iter, nodedata->id);
        }
    }
}

static UPnPShare *
create_share (GUPnPControlPoint *cp,
              GUPnPDeviceInfo   *info)
{
    UPnPShare *share;
    NodeData *nodedata;
    GtkTreeIter iter;
    GtkTreePath *path;
    GError *error = NULL;

    share = g_new0 (UPnPShare, 1);

    share->id_to_node = g_hash_table_new (g_str_hash, g_str_equal);
    share->content_directory =
        (GUPnPServiceProxy *) gupnp_device_info_get_service
        (info, "urn:schemas-upnp-org:service:ContentDirectory:1");

    cd_get_system_update_id (share->content_directory,
                             &share->system_update_id, &error);
    if (error != NULL) {
        g_warning ("Error getting SystemUpdateID: %s", error->message);
        g_error_free (error);
        share->system_update_id = 0;
    }

    nodedata = g_new (NodeData, 1);
    nodedata->id = g_strdup ("0");
    nodedata->title = gupnp_device_info_get_friendly_name (info);
    nodedata->type = NODE_ROOT;
    share->filesystem = g_node_new (nodedata);
    g_hash_table_insert (share->id_to_node, nodedata->id, share->filesystem);

    update_system_info (share);

    /* Add a root node */
    gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                        0, nodedata->title,
                        1, share->filesystem, -1);

    path = gtk_tree_model_get_path (model, &iter);
    nodedata->row = gtk_tree_row_reference_new (model, path);
    gtk_tree_path_free (path);

    gupnp_service_proxy_add_notify (share->content_directory,
                                    "SystemUpdateID", G_TYPE_UINT,
                                    variable_update_notify_cb, share);
    gupnp_service_proxy_add_notify (share->content_directory,
                                    "ContainerUpdateIDs", G_TYPE_STRING,
                                    variable_update_notify_cb, share);
    gupnp_service_proxy_set_subscribed (share->content_directory, TRUE);

    browse_node (share, share->filesystem, &iter, "0");

    return share;
}

static void
device_proxy_available_cb (GUPnPControlPoint *cp,
                           GUPnPDeviceProxy  *proxy,
                           gpointer           userdata)
{
    GUPnPDeviceInfo *info = GUPNP_DEVICE_INFO (proxy);
    GList *services, *s;
    UPnPShare *share;
    int i;

    g_print ("New share available\n");
    g_print ("Location: %s\n", gupnp_device_info_get_location (info));
    g_print ("UDN: %s\n", gupnp_device_info_get_udn (info));
    g_print ("Device Type: %s\n", gupnp_device_info_get_device_type (info));

    services = gupnp_device_info_list_service_types (info);
    for (s = services, i = 0; s; s = s->next, i++) {
        char *type = s->data;

        g_print ("  [%d]: %s\n", i, type);
        g_free (type);
    }

    g_list_free (services);

    share = create_share (cp, info);
    shares = g_list_prepend (shares, share);
}

static void
destroy_window (GtkWidget *window,
                gpointer   userdata)
{
    g_main_loop_quit (main_loop);
}

static void
selection_changed_cb (GtkTreeSelection *selection,
                      gpointer          userdata)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
        GNode *node;
        NodeData *nodedata;

        gtk_tree_model_get (model, &iter, 1, &node, -1);
        nodedata = node->data;

        gtk_text_buffer_set_text (textbuffer, nodedata->xml, -1);
    }
}

static void
make_ui (void)
{
    GtkWidget *scroller, *vbox, *textview;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;
    GtkTreeSelection *selection;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (window, "destroy",
                      G_CALLBACK (destroy_window), NULL);

    vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    scroller = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start (GTK_BOX (vbox), scroller, TRUE, TRUE, 0);

    model = (GtkTreeModel *) gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
    treeview = gtk_tree_view_new_with_model (model);
    g_object_unref (model);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    g_signal_connect (selection, "changed",
                      G_CALLBACK (selection_changed_cb), NULL);

    col = gtk_tree_view_column_new ();
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (col, renderer, TRUE);
    gtk_tree_view_column_add_attribute (col, renderer, "text", 0);

    gtk_container_add (GTK_CONTAINER (scroller), treeview);

    scroller = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start (GTK_BOX (vbox), scroller, TRUE, TRUE, 0);

    textbuffer = gtk_text_buffer_new (NULL);

    textview = gtk_text_view_new_with_buffer (textbuffer);

    gtk_container_add (GTK_CONTAINER (scroller), textview);
    gtk_widget_show_all (window);
}

int
main (int    argc,
      char **argv)
{
    g_thread_init (NULL);
    g_type_init ();

    gtk_init (&argc, &argv);

    make_ui ();

    context = gupnp_context_new (NULL, NULL, 0, NULL);

    cp = gupnp_control_point_new
        (context, "urn:schemas-upnp-org:device:MediaServer:1");

    g_signal_connect (cp, "device-proxy-available",
                      G_CALLBACK (device_proxy_available_cb), NULL);

    /* Start searching */
    gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (cp), TRUE);

    main_loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (main_loop);

    g_main_loop_unref (main_loop);
    g_object_unref (cp);
    g_object_unref (context);

    return 0;
}
