/*
 * Copyright (C) 2016 Jens Georg <mail@jensge.org>
 *
 * Authors: Jens Georg <mail@jensge.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>

#include <string.h>

#include "server-device.h"
#include "icons.h"

#define CONTENT_DIR "urn:schemas-upnp-org:service:ContentDirectory"

#define AV_CP_MEDIA_SERVER_ERROR av_cp_media_server_error_quark()

GQuark
av_cp_media_server_error_quark (void);

typedef enum _AVCPMediaServerError {
        AV_CP_MEDIA_SERVER_FAILED
} AVCPMediaServerError;

static void
av_cp_media_server_async_intable_init (gpointer iface, gpointer iface_data);

static void
av_cp_media_server_init_async (GAsyncInitable      *initable,
                               int                  io_priority,
                               GCancellable        *cancellable,
                               GAsyncReadyCallback  callback,
                               gpointer             user_data);

static gboolean
av_cp_media_server_init_finish (GAsyncInitable *initable,
                                GAsyncResult   *res,
                                GError         **error);
static void
av_cp_media_server_introspect (AVCPMediaServer *self);

static void
av_cp_media_server_introspect_finish (AVCPMediaServer *self);

static void
av_cp_media_server_on_get_search_caps (GObject *object,
                                       GAsyncResult *res,
                                       gpointer user_data);

enum _AVCPMediaServerInitState {
        NONE = 0,
        INITIALIZED,
        INIT_FAILED,
        INITIALIZING
};
typedef enum _AVCPMediaServerInitState AVCPMediaServerInitState;

struct _AVCPMediaServerPrivate {
        GdkPixbuf *icon;
        char *default_sort_order;
        char **search_caps;
        GList *tasks;
        AVCPMediaServerInitState state;
        GUPnPServiceProxy *content_directory;
};
typedef struct _AVCPMediaServerPrivate AVCPMediaServerPrivate;

G_DEFINE_TYPE_WITH_CODE (AVCPMediaServer,
                         av_cp_media_server,
                         GUPNP_TYPE_DEVICE_PROXY,
                         G_ADD_PRIVATE (AVCPMediaServer)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE,
                                                av_cp_media_server_async_intable_init))

enum
{
        PROP_ICON = 1,
        PROP_SORT_ORDER,
        PROP_SEARCH_CAPS,
        N_PROPERTIES
};

static GParamSpec *av_cp_media_server_properties[N_PROPERTIES] = { NULL, };

/* GObject overrides */

static void
av_cp_media_server_finalize (GObject *object)
{
        AVCPMediaServer *self = AV_CP_MEDIA_SERVER (object);
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);
        GObjectClass *parent_class =
                              G_OBJECT_CLASS (av_cp_media_server_parent_class);

        g_clear_pointer (&priv->default_sort_order, g_free);

        parent_class->finalize (object);
}

static void
av_cp_media_server_dispose (GObject *object)
{
        AVCPMediaServer *self = AV_CP_MEDIA_SERVER (object);
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);
        GObjectClass *parent_class =
                              G_OBJECT_CLASS (av_cp_media_server_parent_class);

        g_clear_object (&priv->icon);
        g_clear_object (&priv->content_directory);
        g_clear_pointer (&priv->search_caps, g_strfreev);

        parent_class->dispose (object);
}

static void
av_cp_media_server_get_property (GObject    *obj,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *spec)
{
        AVCPMediaServer *self = AV_CP_MEDIA_SERVER (obj);
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);

        switch (property_id) {
        case PROP_ICON:
                g_value_set_object (value, priv->icon);
                break;
        case PROP_SORT_ORDER:
                g_value_set_string (value, priv->default_sort_order);
                break;
        case PROP_SEARCH_CAPS:
                g_value_set_pointer (value, priv->search_caps);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, spec);
                break;
        }
}

static void
av_cp_media_server_on_icon_updated (GObject *source,
                                    GAsyncResult *res,
                                    gpointer user_data);

