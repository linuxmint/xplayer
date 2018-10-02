/* 
 * Copyright (C) 2006 Bastien Nocera <hadess@hadess.net>
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
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "xplayer-gst-helpers.h"
#include "xplayer-resources.h"
#include "xplayer-mime-types.h"

static gboolean show_mimetype = FALSE;
static gboolean g_fatal_warnings = FALSE;
static char **filenames = NULL;

static void
print_mimetypes (void)
{
	guint i;

	for (i =0; audio_mime_types[i] != NULL; i++) {
		g_print ("%s\n", audio_mime_types[i]);
	}
}

static const GOptionEntry entries[] = {
	{"mimetype", 'm', 0, G_OPTION_ARG_NONE, &show_mimetype, "List the supported mime-types", NULL},
	{"g-fatal-warnings", 0, 0, G_OPTION_ARG_NONE, &g_fatal_warnings, "Make all warnings fatal", NULL},
	{G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, "Audio files to play back", NULL},
	{NULL}
};

static void
setup_play (GstElement *play)
{
	GstElement *audio_sink;
	audio_sink = gst_element_factory_make ("autoaudiosink", "audio-sink");
	g_object_set (play,
		      "audio-sink", audio_sink,
		      "flags", GST_PLAY_FLAG_SOFT_VOLUME | GST_PLAY_FLAG_AUDIO,
		      NULL);
}

static GstBusSyncReply
error_handler (GstBus *bus,
	       GstMessage *message,
	       GstElement *play)
{
	GstMessageType msg_type;

	msg_type = GST_MESSAGE_TYPE (message);
	switch (msg_type) {
	case GST_MESSAGE_ERROR:
		xplayer_gst_message_print (message, play, "xplayer-audio-preview-error");
		exit (1);
	case GST_MESSAGE_EOS:
		exit (0);

	case GST_MESSAGE_ASYNC_DONE:
	case GST_MESSAGE_UNKNOWN:
	case GST_MESSAGE_WARNING:
	case GST_MESSAGE_INFO:
	case GST_MESSAGE_TAG:
	case GST_MESSAGE_BUFFERING:
	case GST_MESSAGE_STATE_CHANGED:
	case GST_MESSAGE_STATE_DIRTY:
	case GST_MESSAGE_STEP_DONE:
	case GST_MESSAGE_CLOCK_PROVIDE:
	case GST_MESSAGE_CLOCK_LOST:
	case GST_MESSAGE_NEW_CLOCK:
	case GST_MESSAGE_STRUCTURE_CHANGE:
	case GST_MESSAGE_STREAM_STATUS:
	case GST_MESSAGE_APPLICATION:
	case GST_MESSAGE_ELEMENT:
	case GST_MESSAGE_SEGMENT_START:
	case GST_MESSAGE_SEGMENT_DONE:
	case GST_MESSAGE_DURATION_CHANGED:
	case GST_MESSAGE_LATENCY:
	case GST_MESSAGE_ASYNC_START:
	case GST_MESSAGE_REQUEST_STATE:
	case GST_MESSAGE_STEP_START:
	case GST_MESSAGE_QOS:
	case GST_MESSAGE_PROGRESS:
	case GST_MESSAGE_TOC:
	case GST_MESSAGE_RESET_TIME:
	case GST_MESSAGE_STREAM_START:
	case GST_MESSAGE_ANY:
#if GST_CHECK_VERSION (1, 1, 3)
	case GST_MESSAGE_NEED_CONTEXT:
	case GST_MESSAGE_HAVE_CONTEXT:
#endif
	default:
		/* Ignored */
		;;
	}

	return GST_BUS_PASS;
}

static void
setup_errors (GstElement *play)
{
	GstBus *bus;

	bus = gst_element_get_bus (play);
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) error_handler, play, NULL);
}

static void
setup_filename (GstElement *play)
{
	if (filenames != NULL) {
		GFile *file;
		char *uri;

		file = g_file_new_for_commandline_arg (filenames[0]);
		uri = g_file_get_uri (file);
		g_object_unref (file);

		g_object_set (play, "uri", uri, NULL);
		g_free (uri);
	} else {
		g_object_set (play, "uri", "fd://0", NULL);
	}
}

int main (int argc, char **argv)
{
	GOptionGroup *options;
	GOptionContext *context;
	GError *error = NULL;
	GMainLoop *loop;
	GstElement *play;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_set_application_name (_("Audio Preview"));
	g_setenv("PULSE_PROP_application.icon_name", "xplayer", TRUE);
	g_setenv("PULSE_PROP_media.role", "music", TRUE);

	context = g_option_context_new ("Plays audio passed on the standard input or the filename passed on the command-line");
	options = gst_init_get_option_group ();
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, options);
	g_type_init ();

	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_print ("couldn't parse command-line options: %s\n", error->message);
		g_error_free (error);
		return 1;
	}

	if (g_fatal_warnings) {
		GLogLevelFlags fatal_mask;

		fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
		fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
		g_log_set_always_fatal (fatal_mask);
	}

	if (show_mimetype == TRUE) {
		print_mimetypes ();
		return 0;
	}

	if (filenames != NULL && g_strv_length (filenames) != 1) {
		char *help;
		help = g_option_context_get_help (context, FALSE, NULL);
		g_print ("%s", help);
		g_free (help);
		return 1;
	}

	play = gst_element_factory_make ("playbin", "play");
	setup_play (play);
	setup_filename (play);
	setup_errors (play);
	xplayer_resources_monitor_start (NULL, -1);
	gst_element_set_state (play, GST_STATE_PLAYING);

	loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (loop);

	return 0;
}

