/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
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
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>
#include <libpeas/peas-activatable.h>
#include <string.h>

#include "totem-plugin.h"
#include "totem.h"
#include "backend/bacon-video-widget.h"

#define TOTEM_TYPE_SCREENSAVER_PLUGIN		(totem_screensaver_plugin_get_type ())
#define TOTEM_SCREENSAVER_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_SCREENSAVER_PLUGIN, TotemScreensaverPlugin))
#define TOTEM_SCREENSAVER_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_SCREENSAVER_PLUGIN, TotemScreensaverPluginClass))
#define TOTEM_IS_SCREENSAVER_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_SCREENSAVER_PLUGIN))
#define TOTEM_IS_SCREENSAVER_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_SCREENSAVER_PLUGIN))
#define TOTEM_SCREENSAVER_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_SCREENSAVER_PLUGIN, TotemScreensaverPluginClass))

typedef struct {
	TotemObject *totem;
	BaconVideoWidget *bvw;
	GSettings *settings;

	guint          handler_id_playing;
	guint          handler_id_metadata;
	guint          inhibit_cookie;
} TotemScreensaverPluginPrivate;

TOTEM_PLUGIN_REGISTER(TOTEM_TYPE_SCREENSAVER_PLUGIN,
		      TotemScreensaverPlugin,
		      totem_screensaver_plugin)

static gboolean
has_video (BaconVideoWidget *bvw)
{
	GValue value = { 0, };
	gboolean ret;

	bacon_video_widget_get_metadata (bvw, BVW_INFO_HAS_VIDEO, &value);
	ret = g_value_get_boolean (&value);
	g_value_unset (&value);

	return ret;
}

static void
totem_screensaver_update_from_state (TotemObject *totem,
				     TotemScreensaverPlugin *pi)
{
	gboolean lock_screensaver_on_audio, has_video_frames;
	BaconVideoWidget *bvw;

	bvw = BACON_VIDEO_WIDGET (totem_get_video_widget ((Totem *)(totem)));

	lock_screensaver_on_audio = g_settings_get_boolean (pi->priv->settings, "lock-screensaver-on-audio");
	has_video_frames = has_video (bvw);

	if ((totem_is_playing (totem) != FALSE && has_video_frames) ||
	    (totem_is_playing (totem) != FALSE && !lock_screensaver_on_audio)) {
		if (pi->priv->inhibit_cookie == 0) {
			GtkWindow *window;

			window = totem_get_main_window (totem);
			pi->priv->inhibit_cookie = gtk_application_inhibit (GTK_APPLICATION (totem),
										window,
										GTK_APPLICATION_INHIBIT_IDLE,
										_("Playing a movie"));
			g_object_unref (window);
		}
	} else {
		if (pi->priv->inhibit_cookie != 0) {
			gtk_application_uninhibit (GTK_APPLICATION (pi->priv->totem), pi->priv->inhibit_cookie);
			pi->priv->inhibit_cookie = 0;
		}
	}
}

static void
property_notify_cb (TotemObject *totem,
		    GParamSpec *spec,
		    TotemScreensaverPlugin *pi)
{
	totem_screensaver_update_from_state (totem, pi);
}

static void
got_metadata_cb (BaconVideoWidget *bvw, TotemScreensaverPlugin *pi)
{
	totem_screensaver_update_from_state (pi->priv->totem, pi);
}

static void
lock_screensaver_on_audio_changed_cb (GSettings *settings, const gchar *key, TotemScreensaverPlugin *pi)
{
	totem_screensaver_update_from_state (pi->priv->totem, pi);
}

static void
impl_activate (PeasActivatable *plugin)
{
	TotemScreensaverPlugin *pi = TOTEM_SCREENSAVER_PLUGIN (plugin);
	TotemObject *totem;

	totem = g_object_get_data (G_OBJECT (plugin), "object");
	pi->priv->bvw = BACON_VIDEO_WIDGET (totem_get_video_widget (totem));

	pi->priv->settings = g_settings_new (TOTEM_GSETTINGS_SCHEMA);
	g_signal_connect (pi->priv->settings, "changed::lock-screensaver-on-audio", (GCallback) lock_screensaver_on_audio_changed_cb, plugin);

	pi->priv->handler_id_playing = g_signal_connect (G_OBJECT (totem),
						   "notify::playing",
						   G_CALLBACK (property_notify_cb),
						   pi);
	pi->priv->handler_id_metadata = g_signal_connect (G_OBJECT (pi->priv->bvw),
						    "got-metadata",
						    G_CALLBACK (got_metadata_cb),
						    pi);

	pi->priv->totem = g_object_ref (totem);

	/* Force setting the current status */
	totem_screensaver_update_from_state (totem, pi);
}

static void
impl_deactivate	(PeasActivatable *plugin)
{
	TotemScreensaverPlugin *pi = TOTEM_SCREENSAVER_PLUGIN (plugin);

	g_object_unref (pi->priv->settings);

	if (pi->priv->handler_id_playing != 0) {
		TotemObject *totem;
		totem = g_object_get_data (G_OBJECT (plugin), "object");
		g_signal_handler_disconnect (G_OBJECT (totem), pi->priv->handler_id_playing);
		pi->priv->handler_id_playing = 0;
	}
	if (pi->priv->handler_id_metadata != 0) {
		g_signal_handler_disconnect (G_OBJECT (pi->priv->bvw), pi->priv->handler_id_metadata);
		pi->priv->handler_id_metadata = 0;
	}

	if (pi->priv->inhibit_cookie != 0) {
		gtk_application_uninhibit (GTK_APPLICATION (pi->priv->totem), pi->priv->inhibit_cookie);
		pi->priv->inhibit_cookie = 0;
	}

	g_object_unref (pi->priv->totem);
	g_object_unref (pi->priv->bvw);
}