static void
av_cp_media_server_on_get_sort_caps (GObject      *object,
                                     GAsyncResult *res,
                                     gpointer      user_data)
{
        AVCPMediaServer *self = AV_CP_MEDIA_SERVER (user_data);
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);
        GUPnPServiceProxyAction *action;

        GError *error = NULL;
        char *sort_caps = NULL;

        action = gupnp_service_proxy_call_action_finish
                                                (GUPNP_SERVICE_PROXY (object),
                                                 res,
                                                 &error);

        if (error != NULL) {
                g_warning ("GetSortCapabilities call failed: %s",
                           error->message);
                g_clear_error (&error);
        } else {
                gupnp_service_proxy_action_get_result (action,
                                                       &error,
                                                       "SortCaps",
                                                       G_TYPE_STRING,
                                                       &sort_caps,
                                                       NULL);
                if (error != NULL) {
                        g_warning ("Failed to get sort caps from server: %s",
                                error->message);
                        g_error_free (error);
                } else if (sort_caps != NULL) {
                        GString *default_sort_order = g_string_new (NULL);
                        if (strstr (sort_caps, "upnp:class") != NULL) {
                                g_string_append (default_sort_order,
                                                 "+upnp:class,");
                        }

                        if (strstr (sort_caps, "dc:title") != NULL) {
                                g_string_append (default_sort_order,
                                                 "+dc:title");
                        }

                        priv->default_sort_order =
                                        g_string_free (default_sort_order, FALSE);

                        g_free (sort_caps);
                }
        }
        g_object_notify (G_OBJECT (self), "sort-order");

        action = gupnp_service_proxy_action_new ("GetSearchCapabilities",
                                                 NULL);

        gupnp_service_proxy_call_action_async
                                       (priv->content_directory,
                                        action,
                                        NULL,
                                        av_cp_media_server_on_get_search_caps,
                                        self);

        gupnp_service_proxy_action_unref (action);
}

static void
av_cp_media_server_on_get_search_caps (GObject      *object,
                                       GAsyncResult *res,
                                       gpointer      user_data)
{
        AVCPMediaServer *self = AV_CP_MEDIA_SERVER (user_data);
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);
        GUPnPServiceProxyAction *action;

        GError *error = NULL;
        char *search_caps = NULL;

        action = gupnp_service_proxy_call_action_finish
                                                (GUPNP_SERVICE_PROXY (object),
                                                 res,
                                                 &error);

        if (error != NULL) {
                g_warning ("GetSearchCapabilites call failed: %s",
                           error->message);
                g_clear_error (&error);
        } else {
                gupnp_service_proxy_action_get_result (action,
                                                       &error,
                                                       "SearchCaps",
                                                       G_TYPE_STRING,
                                                       &search_caps,
                                                       NULL);
                if (error != NULL) {
                        g_warning ("Failed to get sort caps from server: %s",
                                error->message);
                        g_error_free (error);
                } else if (search_caps != NULL) {
                        priv->search_caps = g_strsplit (search_caps, ",", -1);
                } else {
                        priv->search_caps = g_strsplit ("upnp:class,@id", ",", -1);
                }

                g_free (search_caps);
        }
        g_object_notify (G_OBJECT (self), "search-caps");

        priv->state = INITIALIZED;
        av_cp_media_server_introspect_finish (self);
        g_object_unref (self);
}

static void
av_cp_media_server_class_init (AVCPMediaServerClass *klass)
{
        GObjectClass *obj_class = G_OBJECT_CLASS (klass);

        obj_class->get_property = av_cp_media_server_get_property;
        obj_class->dispose = av_cp_media_server_dispose;
        obj_class->finalize = av_cp_media_server_finalize;

        av_cp_media_server_properties[PROP_ICON] =
                g_param_spec_object ("icon",
                                     "icon",
                                     "icon",
                                     GDK_TYPE_PIXBUF,
                                     G_PARAM_STATIC_STRINGS |
                                     G_PARAM_READABLE);

        av_cp_media_server_properties[PROP_SORT_ORDER] =
                g_param_spec_string ("sort-order",
                                     "sort-order",
                                     "sort-order",
                                     NULL,
                                     G_PARAM_STATIC_STRINGS |
                                     G_PARAM_READABLE);

        av_cp_media_server_properties[PROP_SEARCH_CAPS] =
                g_param_spec_pointer ("search-caps",
                                      "search-caps",
                                      "search-caps",
                                      G_PARAM_STATIC_STRINGS |
                                      G_PARAM_READABLE);

        g_object_class_install_properties (obj_class,
                                           N_PROPERTIES,
                                           av_cp_media_server_properties);
}

