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
#include <gmodule.h>
#include <string.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>
#include <libpeas/peas-activatable.h>
#include <bacon-video-widget-properties.h>

#include "xplayer-plugin.h"
#include "xplayer.h"
#include "bacon-video-widget.h"

#define XPLAYER_TYPE_MOVIE_PROPERTIES_PLUGIN		(xplayer_movie_properties_plugin_get_type ())
#define XPLAYER_MOVIE_PROPERTIES_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), XPLAYER_TYPE_MOVIE_PROPERTIES_PLUGIN, XplayerMoviePropertiesPlugin))
#define XPLAYER_MOVIE_PROPERTIES_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), XPLAYER_TYPE_MOVIE_PROPERTIES_PLUGIN, XplayerMoviePropertiesPluginClass))
#define XPLAYER_IS_MOVIE_PROPERTIES_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), XPLAYER_TYPE_MOVIE_PROPERTIES_PLUGIN))
#define XPLAYER_IS_MOVIE_PROPERTIES_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), XPLAYER_TYPE_MOVIE_PROPERTIES_PLUGIN))
#define XPLAYER_MOVIE_PROPERTIES_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), XPLAYER_TYPE_MOVIE_PROPERTIES_PLUGIN, XplayerMoviePropertiesPluginClass))

typedef struct {
	GtkWidget    *props;
	guint         handler_id_stream_length;
} XplayerMoviePropertiesPluginPrivate;

XPLAYER_PLUGIN_REGISTER(XPLAYER_TYPE_MOVIE_PROPERTIES_PLUGIN,
		      XplayerMoviePropertiesPlugin,
		      xplayer_movie_properties_plugin)

/* used in update_properties_from_bvw() */
#define UPDATE_FROM_STRING(type, name) \
	do { \
		const char *temp; \
		bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw), \
						 type, &value); \
		if ((temp = g_value_get_string (&value)) != NULL) { \
			bacon_video_widget_properties_set_label (props, name, \
								 temp); \
		} \
		g_value_unset (&value); \
	} while (0)

#define UPDATE_FROM_INT(type, name, format, empty) \
	do { \
		char *temp; \
		bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw), \
						 type, &value); \
		if (g_value_get_int (&value) != 0) \
			temp = g_strdup_printf (gettext (format), \
					g_value_get_int (&value)); \
		else \
			temp = g_strdup (empty); \
		bacon_video_widget_properties_set_label (props, name, temp); \
		g_free (temp); \
		g_value_unset (&value); \
	} while (0)

#define UPDATE_FROM_INT2(type1, type2, name, format) \
	do { \
		int x, y; \
		char *temp; \
		bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw), \
						 type1, &value); \
		x = g_value_get_int (&value); \
		g_value_unset (&value); \
		bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw), \
						 type2, &value); \
		y = g_value_get_int (&value); \
		g_value_unset (&value); \
		temp = g_strdup_printf (gettext (format), x, y); \
		bacon_video_widget_properties_set_label (props, name, temp); \
		g_free (temp); \
	} while (0)

static void
update_properties_from_bvw (BaconVideoWidgetProperties *props,
				      GtkWidget *widget)
{
	GValue value = { 0, };
	gboolean has_video, has_audio;
	BaconVideoWidget *bvw;

	g_return_if_fail (BACON_IS_VIDEO_WIDGET_PROPERTIES (props));
	g_return_if_fail (BACON_IS_VIDEO_WIDGET (widget));

	bvw = BACON_VIDEO_WIDGET (widget);

	/* General */
	UPDATE_FROM_STRING (BVW_INFO_TITLE, "title");
	UPDATE_FROM_STRING (BVW_INFO_ARTIST, "artist");
	UPDATE_FROM_STRING (BVW_INFO_ALBUM, "album");
	UPDATE_FROM_STRING (BVW_INFO_YEAR, "year");
	UPDATE_FROM_STRING (BVW_INFO_COMMENT, "comment");
	UPDATE_FROM_STRING (BVW_INFO_CONTAINER, "container");

	bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw),
					 BVW_INFO_DURATION, &value);
	bacon_video_widget_properties_set_duration (props,
						    g_value_get_int (&value) * 1000);
	g_value_unset (&value);

	/* Types */
	bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw),
					 BVW_INFO_HAS_VIDEO, &value);
	has_video = g_value_get_boolean (&value);
	g_value_unset (&value);

	bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw),
					 BVW_INFO_HAS_AUDIO, &value);
	has_audio = g_value_get_boolean (&value);
	g_value_unset (&value);

	bacon_video_widget_properties_set_has_type (props, has_video, has_audio);

	/* Video */
	if (has_video != FALSE)
	{
		UPDATE_FROM_INT2 (BVW_INFO_DIMENSION_X, BVW_INFO_DIMENSION_Y,
				  "dimensions", N_("%d x %d"));
		UPDATE_FROM_STRING (BVW_INFO_VIDEO_CODEC, "vcodec");
		UPDATE_FROM_INT (BVW_INFO_VIDEO_BITRATE, "video_bitrate",
				 N_("%d kbps"), C_("Stream bit rate", "N/A"));

		bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw), BVW_INFO_FPS, &value);
		bacon_video_widget_properties_set_framerate (props, g_value_get_int (&value));
		g_value_unset (&value);
	}

	/* Audio */
	if (has_audio != FALSE)
	{
		UPDATE_FROM_INT (BVW_INFO_AUDIO_BITRATE, "audio_bitrate",
				 N_("%d kbps"), C_("Stream bit rate", "N/A"));
		UPDATE_FROM_STRING (BVW_INFO_AUDIO_CODEC, "acodec");
		UPDATE_FROM_INT (BVW_INFO_AUDIO_SAMPLE_RATE, "samplerate",
				N_("%d Hz"), C_("Sample rate", "N/A"));
		UPDATE_FROM_STRING (BVW_INFO_AUDIO_CHANNELS, "channels");
	}

