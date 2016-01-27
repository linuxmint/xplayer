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
 * The Xplayer project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Xplayer. This
 * permission are above and beyond the permissions granted by the GPL license
 * Xplayer is covered by.
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

#include "xplayer-plugin.h"
#include "xplayer.h"
#include "backend/bacon-video-widget.h"

#define XPLAYER_TYPE_SCREENSAVER_PLUGIN		(xplayer_screensaver_plugin_get_type ())
#define XPLAYER_SCREENSAVER_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), XPLAYER_TYPE_SCREENSAVER_PLUGIN, XplayerScreensaverPlugin))
#define XPLAYER_SCREENSAVER_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), XPLAYER_TYPE_SCREENSAVER_PLUGIN, XplayerScreensaverPluginClass))
#define XPLAYER_IS_SCREENSAVER_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), XPLAYER_TYPE_SCREENSAVER_PLUGIN))
#define XPLAYER_IS_SCREENSAVER_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), XPLAYER_TYPE_SCREENSAVER_PLUGIN))
#define XPLAYER_SCREENSAVER_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), XPLAYER_TYPE_SCREENSAVER_PLUGIN, XplayerScreensaverPluginClass))

typedef struct {
	XplayerObject *xplayer;
	BaconVideoWidget *bvw;
	GSettings *settings;

	guint          handler_id_playing;
	guint          handler_id_metadata;
	guint          inhibit_cookie;
} XplayerScreensaverPluginPrivate;

XPLAYER_PLUGIN_REGISTER(XPLAYER_TYPE_SCREENSAVER_PLUGIN,
		      XplayerScreensaverPlugin,
		      xplayer_screensaver_plugin)

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
xplayer_screensaver_update_from_state (XplayerObject *xplayer,
				     XplayerScreensaverPlugin *pi)
{
	gboolean lock_screensaver_on_audio, has_video_frames;
	BaconVideoWidget *bvw;

	bvw = BACON_VIDEO_WIDGET (xplayer_get_video_widget ((Xplayer *)(xplayer)));

	lock_screensaver_on_audio = g_settings_get_boolean (pi->priv->settings, "lock-screensaver-on-audio");
	has_video_frames = has_video (bvw);

	if ((xplayer_is_playing (xplayer) != FALSE && has_video_frames) ||
	    (xplayer_is_playing (xplayer) != FALSE && !lock_screensaver_on_audio)) {
		if (pi->priv->inhibit_cookie == 0) {
			GtkWindow *window;

			window = xplayer_get_main_window (xplayer);
			pi->priv->inhibit_cookie = gtk_application_inhibit (GTK_APPLICATION (xplayer),
										window,
										GTK_APPLICATION_INHIBIT_IDLE,
										_("Playing a movie"));
			g_object_unref (window);
		}
	} else {
		if (pi->priv->inhibit_cookie != 0) {
			gtk_application_uninhibit (GTK_APPLICATION (pi->priv->xplayer), pi->priv->inhibit_cookie);
			pi->priv->inhibit_cookie = 0;
		}
	}
}

static void
property_notify_cb (XplayerObject *xplayer,
		    GParamSpec *spec,
		    XplayerScreensaverPlugin *pi)
{
	xplayer_screensaver_update_from_state (xplayer, pi);
}

static void
got_metadata_cb (BaconVideoWidget *bvw, XplayerScreensaverPlugin *pi)
{
	xplayer_screensaver_update_from_state (pi->priv->xplayer, pi);
}

static void
lock_screensaver_on_audio_changed_cb (GSettings *settings, const gchar *key, XplayerScreensaverPlugin *pi)
{
	xplayer_screensaver_update_from_state (pi->priv->xplayer, pi);
}

static void
impl_activate (PeasActivatable *plugin)
{
	XplayerScreensaverPlugin *pi = XPLAYER_SCREENSAVER_PLUGIN (plugin);
	XplayerObject *xplayer;

	xplayer = g_object_get_data (G_OBJECT (plugin), "object");
	pi->priv->bvw = BACON_VIDEO_WIDGET (xplayer_get_video_widget (xplayer));

	pi->priv->settings = g_settings_new (XPLAYER_GSETTINGS_SCHEMA);
	g_signal_connect (pi->priv->settings, "changed::lock-screensaver-on-audio", (GCallback) lock_screensaver_on_audio_changed_cb, plugin);

	pi->priv->handler_id_playing = g_signal_connect (G_OBJECT (xplayer),
						   "notify::playing",
						   G_CALLBACK (property_notify_cb),
						   pi);
	pi->priv->handler_id_metadata = g_signal_connect (G_OBJECT (pi->priv->bvw),
						    "got-metadata",
						    G_CALLBACK (got_metadata_cb),
						    pi);

	pi->priv->xplayer = g_object_ref (xplayer);

	/* Force setting the current status */
	xplayer_screensaver_update_from_state (xplayer, pi);
}

static void
impl_deactivate	(PeasActivatable *plugin)
{
	XplayerScreensaverPlugin *pi = XPLAYER_SCREENSAVER_PLUGIN (plugin);

	g_object_unref (pi->priv->settings);

	if (pi->priv->handler_id_playing != 0) {
		XplayerObject *xplayer;
		xplayer = g_object_get_data (G_OBJECT (plugin), "object");
		g_signal_handler_disconnect (G_OBJECT (xplayer), pi->priv->handler_id_playing);
		pi->priv->handler_id_playing = 0;
	}
	if (pi->priv->handler_id_metadata != 0) {
		g_signal_handler_disconnect (G_OBJECT (pi->priv->bvw), pi->priv->handler_id_metadata);
		pi->priv->handler_id_metadata = 0;
	}

	if (pi->priv->inhibit_cookie != 0) {
		gtk_application_uninhibit (GTK_APPLICATION (pi->priv->xplayer), pi->priv->inhibit_cookie);
		pi->priv->inhibit_cookie = 0;
	}

	g_object_unref (pi->priv->xplayer);
	g_object_unref (pi->priv->bvw);
}