static void
av_cp_media_server_init (AVCPMediaServer *self)
{
}

static void
av_cp_media_server_on_icon_updated (GObject *source,
                                    GAsyncResult *res,
                                    gpointer user_data)
{
        AVCPMediaServer *self = AV_CP_MEDIA_SERVER (source);
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);
        g_autoptr (GError) error = NULL;

        priv->icon =
                update_icon_finish (GUPNP_DEVICE_INFO (source), res, &error);
        if (error != NULL) {
                g_debug ("Failed to download device icon: %s", error->message);
        }

        av_cp_media_server_get_content_directory (self);

        if (priv->content_directory != NULL) {
                GUPnPServiceProxyAction *action;

                action = gupnp_service_proxy_action_new ("GetSortCapabilities",
                                                         NULL);

                gupnp_service_proxy_call_action_async
                                        (priv->content_directory,
                                         action,
                                         NULL,
                                         av_cp_media_server_on_get_sort_caps,
                                         g_object_ref (self));

                gupnp_service_proxy_action_unref (action);
        } else {
                g_debug ("Invalid MediaServer device without ContentDirectory");
                priv->state = INIT_FAILED;
                av_cp_media_server_introspect_finish (self);
        }
}


static void
av_cp_media_server_async_intable_init (gpointer iface, gpointer iface_data)
{
        GAsyncInitableIface *aii = (GAsyncInitableIface *)iface;
        aii->init_async = av_cp_media_server_init_async;
        aii->init_finish = av_cp_media_server_init_finish;
}

static void
av_cp_media_server_init_async (GAsyncInitable      *initable,
                               int                  io_priority,
                               GCancellable        *cancellable,
                               GAsyncReadyCallback  callback,
                               gpointer             user_data)
{
        AVCPMediaServer *self = AV_CP_MEDIA_SERVER (initable);
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);
        GTask *task = NULL;

        task = g_task_new (initable, cancellable, callback, user_data);
        switch (priv->state) {
                case INITIALIZED:
                        g_task_return_boolean (task, TRUE);
                        g_object_unref (task);

                        break;
                case INIT_FAILED:
                        g_task_return_new_error (task,
                                                 AV_CP_MEDIA_SERVER_ERROR,
                                                 AV_CP_MEDIA_SERVER_FAILED,
                                                 "Initialisation failed");
                        g_object_unref (task);
                        break;
                case NONE:
                        priv->state = INITIALIZING;
                        priv->tasks = g_list_prepend (priv->tasks,
                                                            task);
                        av_cp_media_server_introspect (self);
                        break;
                case INITIALIZING:
                        priv->tasks = g_list_prepend (priv->tasks,
                                                            task);
                        break;
                default:
                        g_assert_not_reached ();
                        break;
        }
}

static gboolean
av_cp_media_server_init_finish (GAsyncInitable *initable,
                                GAsyncResult   *res,
                                GError         **error)
{
        g_return_val_if_fail (g_task_is_valid (res, initable), FALSE);

        return g_task_propagate_boolean (G_TASK (res), error);
}

static void
av_cp_media_server_introspect (AVCPMediaServer *self)
{
        update_icon_async (GUPNP_DEVICE_INFO (self),
                           NULL,
                           av_cp_media_server_on_icon_updated,
                           NULL);
}

static void
av_cp_media_server_introspect_finish (AVCPMediaServer *self)
{
        GList *l;
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);

        for (l = priv->tasks; l != NULL; l = l->next) {
                GTask *task = (GTask *) l->data;

                if (priv->state == INITIALIZED) {
                        g_task_return_boolean (task, TRUE);
                } else {
                        g_task_return_new_error (task,
                                                 AV_CP_MEDIA_SERVER_ERROR,
                                                 AV_CP_MEDIA_SERVER_FAILED,
                                                 "Initialisation failed");
                }

                g_object_unref (task);
        }

        g_clear_pointer (&priv->tasks, g_list_free);
}

GQuark
av_cp_media_server_error_quark (void)
{
        return g_quark_from_static_string ("av-cp-media-server-error-quark");
}