#undef UPDATE_FROM_STRING
#undef UPDATE_FROM_INT
#undef UPDATE_FROM_INT2
}



static void
stream_length_notify_cb (XplayerObject *xplayer,
			 GParamSpec *arg1,
			 XplayerMoviePropertiesPlugin *plugin)
{
	gint64 stream_length;

	g_object_get (G_OBJECT (xplayer),
		      "stream-length", &stream_length,
		      NULL);

	bacon_video_widget_properties_set_duration
		(BACON_VIDEO_WIDGET_PROPERTIES (plugin->priv->props),
		 stream_length);
}

static void
xplayer_movie_properties_plugin_file_opened (XplayerObject *xplayer,
					   const char *mrl,
					   XplayerMoviePropertiesPlugin *plugin)
{
	GtkWidget *bvw;

	bvw = xplayer_get_video_widget (xplayer);
	update_properties_from_bvw
		(BACON_VIDEO_WIDGET_PROPERTIES (plugin->priv->props), bvw);
	g_object_unref (bvw);
	gtk_widget_set_sensitive (plugin->priv->props, TRUE);
}

static void
xplayer_movie_properties_plugin_file_closed (XplayerObject *xplayer,
					   XplayerMoviePropertiesPlugin *plugin)
{
        /* Reset the properties and wait for the signal*/
        bacon_video_widget_properties_reset
		(BACON_VIDEO_WIDGET_PROPERTIES (plugin->priv->props));
	gtk_widget_set_sensitive (plugin->priv->props, FALSE);
}

static void
xplayer_movie_properties_plugin_metadata_updated (XplayerObject *xplayer,
						const char *artist, 
						const char *title, 
						const char *album,
						guint track_num,
						XplayerMoviePropertiesPlugin *plugin)
{
	GtkWidget *bvw;

	bvw = xplayer_get_video_widget (xplayer);
	update_properties_from_bvw
		(BACON_VIDEO_WIDGET_PROPERTIES (plugin->priv->props), bvw);
	g_object_unref (bvw);
}

static void
impl_activate (PeasActivatable *plugin)
{
	XplayerMoviePropertiesPlugin *pi;
	XplayerObject *xplayer;

	pi = XPLAYER_MOVIE_PROPERTIES_PLUGIN (plugin);
	xplayer = g_object_get_data (G_OBJECT (plugin), "object");

	pi->priv->props = bacon_video_widget_properties_new ();
	gtk_widget_show (pi->priv->props);
	xplayer_add_sidebar_page (xplayer,
				"properties",
				_("Properties"),
				pi->priv->props);
	gtk_widget_set_sensitive (pi->priv->props, FALSE);

	g_signal_connect (G_OBJECT (xplayer),
			  "file-opened",
			  G_CALLBACK (xplayer_movie_properties_plugin_file_opened),
			  plugin);
	g_signal_connect (G_OBJECT (xplayer),
			  "file-closed",
			  G_CALLBACK (xplayer_movie_properties_plugin_file_closed),
			  plugin);
	g_signal_connect (G_OBJECT (xplayer),
			  "metadata-updated",
			  G_CALLBACK (xplayer_movie_properties_plugin_metadata_updated),
			  plugin);
	pi->priv->handler_id_stream_length = g_signal_connect (G_OBJECT (xplayer),
							 "notify::stream-length",
							 G_CALLBACK (stream_length_notify_cb),
							 plugin);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	XplayerMoviePropertiesPlugin *pi;
	XplayerObject *xplayer;

	pi = XPLAYER_MOVIE_PROPERTIES_PLUGIN (plugin);
	xplayer = g_object_get_data (G_OBJECT (plugin), "object");

	g_signal_handler_disconnect (G_OBJECT (xplayer), pi->priv->handler_id_stream_length);
	g_signal_handlers_disconnect_by_func (G_OBJECT (xplayer),
					      xplayer_movie_properties_plugin_metadata_updated,
					      plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (xplayer),
					      xplayer_movie_properties_plugin_file_opened,
					      plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (xplayer),
					      xplayer_movie_properties_plugin_file_closed,
					      plugin);
	pi->priv->handler_id_stream_length = 0;
	xplayer_remove_sidebar_page (xplayer, "properties");
}

