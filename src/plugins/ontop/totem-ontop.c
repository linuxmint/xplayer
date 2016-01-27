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

#include "xplayer-plugin.h"
#include "xplayer.h"
#include "backend/bacon-video-widget.h"

#define XPLAYER_TYPE_ONTOP_PLUGIN		(xplayer_ontop_plugin_get_type ())
#define XPLAYER_ONTOP_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), XPLAYER_TYPE_ONTOP_PLUGIN, XplayerOntopPlugin))
#define XPLAYER_ONTOP_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), XPLAYER_TYPE_ONTOP_PLUGIN, XplayerOntopPluginClass))
#define XPLAYER_IS_ONTOP_PLUGIN(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), XPLAYER_TYPE_ONTOP_PLUGIN))
#define XPLAYER_IS_ONTOP_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), XPLAYER_TYPE_ONTOP_PLUGIN))
#define XPLAYER_ONTOP_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), XPLAYER_TYPE_ONTOP_PLUGIN, XplayerOntopPluginClass))

typedef struct
{
	guint handler_id;
	guint handler_id_metadata;
	GtkWindow *window;
	BaconVideoWidget *bvw;
	XplayerObject *xplayer;
} XplayerOntopPluginPrivate;

XPLAYER_PLUGIN_REGISTER(XPLAYER_TYPE_ONTOP_PLUGIN, XplayerOntopPlugin, xplayer_ontop_plugin)

static void
update_from_state (XplayerOntopPluginPrivate *priv)
{
	GValue has_video = { 0, };

	bacon_video_widget_get_metadata (priv->bvw, BVW_INFO_HAS_VIDEO, &has_video);

	gtk_window_set_keep_above (priv->window,
				   (xplayer_is_playing (priv->xplayer) != FALSE &&
				    g_value_get_boolean (&has_video) != FALSE));
	g_value_unset (&has_video);
}

static void
got_metadata_cb (BaconVideoWidget *bvw, XplayerOntopPlugin *pi)
{
	update_from_state (pi->priv);
}

static void
property_notify_cb (XplayerObject *xplayer,
		    GParamSpec *spec,
		    XplayerOntopPlugin *pi)
{
	update_from_state (pi->priv);
}

static void
impl_activate (PeasActivatable *plugin)
{
	XplayerOntopPlugin *pi = XPLAYER_ONTOP_PLUGIN (plugin);

	pi->priv->xplayer = g_object_get_data (G_OBJECT (plugin), "object");
	pi->priv->window = xplayer_get_main_window (pi->priv->xplayer);
	pi->priv->bvw = BACON_VIDEO_WIDGET (xplayer_get_video_widget (pi->priv->xplayer));

	pi->priv->handler_id = g_signal_connect (G_OBJECT (pi->priv->xplayer),
					   "notify::playing",
					   G_CALLBACK (property_notify_cb),
					   pi);
	pi->priv->handler_id_metadata = g_signal_connect (G_OBJECT (pi->priv->bvw),
						    "got-metadata",
						    G_CALLBACK (got_metadata_cb),
						    pi);

	update_from_state (pi->priv);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	XplayerOntopPlugin *pi = XPLAYER_ONTOP_PLUGIN (plugin);

	g_signal_handler_disconnect (G_OBJECT (pi->priv->xplayer), pi->priv->handler_id);
	g_signal_handler_disconnect (G_OBJECT (pi->priv->bvw), pi->priv->handler_id_metadata);

	g_object_unref (pi->priv->bvw);

	/* We can't really "restore" the previous state, as there's
	 * no way to find the old state */
	gtk_window_set_keep_above (pi->priv->window, FALSE);
	g_object_unref (pi->priv->window);
}