GUPnPServiceProxy *
av_cp_media_server_get_content_directory (AVCPMediaServer *self)
{
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);
        if (priv->content_directory == NULL) {
                GUPnPServiceInfo *info = gupnp_device_info_get_service
                                (GUPNP_DEVICE_INFO (self),
                                 CONTENT_DIR);
                priv->content_directory =  GUPNP_SERVICE_PROXY (info);
        }

        return GUPNP_SERVICE_PROXY (g_object_ref (priv->content_directory));
}

typedef struct _BrowseReturn {
        char    *didl_xml;
        guint32  number_returned;
        guint32  total_matches;
} BrowseReturn;

static void
av_cp_media_server_on_browse (GObject      *source,
                              GAsyncResult *result,
                              gpointer      user_data)
{
        GTask   *task = G_TASK (user_data);
        GError  *error = NULL;
        char    *didl_xml = NULL;
        guint32  number_returned;
        guint32  total_matches;

        GUPnPServiceProxyAction *action;

        action = gupnp_service_proxy_call_action_finish
                                                (GUPNP_SERVICE_PROXY (source),
                                                 result,
                                                 &error);
        if (error != NULL) {
                g_task_return_error (task, error);
        } else {
                gupnp_service_proxy_action_get_result (action,
                                                       &error,
                                                       /* OUT args */
                                                       "Result",
                                                       G_TYPE_STRING,
                                                       &didl_xml,
                                                       "NumberReturned",
                                                       G_TYPE_UINT,
                                                       &number_returned,
                                                       "TotalMatches",
                                                       G_TYPE_UINT,
                                                       &total_matches,
                                                       NULL);

                if (error != NULL) {
                        g_task_return_error (task, error);
                } else {
                        BrowseReturn *ret = g_new0 (BrowseReturn, 1);
                        ret->didl_xml = didl_xml;
                        ret->number_returned = number_returned;
                        ret->total_matches = total_matches;

                        g_task_return_pointer (task, ret, g_free);
                }
        }

        g_object_unref (task);
}

void
av_cp_media_server_browse_async (AVCPMediaServer     *self,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 const char          *container_id,
                                 guint32              starting_index,
                                 guint32              requested_count,
                                 gpointer             user_data)
{
        GTask *task = g_task_new (self, cancellable, callback, user_data);
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);
        const char *sort_order =  priv->default_sort_order == NULL ?
                                         "" :
                                         priv->default_sort_order;
        GUPnPServiceProxyAction *action = NULL;

        action = gupnp_service_proxy_action_new ("Browse",
                                                 /* IN args */
                                                 "ObjectID",
                                                 G_TYPE_STRING,
                                                 container_id,
                                                 "BrowseFlag",
                                                 G_TYPE_STRING,
                                                 "BrowseDirectChildren",
                                                 "Filter",
                                                 G_TYPE_STRING,
                                                 "@childCount",
                                                 "StartingIndex",
                                                 G_TYPE_UINT,
                                                 starting_index,
                                                 "RequestedCount",
                                                 G_TYPE_UINT,
                                                 requested_count,
                                                 "SortCriteria",
                                                 G_TYPE_STRING,
                                                 sort_order,
                                                 NULL);

        gupnp_service_proxy_call_action_async (priv->content_directory,
                                               action,
                                               g_task_get_cancellable (task),
                                               av_cp_media_server_on_browse,
                                               task);

        gupnp_service_proxy_action_unref (action);
}

gboolean
av_cp_media_server_browse_finish (AVCPMediaServer  *self,
                                  GAsyncResult     *result,
                                  char            **didl_xml,
                                  guint32          *total_matches,
                                  guint32          *number_returned,
                                  GError          **error)
{
        BrowseReturn *res;

        g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

        res = (BrowseReturn *)g_task_propagate_pointer (G_TASK (result), error);
        if (res != NULL) {
                if (didl_xml != NULL) {
                        *didl_xml = res->didl_xml;
                } else {
                        g_free (res->didl_xml);
                }

                if (total_matches != NULL) {
                        *total_matches = res->total_matches;
                }

                if (number_returned != NULL) {
                        *number_returned = res->number_returned;
                }

                g_free (res);

                return TRUE;
        }

        return FALSE;
}

