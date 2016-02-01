/*
 * Copyright (C) 2003  Andrew Sobala <aes@gnome.org>
 * Copyright (C) 2004  Bastien Nocera <hadess@hadess.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * The Xplayer project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Xplayer. This
 * permission are above and beyond the permissions granted by the GPL license
 * Xplayer is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#define GST_USE_UNSTABLE_API 1
#include <gst/tag/tag.h>
#include <gst/pbutils/pbutils.h>

#include "xplayer-nautilus-properties-view.h"
#include "bacon-video-widget-properties.h"

struct XplayerPropertiesViewPriv {
	GtkWidget *label;
	GtkWidget *vbox;
	BaconVideoWidgetProperties *props;
	GstDiscoverer *disco;
};

static GObjectClass *parent_class = NULL;
static void xplayer_properties_view_finalize (GObject *object);

G_DEFINE_TYPE (XplayerPropertiesView, xplayer_properties_view, GTK_TYPE_GRID)

void
xplayer_properties_view_register_type (GTypeModule *module)
{
	xplayer_properties_view_get_type ();
}

static void
xplayer_properties_view_class_init (XplayerPropertiesViewClass *class)
{
	parent_class = g_type_class_peek_parent (class);
	G_OBJECT_CLASS (class)->finalize = xplayer_properties_view_finalize;
}

static void
update_general (XplayerPropertiesView *props,
		const GstTagList    *list)
{
	struct {
		const char *tag_name;
		const char *widget;
	} items[] = {
		{ GST_TAG_TITLE, "title" },
		{ GST_TAG_ARTIST, "artist" },
		{ GST_TAG_ALBUM, "album" },
	};
	guint i;
        GDate *date;
	GstDateTime *datetime;
	gchar *comment;

	for (i = 0; i < G_N_ELEMENTS(items); i++) {
		char *string;

		if (gst_tag_list_get_string_index (list, items[i].tag_name, 0, &string) != FALSE) {
			bacon_video_widget_properties_set_label (props->priv->props,
								 items[i].widget,
								 string);
			g_free (string);
		}
	}
	
	/* Comment else use Description defined by:
	 * http://xiph.org/vorbis/doc/v-comment.html */
	if (gst_tag_list_get_string (list, GST_TAG_COMMENT, &comment) ||
		gst_tag_list_get_string (list, GST_TAG_DESCRIPTION, &comment)) {

		bacon_video_widget_properties_set_label (props->priv->props,
							 "comment",
							 comment);
		g_free (comment);
        }
	
	/* Date */
        if (gst_tag_list_get_date (list, GST_TAG_DATE, &date)) {
		char *string;

		string = g_strdup_printf ("%d", g_date_get_year (date));
		g_date_free (date);
		bacon_video_widget_properties_set_label (props->priv->props,
							 "year",
							 string);
		g_free (string);
        } else if (gst_tag_list_get_date_time (list, GST_TAG_DATE_TIME, &datetime)) {
		char *string;

		string = g_strdup_printf ("%d", gst_date_time_get_year (datetime));
		gst_date_time_unref (datetime);
		bacon_video_widget_properties_set_label (props->priv->props,
							 "year",
							 string);
		g_free (string);
	}
}

static void
set_codec (XplayerPropertiesView     *props,
	   GstDiscovererStreamInfo *info,
	   const char              *widget)
{
	GstCaps *caps;
	const char *nick;

	nick = gst_discoverer_stream_info_get_stream_type_nick (info);
	if (g_str_equal (nick, "audio") == FALSE &&
	    g_str_equal (nick, "video") == FALSE &&
	    g_str_equal (nick, "container") == FALSE) {
		bacon_video_widget_properties_set_label (props->priv->props,
							 widget,
							 _("N/A"));
		return;
	}

	caps = gst_discoverer_stream_info_get_caps (info);
	if (caps) {
		if (gst_caps_is_fixed (caps)) {
			char *string;

			string = gst_pb_utils_get_codec_description (caps);
			bacon_video_widget_properties_set_label (props->priv->props,
								 widget,
								 string);
			g_free (string);
		}
		gst_caps_unref (caps);
	}
}

static void
set_bitrate (XplayerPropertiesView    *props,
	     guint                   bitrate,
	     const char             *widget)
{
	char *string;

	if (!bitrate) {
		bacon_video_widget_properties_set_label (props->priv->props,
							 widget,
							 C_("Stream bit rate", "N/A"));
		return;
	}
	string = g_strdup_printf (_("%d kbps"), bitrate / 1000);
	bacon_video_widget_properties_set_label (props->priv->props,
						 widget,
						 string);
	g_free (string);
}

static void
update_video (XplayerPropertiesView    *props,
	      GstDiscovererVideoInfo *info)
{
	guint width, height;
	guint fps_n, fps_d;
	char *string;

	width = gst_discoverer_video_info_get_width (info);
	height = gst_discoverer_video_info_get_height (info);
	string = g_strdup_printf (N_("%d x %d"), width, height);
	bacon_video_widget_properties_set_label (props->priv->props,
						 "dimensions",
						 string);
	g_free (string);

	set_codec (props, (GstDiscovererStreamInfo *) info, "vcodec");
	set_bitrate (props, gst_discoverer_video_info_get_bitrate (info), "video_bitrate");

	/* Round up/down to the nearest integer framerate */
	fps_n = gst_discoverer_video_info_get_framerate_num (info);
	fps_d = gst_discoverer_video_info_get_framerate_denom (info);
	if (fps_d == 0)
		bacon_video_widget_properties_set_framerate (props->priv->props, 0);
	else
		bacon_video_widget_properties_set_framerate (props->priv->props,
							     (fps_n + fps_d/2) / fps_d);
}

