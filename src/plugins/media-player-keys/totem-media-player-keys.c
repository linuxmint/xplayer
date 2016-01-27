/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Jan Arne Petersen <jap@gnome.org>
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
#include <gmodule.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>
#include <libpeas/peas-activatable.h>
#include <string.h>

#include "totem-plugin.h"
#include "totem.h"

#define TOTEM_TYPE_MEDIA_PLAYER_KEYS_PLUGIN		(totem_media_player_keys_plugin_get_type ())
#define TOTEM_MEDIA_PLAYER_KEYS_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_MEDIA_PLAYER_KEYS_PLUGIN, TotemMediaPlayerKeysPlugin))
#define TOTEM_MEDIA_PLAYER_KEYS_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_MEDIA_PLAYER_KEYS_PLUGIN, TotemMediaPlayerKeysPluginClass))
#define TOTEM_IS_MEDIA_PLAYER_KEYS_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_MEDIA_PLAYER_KEYS_PLUGIN))
#define TOTEM_IS_MEDIA_PLAYER_KEYS_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_MEDIA_PLAYER_KEYS_PLUGIN))
#define TOTEM_MEDIA_PLAYER_KEYS_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_MEDIA_PLAYER_KEYS_PLUGIN, TotemMediaPlayerKeysPluginClass))

typedef struct {
	GDBusProxy    *proxy;
	guint          handler_id;
	guint          watch_id;
	GCancellable  *cancellable_init;
	GCancellable  *cancellable;
} TotemMediaPlayerKeysPluginPrivate;

TOTEM_PLUGIN_REGISTER(TOTEM_TYPE_MEDIA_PLAYER_KEYS_PLUGIN,
		      TotemMediaPlayerKeysPlugin,
		      totem_media_player_keys_plugin);

static void
on_media_player_key_pressed (TotemObject *totem,
			     const gchar *key)
{
	if (strcmp ("Play", key) == 0)
		totem_action_play_pause (totem);
	else if (strcmp ("Previous", key) == 0)
		totem_action_previous (totem);
	else if (strcmp ("Next", key) == 0)
		totem_action_next (totem);
	else if (strcmp ("Stop", key) == 0)
		totem_action_pause (totem);
	else if (strcmp ("FastForward", key) == 0)
		totem_action_remote (totem, TOTEM_REMOTE_COMMAND_SEEK_FORWARD, NULL);
	else if (strcmp ("Rewind", key) == 0)
		totem_action_remote (totem, TOTEM_REMOTE_COMMAND_SEEK_BACKWARD, NULL);
	else if (strcmp ("Repeat", key) == 0) {
		gboolean value;

		value = totem_action_remote_get_setting (totem, TOTEM_REMOTE_SETTING_REPEAT);
		totem_action_remote_set_setting (totem, TOTEM_REMOTE_SETTING_REPEAT, !value);
	} else if (strcmp ("Shuffle", key) == 0) {
		gboolean value;

		value = totem_action_remote_get_setting (totem, TOTEM_REMOTE_SETTING_SHUFFLE);
		totem_action_remote_set_setting (totem, TOTEM_REMOTE_SETTING_SHUFFLE, !value);
	}
}

static void
grab_media_player_keys_cb (GDBusProxy                 *proxy,
			   GAsyncResult               *res,
			   TotemMediaPlayerKeysPlugin *pi)
{
	GVariant *variant;
	GError *error = NULL;

	variant = g_dbus_proxy_call_finish (proxy, res, &error);
	pi->priv->cancellable = NULL;

	if (variant == NULL) {
		if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			g_warning ("Failed to call \"GrabMediaPlayerKeys\": %s", error->message);
		g_error_free (error);
		return;
	}
	g_variant_unref (variant);

	g_object_unref (pi);
}

static void
grab_media_player_keys (TotemMediaPlayerKeysPlugin *pi)
{
	GCancellable *cancellable;

	if (pi->priv->proxy == NULL)
		return;

	/* Only allow one key grab operation to happen concurrently */
	if (pi->priv->cancellable) {
		g_cancellable_cancel (pi->priv->cancellable);
	}

	cancellable = g_cancellable_new ();
	pi->priv->cancellable = cancellable;

	g_dbus_proxy_call (pi->priv->proxy,
					  "GrabMediaPlayerKeys",
					  g_variant_new ("(su)", "Totem", 0),
					  G_DBUS_CALL_FLAGS_NONE,
					  -1, cancellable,
					  (GAsyncReadyCallback) grab_media_player_keys_cb,
					  g_object_ref (pi));

	/* GDBus keeps a reference throughout the async call */
	g_object_unref (cancellable);
}