static void
av_cp_media_server_on_browse_metadata (GObject      *object,
                                       GAsyncResult *res,
                                       gpointer      user_data)
{
        GTask   *task = G_TASK (user_data);
        GError  *error = NULL;
        char    *didl_xml = NULL;
        GUPnPServiceProxyAction *action = NULL;

        action = gupnp_service_proxy_call_action_finish
                                                (GUPNP_SERVICE_PROXY (object),
                                                                      res,
                                                                      &error);

        if (error != NULL) {
                g_task_return_error (task, error);
        } else {
                gupnp_service_proxy_action_get_result (action,
                                                       &error,
                                                       /* OUT args */
                                                       "Result",
                                                       G_TYPE_STRING,
                                                       &didl_xml,
                                                       NULL);
                if (error != NULL) {
                        g_task_return_error (task, error);
                } else {
                        g_task_return_pointer (task, didl_xml, g_free);
                }
        }

        g_object_unref (task);
}

void
av_cp_media_server_browse_metadata_async (AVCPMediaServer     *self,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback  callback,
                                          const char          *id,
                                          gpointer             user_data)
{
        GTask *task = g_task_new (self, cancellable, callback, user_data);
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);
        GUPnPServiceProxyAction *action;

        action = gupnp_service_proxy_action_new ("Browse",
                                                 /* IN args */
                                                 "ObjectID",
                                                 G_TYPE_STRING,
                                                 id,
                                                 "BrowseFlag",
                                                 G_TYPE_STRING,
                                                 "BrowseMetadata",
                                                 "Filter",
                                                 G_TYPE_STRING,
                                                 "*",
                                                 "StartingIndex",
                                                 G_TYPE_UINT,
                                                 0,
                                                 "RequestedCount",
                                                 G_TYPE_UINT, 0,
                                                 "SortCriteria",
                                                 G_TYPE_STRING,
                                                 "",
                                                 NULL);

        gupnp_service_proxy_call_action_async
                                (priv->content_directory,
                                 action,
                                 g_task_get_cancellable (task),
                                 av_cp_media_server_on_browse_metadata,
                                 task);

        gupnp_service_proxy_action_unref (action);
}

gboolean
av_cp_media_server_browse_metadata_finish (AVCPMediaServer  *self,
                                           GAsyncResult     *result,
                                           char            **didl_xml,
                                           GError          **error)
{
        char *res;

        g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

        res = (char *)g_task_propagate_pointer (G_TASK (result), error);
        if (res != NULL) {
                if (didl_xml != NULL) {
                        *didl_xml = res;
                } else {
                        g_free (res);
                }

                return TRUE;
        }

        return FALSE;
}

void
av_cp_media_server_search_async (AVCPMediaServer     *self,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 const char          *container_id,
                                 const char          *search_criteria,
                                 const char          *filter,
                                 guint32              starting_index,
                                 guint32              requested_count,
                                 gpointer             user_data)
{
        GTask *task = g_task_new (self, cancellable, callback, user_data);
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);

        GUPnPServiceProxyAction *action;
        action = gupnp_service_proxy_action_new ("Search",
                                                /* IN args */
                                                "ContainerID",
                                                G_TYPE_STRING,
                                                container_id,
                                                "SearchCriteria",
                                                G_TYPE_STRING,
                                                search_criteria,
                                                "Filter",
                                                G_TYPE_STRING,
                                                "*",
                                                "StartingIndex",
                                                G_TYPE_UINT,
                                                starting_index,
                                                "RequestedCount",
                                                G_TYPE_UINT,
                                                requested_count,
                                                "SortCriteria",
                                                G_TYPE_STRING,
                                                "",
                                                NULL);

        gupnp_service_proxy_call_action_async (priv->content_directory,
                                               action,
                                               g_task_get_cancellable (task),
                                               av_cp_media_server_on_browse,
                                               task);

        gupnp_service_proxy_action_unref (action);
}

gboolean
av_cp_media_server_search_finish (AVCPMediaServer  *self,
                                  GAsyncResult     *result,
                                  char            **didl_xml,
                                  guint32          *total_matches,
                                  guint32          *number_returned,
                                  GError          **error)
{
        return av_cp_media_server_browse_finish (self, result, didl_xml,
                total_matches, number_returned, error);
}

char const * const *
av_cp_media_server_get_search_caps (AVCPMediaServer *self) {
        AVCPMediaServerPrivate *priv = av_cp_media_server_get_instance_private (self);

        return (char const * const *) priv->search_caps;
}
