/*
 * Copyright (C) 2014 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebKitNotification.h"

#include "WebKitNotificationPrivate.h"
#include "WebKitPrivate.h"
#include "WebNotification.h"
#include <glib/gi18n-lib.h>
#include <wtf/text/CString.h>

/**
 * SECTION: WebKitNotification
 * @Short_description: Object used to hold information about a notification that should be shown to the user.
 * @Title: WebKitNotification
 *
 * Since: 2.8
 */

enum {
    PROP_0,

    PROP_ID,
    PROP_TITLE,
    PROP_BODY
};

struct _WebKitNotificationPrivate {
    CString title;
    CString body;
    guint64 id;

    WebKitWebView* webView;
};

WEBKIT_DEFINE_TYPE(WebKitNotification, webkit_notification, G_TYPE_OBJECT)

static void webkitNotificationGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* paramSpec)
{
    WebKitNotification* notification = WEBKIT_NOTIFICATION(object);

    switch (propId) {
    case PROP_ID:
        g_value_set_uint64(value, webkit_notification_get_id(notification));
        break;
    case PROP_TITLE:
        g_value_set_string(value, webkit_notification_get_title(notification));
        break;
    case PROP_BODY:
        g_value_set_string(value, webkit_notification_get_body(notification));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
    }
}

static void webkit_notification_class_init(WebKitNotificationClass* notificationClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(notificationClass);
    objectClass->get_property = webkitNotificationGetProperty;

    /**
     * WebKitNotification:id:
     *
     * The unique id for the notification.
     *
     * Since: 2.8
     */
    g_object_class_install_property(objectClass,
        PROP_ID,
        g_param_spec_uint64("id",
            _("ID"),
            _("The unique id for the notification"),
            0, G_MAXUINT64, 0,
            WEBKIT_PARAM_READABLE));

    /**
     * WebKitNotification:title:
     *
     * The title for the notification.
     *
     * Since: 2.8
     */
    g_object_class_install_property(objectClass,
        PROP_TITLE,
        g_param_spec_string("title",
            _("Title"),
            _("The title for the notification"),
            nullptr,
            WEBKIT_PARAM_READABLE));

    /**
     * WebKitNotification:body:
     *
     * The body for the notification.
     *
     * Since: 2.8
     */
    g_object_class_install_property(objectClass,
        PROP_BODY,
        g_param_spec_string("body",
            _("Body"),
            _("The body for the notification"),
            nullptr,
            WEBKIT_PARAM_READABLE));
}

WebKitNotification* webkitNotificationCreate(WebKitWebView* webView, const WebKit::WebNotification& webNotification)
{
    WebKitNotification* notification = WEBKIT_NOTIFICATION(g_object_new(WEBKIT_TYPE_NOTIFICATION, nullptr));
    notification->priv->id = webNotification.notificationID();
    notification->priv->title = webNotification.title().utf8();
    notification->priv->body = webNotification.body().utf8();
    notification->priv->webView = webView;
    return notification;
}

WebKitWebView* webkitNotificationGetWebView(WebKitNotification* notification)
{
    return notification->priv->webView;
}

/**
 * webkit_notification_get_id:
 * @notification: a #WebKitNotification
 *
 * Obtains the unique id for the notification.
 *
 * Returns: the unique id for the notification
 *
 * Since: 2.8
 */
guint64 webkit_notification_get_id(WebKitNotification* notification)
{
    g_return_val_if_fail(WEBKIT_IS_NOTIFICATION(notification), 0);

    return notification->priv->id;
}

/**
 * webkit_notification_get_title:
 * @notification: a #WebKitNotification
 *
 * Obtains the title for the notification.
 *
 * Returns: the title for the notification
 *
 * Since: 2.8
 */
const gchar* webkit_notification_get_title(WebKitNotification* notification)
{
    g_return_val_if_fail(WEBKIT_IS_NOTIFICATION(notification), nullptr);

    return notification->priv->title.data();
}

/**
 * webkit_notification_get_body:
 * @notification: a #WebKitNotification
 *
 * Obtains the body for the notification.
 *
 * Returns: the body for the notification
 *
 * Since: 2.8
 */
const gchar* webkit_notification_get_body(WebKitNotification* notification)
{
    g_return_val_if_fail(WEBKIT_IS_NOTIFICATION(notification), nullptr);

    return notification->priv->body.data();
}
