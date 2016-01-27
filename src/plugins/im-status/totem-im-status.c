/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007, 2011 Bastien Nocera <hadess@hadess.net>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * See license_change file for details.
 *
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>
#include <libpeas/peas-activatable.h>

#include "totem.h"
#include "totem-interface.h"
#include "totem-plugin.h"

#define TOTEM_TYPE_IM_STATUS_PLUGIN		(totem_im_status_plugin_get_type ())
#define TOTEM_IM_STATUS_PLUGIN(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_IM_STATUS_PLUGIN, TotemImStatusPlugin))
#define TOTEM_IM_STATUS_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_IM_STATUS_PLUGIN, TotemImStatusPluginClass))
#define TOTEM_IS_IM_STATUS_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_IM_STATUS_PLUGIN))
#define TOTEM_IS_IM_STATUS_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_IM_STATUS_PLUGIN))
#define TOTEM_IM_STATUS_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_IM_STATUS_PLUGIN, TotemImStatusPluginClass))

typedef struct {
	guint		handler_id_fullscreen;
	guint		handler_id_playing;
	GCancellable   *cancellable;
	gboolean	idle; /* Whether we're idle */
	GDBusProxy     *proxy;
} TotemImStatusPluginPrivate;

enum {
	STATUS_AVAILABLE = 0,
	STATUS_INVISIBLE = 1,
	STATUS_BUSY      = 2,
	STATUS_IDLE      = 3
};

TOTEM_PLUGIN_REGISTER (TOTEM_TYPE_IM_STATUS_PLUGIN, TotemImStatusPlugin, totem_im_status_plugin);

static void
totem_im_status_set_idleness (TotemImStatusPlugin *pi,
			      gboolean             idle)
{
	GVariant *variant;
	GError *error = NULL;

	if (pi->priv->idle == idle)
		return;

	pi->priv->idle = idle;
	variant = g_dbus_proxy_call_sync (pi->priv->proxy,
					  "SetStatus",
					  g_variant_new ("(u)", idle ? STATUS_BUSY : STATUS_AVAILABLE),
					  G_DBUS_CALL_FLAGS_NONE,
					  -1,
					  NULL,
					  &error);
	if (variant == NULL) {
		g_warning ("Failed to set presence: %s", error->message);
		g_error_free (error);
		return;
	}
	g_variant_unref (variant);
}

static void
totem_im_status_update_from_state (TotemObject         *totem,
				   TotemImStatusPlugin *pi)
{
	/* Session Proxy not ready yet */
	if (pi->priv->proxy == NULL)
		return;

	if (totem_is_playing (totem) != FALSE
	    && totem_is_fullscreen (totem) != FALSE) {
		totem_im_status_set_idleness (pi, TRUE);
	} else {
		totem_im_status_set_idleness (pi, FALSE);
	}
}

static void
property_notify_cb (TotemObject         *totem,
		    GParamSpec          *spec,
		    TotemImStatusPlugin *plugin)
{
	totem_im_status_update_from_state (totem, plugin);
}

static void
got_proxy_cb (GObject             *source_object,
	      GAsyncResult        *res,
	      TotemImStatusPlugin *pi)
{
	GError *error = NULL;
	TotemObject *totem;

	pi->priv->proxy = g_dbus_proxy_new_for_bus_finish (res, &error);

	g_object_unref (pi->priv->cancellable);
	pi->priv->cancellable = NULL;

	if (pi->priv->proxy == NULL) {
		g_warning ("Failed to contact session manager: %s", error->message);
		g_error_free (error);
		return;
	}
	g_object_get (pi, "object", &totem, NULL);
	totem_im_status_update_from_state (totem, pi);
	g_object_unref (totem);
}

static void
impl_activate (PeasActivatable *plugin)
{
	TotemImStatusPlugin *pi = TOTEM_IM_STATUS_PLUGIN (plugin);
	TotemObject *totem;

	pi->priv->cancellable = g_cancellable_new ();
	g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
				  G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
				  G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
				  NULL,
				  "org.gnome.SessionManager",
				  "/org/gnome/SessionManager/Presence",
				  "org.gnome.SessionManager.Presence",
				  pi->priv->cancellable,
				  (GAsyncReadyCallback) got_proxy_cb,
				  pi);

	g_object_get (plugin, "object", &totem, NULL);

	pi->priv->handler_id_fullscreen = g_signal_connect (G_OBJECT (totem),
				"notify::fullscreen",
				G_CALLBACK (property_notify_cb),
				pi);
	pi->priv->handler_id_playing = g_signal_connect (G_OBJECT (totem),
				"notify::playing",
				G_CALLBACK (property_notify_cb),
				pi);

	g_object_unref (totem);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	TotemImStatusPlugin *pi = TOTEM_IM_STATUS_PLUGIN (plugin);
	TotemObject *totem;

	/* In flight? */
	if (pi->priv->cancellable != NULL) {
		g_cancellable_cancel (pi->priv->cancellable);
		g_object_unref (pi->priv->cancellable);
		pi->priv->cancellable = NULL;
	}

	if (pi->priv->proxy != NULL) {
		g_object_unref (pi->priv->proxy);
		pi->priv->proxy = NULL;
	}

	g_object_get (plugin, "object", &totem, NULL);

	if (pi->priv->handler_id_fullscreen != 0) {
		g_signal_handler_disconnect (G_OBJECT (totem),
					     pi->priv->handler_id_fullscreen);
		pi->priv->handler_id_fullscreen = 0;
	}
	if (pi->priv->handler_id_playing != 0) {
		g_signal_handler_disconnect (G_OBJECT (totem),
					     pi->priv->handler_id_playing);
		pi->priv->handler_id_playing = 0;
	}

	g_object_unref (totem);
}