static void
update_audio (XplayerPropertiesView    *props,
	      GstDiscovererAudioInfo *info)
{
	guint samplerate, channels;

	set_codec (props, (GstDiscovererStreamInfo *) info, "acodec");

	set_bitrate (props, gst_discoverer_audio_info_get_bitrate (info), "audio_bitrate");

	samplerate = gst_discoverer_audio_info_get_sample_rate (info);
	if (samplerate) {
		char *string;
		string = g_strdup_printf (_("%d Hz"), samplerate);
		bacon_video_widget_properties_set_label (props->priv->props,
							 "samplerate",
							 string);
		g_free (string);
	} else {
		bacon_video_widget_properties_set_label (props->priv->props,
							 "samplerate",
							 C_("Sample rate", "N/A"));
	}

	channels = gst_discoverer_audio_info_get_channels (info);
	if (channels) {
		char *string;

		if (channels > 2) {
			string = g_strdup_printf ("%s %d.1", _("Surround"), channels - 1);
		} else if (channels == 1) {
			string = g_strdup (_("Mono"));
		} else if (channels == 2) {
			string = g_strdup (_("Stereo"));
		}
		bacon_video_widget_properties_set_label (props->priv->props,
							 "channels",
							 string);
		g_free (string);
	} else {
		bacon_video_widget_properties_set_label (props->priv->props,
							 "channels",
							 C_("Number of audio channels", "N/A"));
	}
}

static void
discovered_cb (GstDiscoverer       *discoverer,
	       GstDiscovererInfo   *info,
	       GError              *error,
	       XplayerPropertiesView *props)
{
	GList *video_streams, *audio_streams;
	const GstTagList *taglist;
	gboolean has_audio, has_video;
	const char *label;
        GstClockTime duration;
        GstDiscovererStreamInfo *sinfo;

	if (error) {
		g_warning ("Couldn't get information about '%s': %s",
			   gst_discoverer_info_get_uri (info),
			   error->message);
		return;
	}

	video_streams = gst_discoverer_info_get_video_streams (info);
	has_video = (video_streams != NULL);
	audio_streams = gst_discoverer_info_get_audio_streams (info);
	has_audio = (audio_streams != NULL);

	if (has_audio == has_video)
		label = N_("Audio/Video");
	else if (has_audio)
		label = N_("Audio");
	else
		label = N_("Video");

	gtk_label_set_text (GTK_LABEL (props->priv->label), _(label));

	/* Widgets */
	bacon_video_widget_properties_set_has_type (props->priv->props,
						    has_video,
						    has_audio);

	/* General */
        duration = gst_discoverer_info_get_duration (info);
        bacon_video_widget_properties_set_duration (props->priv->props, duration / GST_SECOND * 1000);

        sinfo = gst_discoverer_info_get_stream_info (info);
        if (sinfo) {
		set_codec (props, sinfo, "container");
		gst_discoverer_stream_info_unref (sinfo);
	}

	taglist = gst_discoverer_info_get_tags (info);
	update_general (props, taglist);

	/* Video and Audio */
	if (video_streams)
		update_video (props, video_streams->data);
	if (audio_streams)
		update_audio (props, audio_streams->data);

	gst_discoverer_stream_info_list_free (video_streams);
	gst_discoverer_stream_info_list_free (audio_streams);
}

static void
xplayer_properties_view_init (XplayerPropertiesView *props)
{
	GError *err = NULL;

	props->priv = g_new0 (XplayerPropertiesViewPriv, 1);

	props->priv->vbox = bacon_video_widget_properties_new ();
	gtk_grid_attach (GTK_GRID (props), props->priv->vbox, 0, 0, 1, 1);
	gtk_widget_show (GTK_WIDGET (props));

	props->priv->props = BACON_VIDEO_WIDGET_PROPERTIES (props->priv->vbox);

	props->priv->disco = gst_discoverer_new (GST_SECOND * 60, &err);
	if (props->priv->disco == NULL) {
		g_warning ("Could not create discoverer object: %s", err->message);
		g_error_free (err);
		return;
	}
	g_signal_connect (props->priv->disco, "discovered",
			  G_CALLBACK (discovered_cb), props);
}

static void
xplayer_properties_view_finalize (GObject *object)
{
	XplayerPropertiesView *props;

	props = XPLAYER_PROPERTIES_VIEW (object);

	if (props->priv != NULL)
	{
		if (props->priv->disco != NULL) {
			g_object_unref (G_OBJECT (props->priv->disco));
			props->priv->disco = NULL;
		}
		if (props->priv->label != NULL) {
			g_object_unref (G_OBJECT (props->priv->label));
			props->priv->label = NULL;
		}
		g_free (props->priv);
	}
	props->priv = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
xplayer_properties_view_new (const char *location, GtkWidget *label)
{
	XplayerPropertiesView *self;

	self = g_object_new (XPLAYER_TYPE_PROPERTIES_VIEW, NULL);
	g_object_ref (label);
	self->priv->label = label;
	xplayer_properties_view_set_location (self, location);

	return GTK_WIDGET (self);
}

void
xplayer_properties_view_set_location (XplayerPropertiesView *props,
				    const char          *location)
{
	g_assert (XPLAYER_IS_PROPERTIES_VIEW (props));

	if (props->priv->disco)
		gst_discoverer_stop (props->priv->disco);

	bacon_video_widget_properties_reset (props->priv->props);

	if (location != NULL && props->priv->disco != NULL) {
		gst_discoverer_start (props->priv->disco);

		if (gst_discoverer_discover_uri_async (props->priv->disco, location) == FALSE) {
			g_warning ("Couldn't add %s to list", location);
			return;
		}
	}
}