static gboolean
on_window_focus_in_event (GtkWidget                  *window,
			  GdkEventFocus              *event,
			  TotemMediaPlayerKeysPlugin *pi)
{
	grab_media_player_keys (pi);

	return FALSE;
}

static void
key_pressed (GDBusProxy                 *proxy,
	     gchar                      *sender_name,
	     gchar                      *signal_name,
	     GVariant                   *parameters,
	     TotemMediaPlayerKeysPlugin *pi)
{
	char *app, *cmd;

	if (g_strcmp0 (signal_name, "MediaPlayerKeyPressed") != 0)
		return;
	g_variant_get (parameters, "(ss)", &app, &cmd);
	if (g_strcmp0 (app, "Totem") == 0) {
		TotemObject *totem;

		totem = g_object_get_data (G_OBJECT (pi), "object");
		on_media_player_key_pressed (totem, cmd);
	}
	g_free (app);
	g_free (cmd);
}

static void
got_proxy_cb (GObject                    *source_object,
	      GAsyncResult               *res,
	      TotemMediaPlayerKeysPlugin *pi)
{
	GError *error = NULL;

	pi->priv->proxy = g_dbus_proxy_new_for_bus_finish (res, &error);
	pi->priv->cancellable_init = NULL;

	if (pi->priv->proxy == NULL) {
		if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			g_warning ("Failed to contact settings daemon: %s", error->message);
		g_error_free (error);
		return;
	}

	grab_media_player_keys (pi);

	g_signal_connect (G_OBJECT (pi->priv->proxy), "g-signal",
			  G_CALLBACK (key_pressed), pi);

	g_object_unref (pi);
}

static void
name_appeared_cb (GDBusConnection            *connection,
		  const gchar                *name,
		  const gchar                *name_owner,
		  TotemMediaPlayerKeysPlugin *pi)
{
	GCancellable *cancellable;

	cancellable = g_cancellable_new ();
	pi->priv->cancellable_init = cancellable;

	g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
				  G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
				  G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
				  NULL,
				  "org.gnome.SettingsDaemon",
				  "/org/gnome/SettingsDaemon/MediaKeys",
				  "org.gnome.SettingsDaemon.MediaKeys",
				  cancellable,
				  (GAsyncReadyCallback) got_proxy_cb,
				  g_object_ref (pi));

	/* GDBus keeps a reference throughout the async call */
	g_object_unref (cancellable);
}

static void
name_vanished_cb (GDBusConnection            *connection,
		  const gchar                *name,
		  TotemMediaPlayerKeysPlugin *pi)
{
	if (pi->priv->proxy != NULL) {
		g_object_unref (pi->priv->proxy);
		pi->priv->proxy = NULL;
	}

	if (pi->priv->cancellable) {
		g_cancellable_cancel (pi->priv->cancellable);
	}
}

static void
impl_activate (PeasActivatable *plugin)
{
	TotemMediaPlayerKeysPlugin *pi = TOTEM_MEDIA_PLAYER_KEYS_PLUGIN (plugin);
	TotemObject *totem;
	GtkWindow *window;

	pi->priv->watch_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
					       "org.gnome.SettingsDaemon",
					       G_BUS_NAME_WATCHER_FLAGS_NONE,
					       (GBusNameAppearedCallback) name_appeared_cb,
					       (GBusNameVanishedCallback) name_vanished_cb,
					       g_object_ref (pi), (GDestroyNotify) g_object_unref);

	totem = g_object_get_data (G_OBJECT (plugin), "object");
	window = totem_get_main_window (totem);
	pi->priv->handler_id = g_signal_connect (G_OBJECT (window), "focus-in-event",
						 G_CALLBACK (on_window_focus_in_event), pi);

	g_object_unref (G_OBJECT (window));
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	TotemMediaPlayerKeysPlugin *pi = TOTEM_MEDIA_PLAYER_KEYS_PLUGIN (plugin);
	GtkWindow *window;

	if (pi->priv->cancellable_init) {
		g_cancellable_cancel (pi->priv->cancellable_init);
	}

	if (pi->priv->cancellable) {
		g_cancellable_cancel (pi->priv->cancellable);
	}

	if (pi->priv->proxy != NULL) {
		g_object_unref (pi->priv->proxy);
		pi->priv->proxy = NULL;
	}

	if (pi->priv->handler_id != 0) {
		TotemObject *totem;

		totem = g_object_get_data (G_OBJECT (plugin), "object");
		window = totem_get_main_window (totem);
		if (window == NULL)
			return;

		g_signal_handler_disconnect (G_OBJECT (window), pi->priv->handler_id);

		g_object_unref (window);
		pi->priv->handler_id = 0;
	}
	if (pi->priv->watch_id != 0) {
		g_bus_unwatch_name (pi->priv->watch_id);
		pi->priv->watch_id = 0;
	}
}

