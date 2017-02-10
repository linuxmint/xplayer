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
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

/**
 * SECTION:xplayer-object
 * @short_description: main Xplayer object
 * @stability: Unstable
 * @include: xplayer.h
 *
 * #XplayerObject is the core object of Xplayer; a singleton which controls all Xplayer's main functions.
 **/

#include "config.h"

#include <glib-object.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <gio/gio.h>

#include <gst/tag/tag.h>
#include <string.h>

#include "xplayer.h"
#include "xplayer-private.h"
#include "xplayer-options.h"
#include "xplayer-plugins-engine.h"
#include "xplayer-playlist.h"
#include "bacon-video-widget.h"
#include "xplayer-statusbar.h"
#include "xplayer-time-label.h"
#include "xplayer-time-helpers.h"
#include "xplayer-sidebar.h"
#include "xplayer-menu.h"
#include "xplayer-uri.h"
#include "xplayer-interface.h"
#include "video-utils.h"
#include "xplayer-dnd-menu.h"
#include "xplayer-preferences.h"
#include "xplayer-rtl-helpers.h"

#include "xplayer-mime-types.h"
#include "xplayer-uri-schemes.h"

#define REWIND_OR_PREVIOUS 4000

#define SEEK_FORWARD_SHORT_OFFSET 15
#define SEEK_BACKWARD_SHORT_OFFSET -5

#define SEEK_FORWARD_LONG_OFFSET 10*60
#define SEEK_BACKWARD_LONG_OFFSET -3*60

#define DEFAULT_WINDOW_W 650
#define DEFAULT_WINDOW_H 500

#define VOLUME_EPSILON (1e-10)

/* casts are to shut gcc up */
static const GtkTargetEntry target_table[] = {
	{ (gchar*) "text/uri-list", 0, 0 },
	{ (gchar*) "_NETSCAPE_URL", 0, 1 }
};

static gboolean xplayer_action_open_files_list (XplayerObject *xplayer, GSList *list);
static void update_buttons (XplayerObject *xplayer);
static void update_fill (XplayerObject *xplayer, gdouble level);
static void update_media_menu_items (XplayerObject *xplayer);
static void playlist_changed_cb (GtkWidget *playlist, XplayerObject *xplayer);
static void play_pause_set_label (XplayerObject *xplayer, XplayerStates state);

/* Callback functions for GtkBuilder */
G_MODULE_EXPORT gboolean main_window_destroy_cb (GtkWidget *widget, GdkEvent *event, XplayerObject *xplayer);
G_MODULE_EXPORT gboolean window_state_event_cb (GtkWidget *window, GdkEventWindowState *event, XplayerObject *xplayer);
G_MODULE_EXPORT gboolean seek_slider_pressed_cb (GtkWidget *widget, GdkEventButton *event, XplayerObject *xplayer);
G_MODULE_EXPORT void seek_slider_changed_cb (GtkAdjustment *adj, XplayerObject *xplayer);
G_MODULE_EXPORT gboolean seek_slider_released_cb (GtkWidget *widget, GdkEventButton *event, XplayerObject *xplayer);
G_MODULE_EXPORT void volume_button_value_changed_cb (GtkScaleButton *button, gdouble value, XplayerObject *xplayer);
G_MODULE_EXPORT gboolean window_key_press_event_cb (GtkWidget *win, GdkEventKey *event, XplayerObject *xplayer);
G_MODULE_EXPORT int window_scroll_event_cb (GtkWidget *win, GdkEvent *event, XplayerObject *xplayer);
G_MODULE_EXPORT void main_pane_size_allocated (GtkWidget *main_pane, GtkAllocation *allocation, XplayerObject *xplayer);
G_MODULE_EXPORT void fs_exit1_activate_cb (GtkButton *button, XplayerObject *xplayer);
G_MODULE_EXPORT void fs_blank1_activate_cb (GtkToggleButton *button, XplayerObject *xplayer);


enum {
	PROP_0,
	PROP_FULLSCREEN,
	PROP_PLAYING,
	PROP_STREAM_LENGTH,
	PROP_SEEKABLE,
	PROP_CURRENT_TIME,
	PROP_CURRENT_MRL,
	PROP_CURRENT_CONTENT_TYPE,
	PROP_CURRENT_DISPLAY_NAME,
	PROP_REMEMBER_POSITION
};

enum {
	FILE_OPENED,
	FILE_CLOSED,
	FILE_HAS_PLAYED,
	METADATA_UPDATED,
	GET_USER_AGENT,
	GET_TEXT_SUBTITLE,
	LAST_SIGNAL
};

static void xplayer_object_set_property		(GObject *object,
						 guint property_id,
						 const GValue *value,
						 GParamSpec *pspec);
static void xplayer_object_get_property		(GObject *object,
						 guint property_id,
						 GValue *value,
						 GParamSpec *pspec);
static void xplayer_object_finalize (GObject *xplayer);

static int xplayer_table_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(XplayerObject, xplayer_object, GTK_TYPE_APPLICATION)

static gboolean
xplayer_object_local_command_line (GApplication              *application,
				 gchar                   ***arguments,
				 int                       *exit_status)
{
	GOptionContext *context;
	GError *error = NULL;
	char **argv;
	int argc;

	/* Dupe so that the remote arguments are listed, but
	 * not removed from the list */
	argv = g_strdupv (*arguments);
	argc = g_strv_length (argv);

	context = xplayer_options_get_context ();
	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_print (_("%s\nRun '%s --help' to see a full list of available command line options.\n"),
				error->message, argv[0]);
		g_error_free (error);
	        *exit_status = 1;
	        goto bail;
	}

	/* Replace relative paths with absolute URIs */
	if (optionstate.filenames != NULL) {
		guint n_files;
		int i, n_args;

		n_args = g_strv_length (*arguments);
		n_files = g_strv_length (optionstate.filenames);

		i = n_args - n_files;
		for ( ; i < n_args; i++) {
			char *new_path;

			new_path = xplayer_create_full_path ((*arguments)[i]);
			if (new_path == NULL)
				continue;

			g_free ((*arguments)[i]);
			(*arguments)[i] = new_path;
		}
	}

	g_strfreev (optionstate.filenames);
	optionstate.filenames = NULL;

	*exit_status = 0;
bail:
	g_option_context_free (context);
	g_strfreev (argv);

	return FALSE;
}

static gboolean
accumulator_first_non_null_wins (GSignalInvocationHint *ihint,
				 GValue *return_accu,
				 const GValue *handler_return,
				 gpointer data)
{
	const gchar *str;

	str = g_value_get_string (handler_return);
	if (str == NULL)
		return TRUE;
	g_value_set_string (return_accu, str);

	return FALSE;
}

static void
xplayer_object_class_init (XplayerObjectClass *klass)
{
	GObjectClass *object_class;
	GApplicationClass *app_class;

	object_class = (GObjectClass *) klass;
	app_class = (GApplicationClass *) klass;

	object_class->set_property = xplayer_object_set_property;
	object_class->get_property = xplayer_object_get_property;
	object_class->finalize = xplayer_object_finalize;

	app_class->local_command_line = xplayer_object_local_command_line;

	/**
	 * XplayerObject:fullscreen:
	 *
	 * If %TRUE, Xplayer is in fullscreen mode.
	 **/
	g_object_class_install_property (object_class, PROP_FULLSCREEN,
					 g_param_spec_boolean ("fullscreen", "Fullscreen?", "Whether Xplayer is in fullscreen mode.",
							       FALSE, G_PARAM_READABLE));

	/**
	 * XplayerObject:playing:
	 *
	 * If %TRUE, Xplayer is playing an audio or video file.
	 **/
	g_object_class_install_property (object_class, PROP_PLAYING,
					 g_param_spec_boolean ("playing", "Playing?", "Whether Xplayer is currently playing a file.",
							       FALSE, G_PARAM_READABLE));

	/**
	 * XplayerObject:stream-length:
	 *
	 * The length of the current stream, in milliseconds.
	 **/
	g_object_class_install_property (object_class, PROP_STREAM_LENGTH,
					 g_param_spec_int64 ("stream-length", "Stream length", "The length of the current stream.",
							     G_MININT64, G_MAXINT64, 0,
							     G_PARAM_READABLE));

	/**
	 * XplayerObject:current-time:
	 *
	 * The player's position (time) in the current stream, in milliseconds.
	 **/
	g_object_class_install_property (object_class, PROP_CURRENT_TIME,
					 g_param_spec_int64 ("current-time", "Current time", "The player's position (time) in the current stream.",
							     G_MININT64, G_MAXINT64, 0,
							     G_PARAM_READABLE));

	/**
	 * XplayerObject:seekable:
	 *
	 * If %TRUE, the current stream is seekable.
	 **/
	g_object_class_install_property (object_class, PROP_SEEKABLE,
					 g_param_spec_boolean ("seekable", "Seekable?", "Whether the current stream is seekable.",
							       FALSE, G_PARAM_READABLE));

	/**
	 * XplayerObject:current-mrl:
	 *
	 * The MRL of the current stream.
	 **/
	g_object_class_install_property (object_class, PROP_CURRENT_MRL,
					 g_param_spec_string ("current-mrl", "Current MRL", "The MRL of the current stream.",
							      NULL, G_PARAM_READABLE));

	/**
	 * XplayerObject:current-content-type:
	 *
	 * The content-type of the current stream.
	 **/
	g_object_class_install_property (object_class, PROP_CURRENT_CONTENT_TYPE,
					 g_param_spec_string ("current-content-type",
							      "Current stream's content-type",
							      "Current stream's content-type.",
							      NULL, G_PARAM_READABLE));

	/**
	 * XplayerObject:current-display-name:
	 *
	 * The display name of the current stream.
	 **/
	g_object_class_install_property (object_class, PROP_CURRENT_DISPLAY_NAME,
					 g_param_spec_string ("current-display-name",
							      "Current stream's display name",
							      "Current stream's display name.",
							      NULL, G_PARAM_READABLE));

	/**
	 * XplayerObject:remember-position:
	 *
	 * If %TRUE, Xplayer will remember the position it was at last time a given file was opened.
	 **/
	g_object_class_install_property (object_class, PROP_REMEMBER_POSITION,
					 g_param_spec_boolean ("remember-position", "Remember position?",
					                       "Whether to remember the position each video was at last time.",
							       FALSE, G_PARAM_READWRITE));

	/**
	 * XplayerObject::file-opened:
	 * @xplayer: the #XplayerObject which received the signal
	 * @mrl: the MRL of the opened stream
	 *
	 * The #XplayerObject::file-opened signal is emitted when a new stream is opened by Xplayer.
	 */
	xplayer_table_signals[FILE_OPENED] =
		g_signal_new ("file-opened",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (XplayerObjectClass, file_opened),
				NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1, G_TYPE_STRING);

	/**
	 * XplayerObject::file-has-played:
	 * @xplayer: the #XplayerObject which received the signal
	 * @mrl: the MRL of the opened stream
	 *
	 * The #XplayerObject::file-has-played signal is emitted when a new stream has started playing in Xplayer.
	 */
	xplayer_table_signals[FILE_HAS_PLAYED] =
		g_signal_new ("file-has-played",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (XplayerObjectClass, file_has_played),
				NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1, G_TYPE_STRING);

	/**
	 * XplayerObject::file-closed:
	 * @xplayer: the #XplayerObject which received the signal
	 *
	 * The #XplayerObject::file-closed signal is emitted when Xplayer closes a stream.
	 */
	xplayer_table_signals[FILE_CLOSED] =
		g_signal_new ("file-closed",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (XplayerObjectClass, file_closed),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0, G_TYPE_NONE);

	/**
	 * XplayerObject::metadata-updated:
	 * @xplayer: the #XplayerObject which received the signal
	 * @artist: the name of the artist, or %NULL
	 * @title: the stream title, or %NULL
	 * @album: the name of the stream's album, or %NULL
	 * @track_number: the stream's track number
	 *
	 * The #XplayerObject::metadata-updated signal is emitted when the metadata of a stream is updated, typically
	 * when it's being loaded.
	 */
	xplayer_table_signals[METADATA_UPDATED] =
		g_signal_new ("metadata-updated",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (XplayerObjectClass, metadata_updated),
				NULL, NULL,
	                        g_cclosure_marshal_generic,
				G_TYPE_NONE, 4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT);

	/**
	 * XplayerObject::get-user-agent:
	 * @xplayer: the #XplayerObject which received the signal
	 * @mrl: the MRL of the opened stream
	 *
	 * The #XplayerObject::get-user-agent signal is emitted before opening a stream, so that plugins
	 * have the opportunity to return the user-agent to be set.
	 *
	 * Return value: allocated string representing the user-agent to use for @mrl
	 */
	xplayer_table_signals[GET_USER_AGENT] =
		g_signal_new ("get-user-agent",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XplayerObjectClass, get_user_agent),
			      accumulator_first_non_null_wins, NULL,
	                      g_cclosure_marshal_generic,
			      G_TYPE_STRING, 1, G_TYPE_STRING);

	/**
	 * XplayerObject::get-text-subtitle:
	 * @xplayer: the #XplayerObject which received the signal
	 * @mrl: the MRL of the opened stream
	 *
	 * The #XplayerObject::get-text-subtitle signal is emitted before opening a stream, so that plugins
	 * have the opportunity to detect or download text subtitles for the stream if necessary.
	 *
	 * Return value: allocated string representing the URI of the subtitle to use for @mrl
	 */
	xplayer_table_signals[GET_TEXT_SUBTITLE] =
		g_signal_new ("get-text-subtitle",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XplayerObjectClass, get_text_subtitle),
			      accumulator_first_non_null_wins, NULL,
	                      g_cclosure_marshal_generic,
			      G_TYPE_STRING, 1, G_TYPE_STRING);
}

static void
xplayer_object_init (XplayerObject *xplayer)
{
	//FIXME nothing yet
}

static void
xplayer_object_finalize (GObject *object)
{

	G_OBJECT_CLASS (xplayer_object_parent_class)->finalize (object);
}

static void
xplayer_object_set_property (GObject *object,
			   guint property_id,
			   const GValue *value,
			   GParamSpec *pspec)
{
	XplayerObject *xplayer = XPLAYER_OBJECT (object);

	switch (property_id) {
		case PROP_REMEMBER_POSITION:
			xplayer->remember_position = g_value_get_boolean (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
xplayer_object_get_property (GObject *object,
			   guint property_id,
			   GValue *value,
			   GParamSpec *pspec)
{
	XplayerObject *xplayer;

	xplayer = XPLAYER_OBJECT (object);

	switch (property_id)
	{
	case PROP_FULLSCREEN:
		g_value_set_boolean (value, xplayer_is_fullscreen (xplayer));
		break;
	case PROP_PLAYING:
		g_value_set_boolean (value, xplayer_is_playing (xplayer));
		break;
	case PROP_STREAM_LENGTH:
		g_value_set_int64 (value, bacon_video_widget_get_stream_length (xplayer->bvw));
		break;
	case PROP_CURRENT_TIME:
		g_value_set_int64 (value, bacon_video_widget_get_current_time (xplayer->bvw));
		break;
	case PROP_SEEKABLE:
		g_value_set_boolean (value, xplayer_is_seekable (xplayer));
		break;
	case PROP_CURRENT_MRL:
		g_value_set_string (value, xplayer->mrl);
		break;
	case PROP_CURRENT_CONTENT_TYPE:
		g_value_take_string (value, xplayer_playlist_get_current_content_type (xplayer->playlist));
		break;
	case PROP_CURRENT_DISPLAY_NAME:
		g_value_take_string (value, xplayer_playlist_get_current_title (xplayer->playlist));
		break;
	case PROP_REMEMBER_POSITION:
		g_value_set_boolean (value, xplayer->remember_position);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

/**
 * xplayer_object_plugins_init:
 * @xplayer: a #XplayerObject
 *
 * Initialises the plugin engine and activates all the
 * enabled plugins.
 **/
void
xplayer_object_plugins_init (XplayerObject *xplayer)
{
	if (xplayer->engine == NULL)
		xplayer->engine = xplayer_plugins_engine_get_default (xplayer);
}

/**
 * xplayer_object_plugins_shutdown:
 * @xplayer: a #XplayerObject
 *
 * Shuts down the plugin engine and deactivates all the
 * plugins.
 **/
void
xplayer_object_plugins_shutdown (XplayerObject *xplayer)
{
	g_clear_object (&xplayer->engine);
}

/**
 * xplayer_object_get_main_window:
 * @xplayer: a #XplayerObject
 *
 * Gets Xplayer's main window and increments its reference count.
 *
 * Return value: (transfer full): Xplayer's main window
 **/
GtkWindow *
xplayer_object_get_main_window (XplayerObject *xplayer)
{
	g_return_val_if_fail (XPLAYER_IS_OBJECT (xplayer), NULL);

	g_object_ref (G_OBJECT (xplayer->win));

	return GTK_WINDOW (xplayer->win);
}

/**
 * xplayer_object_get_ui_manager:
 * @xplayer: a #XplayerObject
 *
 * Gets Xplayer's UI manager, but does not change its reference count.
 *
 * Return value: (transfer none): Xplayer's UI manager
 **/
GtkUIManager *
xplayer_object_get_ui_manager (XplayerObject *xplayer)
{
	g_return_val_if_fail (XPLAYER_IS_OBJECT (xplayer), NULL);

	return xplayer->ui_manager;
}

/**
 * xplayer_object_get_video_widget:
 * @xplayer: a #XplayerObject
 *
 * Gets Xplayer's video widget and increments its reference count.
 *
 * Return value: (transfer full): Xplayer's video widget
 **/
GtkWidget *
xplayer_object_get_video_widget (XplayerObject *xplayer)
{
	g_return_val_if_fail (XPLAYER_IS_OBJECT (xplayer), NULL);

	g_object_ref (G_OBJECT (xplayer->bvw));

	return GTK_WIDGET (xplayer->bvw);
}

/**
 * xplayer_object_get_version:
 *
 * Gets the application name and version (e.g. "Xplayer 2.28.0").
 *
 * Return value: a newly-allocated string of the name and version of the application
 **/
char *
xplayer_object_get_version (void)
{
	/* Translators: %s is the xplayer version number */
	return g_strdup_printf (_("Xplayer %s"), VERSION);
}

/**
 * xplayer_get_current_time:
 * @xplayer: a #XplayerObject
 *
 * Gets the current position's time in the stream as a gint64.
 *
 * Return value: the current position in the stream
 **/
gint64
xplayer_get_current_time (XplayerObject *xplayer)
{
	g_return_val_if_fail (XPLAYER_IS_OBJECT (xplayer), 0);

	return bacon_video_widget_get_current_time (xplayer->bvw);
}

typedef struct {
	XplayerObject *xplayer;
	gchar *uri;
	gchar *display_name;
} AddToPlaylistData;

static void
add_to_playlist_and_play_cb (XplayerPlaylist *playlist, GAsyncResult *async_result, AddToPlaylistData *data)
{
	int end;
	gboolean playlist_changed;

	playlist_changed = xplayer_playlist_add_mrl_finish (playlist, async_result);

	end = xplayer_playlist_get_last (playlist);

	xplayer_signal_unblock_by_data (playlist, data->xplayer);

	if (playlist_changed && end != -1) {
		char *mrl, *subtitle;

		subtitle = NULL;
		xplayer_playlist_set_current (playlist, end);
		mrl = xplayer_playlist_get_current_mrl (playlist, &subtitle);
		xplayer_action_set_mrl_and_play (data->xplayer, mrl, subtitle);
		g_free (mrl);
		g_free (subtitle);
	}

	/* Free the closure data */
	g_object_unref (data->xplayer);
	g_free (data->uri);
	g_free (data->display_name);
	g_slice_free (AddToPlaylistData, data);
}

/**
 * xplayer_object_add_to_playlist_and_play:
 * @xplayer: a #XplayerObject
 * @uri: the URI to add to the playlist
 * @display_name: the display name of the URI
 *
 * Add @uri to the playlist and play it immediately.
 **/
void
xplayer_object_add_to_playlist_and_play (XplayerObject *xplayer,
				const char *uri,
				const char *display_name)
{
	AddToPlaylistData *data;

	/* Block all signals from the playlist until we're finished. They're unblocked in the callback, add_to_playlist_and_play_cb.
	 * There are no concurrency issues here, since blocking the signals multiple times should require them to be unblocked the
	 * same number of times before they fire again. */
	xplayer_signal_block_by_data (xplayer->playlist, xplayer);

	data = g_slice_new (AddToPlaylistData);
	data->xplayer = g_object_ref (xplayer);
	data->uri = g_strdup (uri);
	data->display_name = g_strdup (display_name);

	xplayer_playlist_add_mrl (xplayer->playlist, uri, display_name, TRUE,
	                        NULL, (GAsyncReadyCallback) add_to_playlist_and_play_cb, data);
}

/**
 * xplayer_object_get_current_mrl:
 * @xplayer: a #XplayerObject
 *
 * Get the MRL of the current stream, or %NULL if nothing's playing.
 * Free with g_free().
 *
 * Return value: a newly-allocated string containing the MRL of the current stream
 **/
char *
xplayer_object_get_current_mrl (XplayerObject *xplayer)
{
	return xplayer_playlist_get_current_mrl (xplayer->playlist, NULL);
}

/**
 * xplayer_object_get_playlist_length:
 * @xplayer: a #XplayerObject
 *
 * Returns the length of the current playlist.
 *
 * Return value: the playlist length
 **/
guint
xplayer_object_get_playlist_length (XplayerObject *xplayer)
{
	int last;

	last = xplayer_playlist_get_last (xplayer->playlist);
	if (last == -1)
		return 0;
	return last + 1;
}

/**
 * xplayer_object_get_playlist_pos:
 * @xplayer: a #XplayerObject
 *
 * Returns the <code class="literal">0</code>-based index of the current entry in the playlist. If
 * there is no current entry in the playlist, <code class="literal">-1</code> is returned.
 *
 * Return value: the index of the current playlist entry, or <code class="literal">-1</code>
 **/
int
xplayer_object_get_playlist_pos (XplayerObject *xplayer)
{
	return xplayer_playlist_get_current (xplayer->playlist);
}

/**
 * xplayer_object_get_title_at_playlist_pos:
 * @xplayer: a #XplayerObject
 * @playlist_index: the <code class="literal">0</code>-based entry index
 *
 * Gets the title of the playlist entry at @index.
 *
 * Return value: the entry title at @index, or %NULL; free with g_free()
 **/
char *
xplayer_object_get_title_at_playlist_pos (XplayerObject *xplayer, guint playlist_index)
{
	return xplayer_playlist_get_title (xplayer->playlist, playlist_index);
}

/**
 * xplayer_get_short_title:
 * @xplayer: a #XplayerObject
 *
 * Gets the title of the current entry in the playlist.
 *
 * Return value: the current entry's title, or %NULL; free with g_free()
 **/
char *
xplayer_get_short_title (XplayerObject *xplayer)
{
	return xplayer_playlist_get_current_title (xplayer->playlist);
}

/**
 * xplayer_object_set_current_subtitle:
 * @xplayer: a #XplayerObject
 * @subtitle_uri: the URI of the subtitle file to add
 *
 * Add the @subtitle_uri subtitle file to the playlist, setting it as the subtitle for the current
 * playlist entry.
 **/
void
xplayer_object_set_current_subtitle (XplayerObject *xplayer, const char *subtitle_uri)
{
	xplayer_playlist_set_current_subtitle (xplayer->playlist, subtitle_uri);
}

/**
 * xplayer_object_add_sidebar_page:
 * @xplayer: a #XplayerObject
 * @page_id: a string used to identify the page
 * @title: the page's title
 * @main_widget: the main widget for the page
 *
 * Adds a sidebar page to Xplayer's sidebar with the given @page_id.
 * @main_widget is added into the page and shown automatically, while
 * @title is displayed as the page's title in the tab bar.
 **/
void
xplayer_object_add_sidebar_page (XplayerObject *xplayer,
			       const char *page_id,
			       const char *title,
			       GtkWidget *main_widget)
{
	xplayer_sidebar_add_page (xplayer,
				page_id,
				title,
				NULL,
				main_widget);
}

/**
 * xplayer_object_remove_sidebar_page:
 * @xplayer: a #XplayerObject
 * @page_id: a string used to identify the page
 *
 * Removes the page identified by @page_id from Xplayer's sidebar.
 * If @page_id doesn't exist in the sidebar, this function does
 * nothing.
 **/
void
xplayer_object_remove_sidebar_page (XplayerObject *xplayer,
			   const char *page_id)
{
	xplayer_sidebar_remove_page (xplayer, page_id);
}

/**
 * xplayer_file_opened:
 * @xplayer: a #XplayerObject
 * @mrl: the MRL opened
 *
 * Emits the #XplayerObject::file-opened signal on @xplayer, with the
 * specified @mrl.
 **/
void
xplayer_file_opened (XplayerObject *xplayer,
		   const char *mrl)
{
	g_signal_emit (G_OBJECT (xplayer),
		       xplayer_table_signals[FILE_OPENED],
		       0, mrl);
}

/**
 * xplayer_file_closed:
 * @xplayer: a #XplayerObject
 *
 * Emits the #XplayerObject::file-closed signal on @xplayer.
 **/
void
xplayer_file_closed (XplayerObject *xplayer)
{
	g_signal_emit (G_OBJECT (xplayer),
		       xplayer_table_signals[FILE_CLOSED],
		       0);

}

/**
 * xplayer_file_has_played:
 * @xplayer: a #XplayerObject
 *
 * Emits the #XplayerObject::file-played signal on @xplayer.
 **/
void
xplayer_file_has_played (XplayerObject *xplayer,
		       const char  *mrl)
{
	g_signal_emit (G_OBJECT (xplayer),
		       xplayer_table_signals[FILE_HAS_PLAYED],
		       0, mrl);
}

/**
 * xplayer_metadata_updated:
 * @xplayer: a #XplayerObject
 * @artist: the stream's artist, or %NULL
 * @title: the stream's title, or %NULL
 * @album: the stream's album, or %NULL
 * @track_num: the track number of the stream
 *
 * Emits the #XplayerObject::metadata-updated signal on @xplayer,
 * with the specified stream data.
 **/
void
xplayer_metadata_updated (XplayerObject *xplayer,
			const char *artist,
			const char *title,
			const char *album,
			guint track_num)
{
	g_signal_emit (G_OBJECT (xplayer),
		       xplayer_table_signals[METADATA_UPDATED],
		       0,
		       artist,
		       title,
		       album,
		       track_num);
}

GQuark
xplayer_remote_command_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("xplayer_remote_command");

	return quark;
}

/* This should really be standard. */
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
xplayer_remote_command_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_UNKNOWN, "unknown"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_PLAY, "play"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_PAUSE, "pause"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_STOP, "stop"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_PLAYPAUSE, "play-pause"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_NEXT, "next"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_PREVIOUS, "previous"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_SEEK_FORWARD, "seek-forward"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_SEEK_BACKWARD, "seek-backward"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_VOLUME_UP, "volume-up"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_VOLUME_DOWN, "volume-down"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_FULLSCREEN, "fullscreen"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_QUIT, "quit"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_ENQUEUE, "enqueue"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_REPLACE, "replace"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_SHOW, "show"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_TOGGLE_CONTROLS, "toggle-controls"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_UP, "up"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_DOWN, "down"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_LEFT, "left"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_RIGHT, "right"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_SELECT, "select"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_DVD_MENU, "dvd-menu"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_ZOOM_UP, "zoom-up"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_ZOOM_DOWN, "zoom-down"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_EJECT, "eject"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_PLAY_DVD, "play-dvd"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_MUTE, "mute"),
			ENUM_ENTRY (XPLAYER_REMOTE_COMMAND_TOGGLE_ASPECT, "toggle-aspect-ratio"),
			{ 0, NULL, NULL }
		};

		etype = g_enum_register_static ("XplayerRemoteCommand", values);
	}

	return etype;
}

GQuark
xplayer_remote_setting_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("xplayer_remote_setting");

	return quark;
}

GType
xplayer_remote_setting_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (XPLAYER_REMOTE_SETTING_SHUFFLE, "shuffle"),
			ENUM_ENTRY (XPLAYER_REMOTE_SETTING_REPEAT, "repeat"),
			{ 0, NULL, NULL }
		};

		etype = g_enum_register_static ("XplayerRemoteSetting", values);
	}

	return etype;
}

static void
reset_seek_status (XplayerObject *xplayer)
{
	/* Release the lock and reset everything so that we
	 * avoid being "stuck" seeking on errors */

	if (xplayer->seek_lock != FALSE) {
		xplayer_statusbar_set_seeking (XPLAYER_STATUSBAR (xplayer->statusbar), FALSE);
		xplayer_time_label_set_seeking (XPLAYER_TIME_LABEL (xplayer->fs->time_label), FALSE);
		xplayer->seek_lock = FALSE;
		bacon_video_widget_seek (xplayer->bvw, 0, NULL);
		xplayer_action_stop (xplayer);
	}
}

/**
 * xplayer_object_action_error:
 * @xplayer: a #XplayerObject
 * @title: the error dialog title
 * @reason: the error dialog text
 *
 * Displays a non-blocking error dialog with the
 * given @title and @reason.
 **/
void
xplayer_object_action_error (XplayerObject *xplayer, const char *title, const char *reason)
{
	reset_seek_status (xplayer);
	xplayer_interface_error (title, reason,
			GTK_WINDOW (xplayer->win));
}

G_GNUC_NORETURN void
xplayer_action_error_and_exit (const char *title,
		const char *reason, XplayerObject *xplayer)
{
	reset_seek_status (xplayer);
	xplayer_interface_error_blocking (title, reason,
			GTK_WINDOW (xplayer->win));
	xplayer_action_exit (xplayer);
}

static void
xplayer_action_save_size (XplayerObject *xplayer)
{
	GtkPaned *item;

	if (xplayer->bvw == NULL)
		return;

	if (xplayer_is_fullscreen (xplayer) != FALSE)
		return;

	/* Save the size of the video widget */
	item = GTK_PANED (gtk_builder_get_object (xplayer->xml, "tmw_main_pane"));
	gtk_window_get_size (GTK_WINDOW (xplayer->win), &xplayer->window_w,
			&xplayer->window_h);
	xplayer->sidebar_w = xplayer->window_w
		- gtk_paned_get_position (item);
}

static void
xplayer_action_save_state (XplayerObject *xplayer, const char *page_id)
{
	GKeyFile *keyfile;
	char *contents, *filename;

	if (xplayer->win == NULL)
		return;
	if (xplayer->window_w == 0
	    || xplayer->window_h == 0)
		return;

	keyfile = g_key_file_new ();
	g_key_file_set_integer (keyfile, "State",
				"window_w", xplayer->window_w);
	g_key_file_set_integer (keyfile, "State",
			"window_h", xplayer->window_h);
	g_key_file_set_boolean (keyfile, "State",
			"show_sidebar", xplayer_sidebar_is_visible (xplayer));
	g_key_file_set_boolean (keyfile, "State",
			"maximised", xplayer->maximised);
	g_key_file_set_integer (keyfile, "State",
			"sidebar_w", xplayer->sidebar_w);

	g_key_file_set_string (keyfile, "State",
			"sidebar_page", page_id);

	contents = g_key_file_to_data (keyfile, NULL, NULL);
	g_key_file_free (keyfile);
	filename = g_build_filename (xplayer_dot_dir (), "state.ini", NULL);
	g_file_set_contents (filename, contents, -1, NULL);

	g_free (filename);
	g_free (contents);
}

G_GNUC_NORETURN static void
xplayer_action_wait_force_exit (gpointer user_data)
{
	g_usleep (10 * G_USEC_PER_SEC);
	exit (1);
}

/**
 * xplayer_object_action_exit:
 * @xplayer: a #XplayerObject
 *
 * Closes Xplayer.
 **/
void
xplayer_object_action_exit (XplayerObject *xplayer)
{
	GdkDisplay *display = NULL;
	char *page_id;

	/* Save the page ID before we close the plugins, otherwise
	 * we'll never save it properly */
	page_id = xplayer_sidebar_get_current_page (xplayer);

	/* Shut down the plugins first, allowing them to display modal dialogues (etc.) without threat of being killed from another thread */
	if (xplayer != NULL && xplayer->engine != NULL)
		xplayer_object_plugins_shutdown (xplayer);

	/* Exit forcefully if we can't do the shutdown in 10 seconds */
	g_thread_new ("force-exit", (GThreadFunc) xplayer_action_wait_force_exit, NULL);

	if (gtk_main_level () > 0)
		gtk_main_quit ();

	if (xplayer == NULL)
		exit (0);

	if (xplayer->bvw)
		xplayer_action_save_size (xplayer);

	if (xplayer->win != NULL) {
		gtk_widget_hide (xplayer->win);
		display = gtk_widget_get_display (xplayer->win);
	}

	if (xplayer->prefs != NULL)
		gtk_widget_hide (xplayer->prefs);

	if (display != NULL)
		gdk_display_sync (display);

	if (xplayer->bvw) {
		xplayer_save_position (xplayer);
		bacon_video_widget_close (xplayer->bvw);
	}

	xplayer_action_save_state (xplayer, page_id);
	g_free (page_id);

	xplayer_sublang_exit (xplayer);
	xplayer_destroy_file_filters ();

	g_clear_object (&xplayer->settings);
	g_clear_object (&xplayer->fs);

	if (xplayer->win)
		gtk_widget_destroy (GTK_WIDGET (xplayer->win));

	g_object_unref (xplayer);

	exit (0);
}

static void
xplayer_action_menu_popup (XplayerObject *xplayer, guint button)
{
	GtkWidget *menu;

	menu = gtk_ui_manager_get_widget (xplayer->ui_manager,
			"/xplayer-main-popup");
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
			button, gtk_get_current_event_time ());
	gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), FALSE);
}

G_GNUC_NORETURN gboolean
main_window_destroy_cb (GtkWidget *widget, GdkEvent *event, XplayerObject *xplayer)
{
	xplayer_action_exit (xplayer);
}

static void
play_pause_set_label (XplayerObject *xplayer, XplayerStates state)
{
	GtkAction *action;
	const char *id, *tip;
	GSList *l, *proxies;

	if (state == xplayer->state)
		return;

	switch (state)
	{
	case STATE_PLAYING:
		xplayer_statusbar_set_text (XPLAYER_STATUSBAR (xplayer->statusbar),
				_("Playing"));
		id = "media-playback-pause-symbolic";
		tip = N_("Pause");
		xplayer_playlist_set_playing (xplayer->playlist, XPLAYER_PLAYLIST_STATUS_PLAYING);
		break;
	case STATE_PAUSED:
		xplayer_statusbar_set_text (XPLAYER_STATUSBAR (xplayer->statusbar),
				_("Paused"));
		id = xplayer_get_rtl_icon_name ("media-playback-start");
		tip = N_("Play");
		xplayer_playlist_set_playing (xplayer->playlist, XPLAYER_PLAYLIST_STATUS_PAUSED);
		break;
	case STATE_STOPPED:
		xplayer_statusbar_set_text (XPLAYER_STATUSBAR (xplayer->statusbar),
				_("Stopped"));
		xplayer_statusbar_set_time_and_length
			(XPLAYER_STATUSBAR (xplayer->statusbar), 0, 0);
		id = xplayer_get_rtl_icon_name ("media-playback-start");
		xplayer_playlist_set_playing (xplayer->playlist, XPLAYER_PLAYLIST_STATUS_NONE);
		tip = N_("Play");
		break;
	default:
		g_assert_not_reached ();
		return;
	}

	action = gtk_action_group_get_action (xplayer->main_action_group, "play");
	g_object_set (G_OBJECT (action),
			"tooltip", _(tip),
			"icon-name", id, NULL);

	proxies = gtk_action_get_proxies (action);
	for (l = proxies; l != NULL; l = l->next) {
		atk_object_set_name (gtk_widget_get_accessible (l->data),
				_(tip));
	}

	xplayer->state = state;

	g_object_notify (G_OBJECT (xplayer), "playing");
}

void
xplayer_action_eject (XplayerObject *xplayer)
{
	GMount *mount;

	mount = xplayer_get_mount_for_media (xplayer->mrl);
	if (mount == NULL)
		return;

	g_free (xplayer->mrl);
	xplayer->mrl = NULL;
	bacon_video_widget_close (xplayer->bvw);
	xplayer_file_closed (xplayer);
	xplayer->has_played_emitted = FALSE;

	/* The volume monitoring will take care of removing the items */
	g_mount_eject_with_operation (mount, G_MOUNT_UNMOUNT_NONE, NULL, NULL, NULL, NULL);
	g_object_unref (mount);
}

void
xplayer_action_show_properties (XplayerObject *xplayer)
{
	if (xplayer_is_fullscreen (xplayer) == FALSE)
		xplayer_sidebar_set_current_page (xplayer, "properties", TRUE);
}

/**
 * xplayer_object_action_play:
 * @xplayer: a #XplayerObject
 *
 * Plays the current stream. If Xplayer is already playing, it continues
 * to play. If the stream cannot be played, and error dialog is displayed.
 **/
void
xplayer_object_action_play (XplayerObject *xplayer)
{
	GError *err = NULL;
	int retval;
	char *msg, *disp;

	if (xplayer->mrl == NULL)
		return;

	if (bacon_video_widget_is_playing (xplayer->bvw) != FALSE)
		return;

	retval = bacon_video_widget_play (xplayer->bvw,  &err);
	play_pause_set_label (xplayer, retval ? STATE_PLAYING : STATE_STOPPED);

	if (retval != FALSE) {
		if (xplayer->has_played_emitted == FALSE) {
			xplayer_file_has_played (xplayer, xplayer->mrl);
			xplayer->has_played_emitted = TRUE;
		}
		return;
	}

	disp = xplayer_uri_escape_for_display (xplayer->mrl);
	msg = g_strdup_printf(_("Xplayer could not play '%s'."), disp);
	g_free (disp);

	xplayer_action_error (xplayer, msg, err->message);
	xplayer_action_stop (xplayer);
	g_free (msg);
	g_error_free (err);
}

static void
xplayer_action_seek (XplayerObject *xplayer, double pos)
{
	GError *err = NULL;
	int retval;

	if (xplayer->mrl == NULL)
		return;
	if (bacon_video_widget_is_seekable (xplayer->bvw) == FALSE)
		return;

	retval = bacon_video_widget_seek (xplayer->bvw, pos, &err);

	if (retval == FALSE)
	{
		char *msg, *disp;

		disp = xplayer_uri_escape_for_display (xplayer->mrl);
		msg = g_strdup_printf(_("Xplayer could not play '%s'."), disp);
		g_free (disp);

		reset_seek_status (xplayer);

		xplayer_action_error (xplayer, msg, err->message);
		g_free (msg);
		g_error_free (err);
	}
}

/**
 * xplayer_action_set_mrl_and_play:
 * @xplayer: a #XplayerObject
 * @mrl: the MRL to play
 * @subtitle: a subtitle file to load, or %NULL
 *
 * Loads the specified @mrl and plays it, if possible.
 * Calls xplayer_action_set_mrl() then xplayer_action_play().
 * For more information, see the documentation for xplayer_action_set_mrl_with_warning().
 **/
void
xplayer_action_set_mrl_and_play (XplayerObject *xplayer, const char *mrl, const char *subtitle)
{
	if (xplayer_action_set_mrl (xplayer, mrl, subtitle) != FALSE)
		xplayer_action_play (xplayer);
}

static gboolean
xplayer_action_open_dialog (XplayerObject *xplayer, const char *path, gboolean play)
{
	GSList *filenames;
	gboolean playlist_modified;

	filenames = xplayer_add_files (GTK_WINDOW (xplayer->win), path);

	if (filenames == NULL)
		return FALSE;

	playlist_modified = xplayer_action_open_files_list (xplayer,
			filenames);

	if (playlist_modified == FALSE) {
		g_slist_foreach (filenames, (GFunc) g_free, NULL);
		g_slist_free (filenames);
		return FALSE;
	}

	g_slist_foreach (filenames, (GFunc) g_free, NULL);
	g_slist_free (filenames);

	if (play != FALSE) {
		char *mrl, *subtitle;

		mrl = xplayer_playlist_get_current_mrl (xplayer->playlist, &subtitle);
		xplayer_action_set_mrl_and_play (xplayer, mrl, subtitle);
		g_free (mrl);
		g_free (subtitle);
	}

	return TRUE;
}

/**
 * xplayer_object_action_stop:
 * @xplayer: a #XplayerObject
 *
 * Stops the current stream.
 **/
void
xplayer_object_action_stop (XplayerObject *xplayer)
{
	bacon_video_widget_stop (xplayer->bvw);
	play_pause_set_label (xplayer, STATE_STOPPED);
}

static
char * get_language_name (char * language_code)
{
	const char * language;
	char * upper_language;

	language = gst_tag_get_language_name (language_code);

	if (language)
	{
		upper_language = g_strdup_printf("%c%s", toupper (language[0]), language+1);
		return upper_language;
	}
	else if (language_code)
	{
		return g_strdup (language_code);
	}
	else
	{
		return g_strdup(_("Unknown"));
	}
}

/**
 * xplayer_object_action_cycle_language:
 * @xplayer: a #XplayerObject
 *
 * Switch to the next available audio track.
 **/
void
xplayer_object_action_cycle_language (XplayerObject *xplayer)
{
	int current_track, new_track;
	char * track_name;
	GList *list, *track;

	if (xplayer->mrl == NULL)
		return;

	current_track = bacon_video_widget_get_language (xplayer->bvw);
	list = bacon_video_widget_get_languages (xplayer->bvw);
	new_track = current_track + 1;

	if (new_track < 0)
	{
		new_track = 0;
	}

	if (new_track >= g_list_length (list))
	{
		new_track = 0;;
	}

	track = g_list_nth (list, new_track);
	bacon_video_widget_set_language (xplayer->bvw, new_track);
	track_name = get_language_name (track->data);

	// Show track name
	bacon_video_widget_show_osd (xplayer->bvw, "preferences-desktop-locale-symbolic", track_name);
	g_free (track_name);

	// Refresh the menus
	xplayer_languages_update (xplayer, list);
}

/**
 * xplayer_object_action_cycle_subtitle:
 * @xplayer: a #XplayerObject
 *
 * Switch to the next available subtitle track.
 **/
void
xplayer_object_action_cycle_subtitle (XplayerObject *xplayer)
{
	int current_track, new_track;
	char * track_name;
	GList *list, *track;

	if (xplayer->mrl == NULL)
		return;

	current_track = bacon_video_widget_get_subtitle (xplayer->bvw);
	list = bacon_video_widget_get_subtitles (xplayer->bvw);
	new_track = current_track + 1;

	if (new_track < 0)
	{
		new_track = 0;
	}

	if (new_track >= g_list_length (list))
	{
		bacon_video_widget_set_subtitle (xplayer->bvw, -1);
		track_name = g_strdup(_("None"));
	}
	else {
		track = g_list_nth (list, new_track);
		bacon_video_widget_set_subtitle (xplayer->bvw, new_track);
		track_name = get_language_name (track->data);
	}

	// Show track name
	bacon_video_widget_show_osd (xplayer->bvw, "media-view-subtitles-symbolic", track_name);
	g_free (track_name);

	// Refresh the menus
	xplayer_subtitles_update (xplayer, list);
}

/**
 * xplayer_object_action_play_pause:
 * @xplayer: a #XplayerObject
 *
 * Gets the current MRL from the playlist and attempts to play it.
 * If the stream is already playing, playback is paused.
 **/
void
xplayer_object_action_play_pause (XplayerObject *xplayer)
{
	if (xplayer->mrl == NULL) {
		char *mrl, *subtitle;

		/* Try to pull an mrl from the playlist */
		mrl = xplayer_playlist_get_current_mrl (xplayer->playlist, &subtitle);
		if (mrl == NULL) {
			play_pause_set_label (xplayer, STATE_STOPPED);
			return;
		} else {
			xplayer_action_set_mrl_and_play (xplayer, mrl, subtitle);
			g_free (mrl);
			g_free (subtitle);
			return;
		}
	}

	if (bacon_video_widget_is_playing (xplayer->bvw) == FALSE) {
		if (bacon_video_widget_play (xplayer->bvw, NULL) != FALSE &&
		    xplayer->has_played_emitted == FALSE) {
			xplayer_file_has_played (xplayer, xplayer->mrl);
			xplayer->has_played_emitted = TRUE;
		}
		play_pause_set_label (xplayer, STATE_PLAYING);
	} else {
		bacon_video_widget_pause (xplayer->bvw);
		play_pause_set_label (xplayer, STATE_PAUSED);

		/* Save the stream position */
		xplayer_save_position (xplayer);
	}
}

/**
 * xplayer_action_pause:
 * @xplayer: a #XplayerObject
 *
 * Pauses the current stream. If Xplayer is already paused, it continues
 * to be paused.
 **/
void
xplayer_action_pause (XplayerObject *xplayer)
{
	if (bacon_video_widget_is_playing (xplayer->bvw) != FALSE) {
		bacon_video_widget_pause (xplayer->bvw);
		play_pause_set_label (xplayer, STATE_PAUSED);

		/* Save the stream position */
		xplayer_save_position (xplayer);
	}
}

gboolean
window_state_event_cb (GtkWidget *window, GdkEventWindowState *event,
		       XplayerObject *xplayer)
{
	if (event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) {
		xplayer->maximised = (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
		xplayer_action_set_sensitivity ("zoom-1-2", !xplayer->maximised);
		xplayer_action_set_sensitivity ("zoom-1-1", !xplayer->maximised);
		xplayer_action_set_sensitivity ("zoom-2-1", !xplayer->maximised);
		return FALSE;
	}

	if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) == 0)
		return FALSE;

	if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) {
		if (xplayer->controls_visibility != XPLAYER_CONTROLS_UNDEFINED)
			xplayer_action_save_size (xplayer);
		xplayer_fullscreen_set_fullscreen (xplayer->fs, TRUE);

		xplayer->controls_visibility = XPLAYER_CONTROLS_FULLSCREEN;
		show_controls (xplayer, FALSE);
		xplayer_action_set_sensitivity ("fullscreen", FALSE);
	} else {
		GtkAction *action;

		xplayer_fullscreen_set_fullscreen (xplayer->fs, FALSE);

		action = gtk_action_group_get_action (xplayer->main_action_group,
				"show-controls");

		if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
			xplayer->controls_visibility = XPLAYER_CONTROLS_VISIBLE;
		else
			xplayer->controls_visibility = XPLAYER_CONTROLS_HIDDEN;

		show_controls (xplayer, TRUE);
		xplayer_action_set_sensitivity ("fullscreen", TRUE);
	}

	g_object_notify (G_OBJECT (xplayer), "fullscreen");

	return FALSE;
}

/**
 * xplayer_object_action_fullscreen_toggle:
 * @xplayer: a #XplayerObject
 *
 * Toggles Xplayer's fullscreen state; if Xplayer is fullscreened, calling
 * this makes it unfullscreened and vice-versa.
 **/
void
xplayer_object_action_fullscreen_toggle (XplayerObject *xplayer)
{
	if (xplayer_is_fullscreen (xplayer) != FALSE)
		gtk_window_unfullscreen (GTK_WINDOW (xplayer->win));
	else
		gtk_window_fullscreen (GTK_WINDOW (xplayer->win));
}

/**
 * xplayer_action_fullscreen:
 * @xplayer: a #XplayerObject
 * @state: %TRUE if Xplayer should be fullscreened
 *
 * Sets Xplayer's fullscreen state according to @state.
 **/
void
xplayer_action_fullscreen (XplayerObject *xplayer, gboolean state)
{
	if (xplayer_is_fullscreen (xplayer) == state)
		return;

	xplayer_action_fullscreen_toggle (xplayer);
}

void
xplayer_action_blank (XplayerObject *xplayer)
{
	xplayer_fullscreen_toggle_blank_monitors(xplayer->fs, GTK_WINDOW (xplayer->win));
}

void
fs_exit1_activate_cb (GtkButton *button, XplayerObject *xplayer)
{
	xplayer_action_fullscreen (xplayer, FALSE);
}

void
fs_blank1_activate_cb (GtkToggleButton *button, XplayerObject *xplayer)
{
	xplayer_action_blank (xplayer);
}

void
xplayer_action_open (XplayerObject *xplayer)
{
	xplayer_action_open_dialog (xplayer, NULL, TRUE);
}

static void
xplayer_open_location_response_cb (GtkDialog *dialog, gint response, XplayerObject *xplayer)
{
	char *uri;

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (xplayer->open_location));
		return;
	}

	gtk_widget_hide (GTK_WIDGET (dialog));

	/* Open the specified URI */
	uri = xplayer_open_location_get_uri (xplayer->open_location);

	if (uri != NULL)
	{
		char *mrl, *subtitle;
		const char *filenames[2];

		filenames[0] = uri;
		filenames[1] = NULL;
		xplayer_action_open_files (xplayer, (char **) filenames);

		mrl = xplayer_playlist_get_current_mrl (xplayer->playlist, &subtitle);
		xplayer_action_set_mrl_and_play (xplayer, mrl, subtitle);
		g_free (mrl);
		g_free (subtitle);
	}
 	g_free (uri);

	gtk_widget_destroy (GTK_WIDGET (xplayer->open_location));
}

void
xplayer_action_open_location (XplayerObject *xplayer)
{
	if (xplayer->open_location != NULL) {
		gtk_window_present (GTK_WINDOW (xplayer->open_location));
		return;
	}

	xplayer->open_location = XPLAYER_OPEN_LOCATION (xplayer_open_location_new ());

	g_signal_connect (G_OBJECT (xplayer->open_location), "delete-event",
			G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect (G_OBJECT (xplayer->open_location), "response",
			G_CALLBACK (xplayer_open_location_response_cb), xplayer);
	g_object_add_weak_pointer (G_OBJECT (xplayer->open_location), (gpointer *)&(xplayer->open_location));

	gtk_window_set_transient_for (GTK_WINDOW (xplayer->open_location),
			GTK_WINDOW (xplayer->win));
	gtk_widget_show (GTK_WIDGET (xplayer->open_location));
}

static char *
xplayer_get_nice_name_for_stream (XplayerObject *xplayer)
{
	GValue title_value = { 0, };
	GValue album_value = { 0, };
	GValue artist_value = { 0, };
	GValue value = { 0, };
	char *retval;
	int tracknum;

	bacon_video_widget_get_metadata (xplayer->bvw, BVW_INFO_TITLE, &title_value);
	bacon_video_widget_get_metadata (xplayer->bvw, BVW_INFO_ARTIST, &artist_value);
	bacon_video_widget_get_metadata (xplayer->bvw, BVW_INFO_ALBUM, &album_value);
	bacon_video_widget_get_metadata (xplayer->bvw,
					 BVW_INFO_TRACK_NUMBER,
					 &value);

	tracknum = g_value_get_int (&value);
	g_value_unset (&value);

	if (g_value_get_string (&title_value) != NULL) {
		xplayer_metadata_updated (xplayer,
			g_value_get_string (&artist_value),
			g_value_get_string (&title_value),
			g_value_get_string (&album_value),
			tracknum);
	}
	else {
		xplayer_metadata_updated (xplayer,
			_("Media Player"),
			xplayer_get_short_title(xplayer),
			"Xplayer",
			0);
	}

	if (g_value_get_string (&title_value) == NULL) {
		retval = NULL;
		goto bail;
	}
	if (g_value_get_string (&artist_value) == NULL) {
		retval = g_value_dup_string (&title_value);
		goto bail;
	}

	if (tracknum != 0) {
		retval = g_strdup_printf ("%02d. %s - %s",
					  tracknum,
					  g_value_get_string (&artist_value),
					  g_value_get_string (&title_value));
	} else {
		retval = g_strdup_printf ("%s - %s",
					  g_value_get_string (&artist_value),
					  g_value_get_string (&title_value));
	}

bail:
	g_value_unset (&album_value);
	g_value_unset (&artist_value);
	g_value_unset (&title_value);

	return retval;
}

static void
update_mrl_label (XplayerObject *xplayer, const char *name)
{
	if (name != NULL)
	{
		/* Update the mrl label */
		xplayer_fullscreen_set_title (xplayer->fs, name);

		/* Title */
		gtk_window_set_title (GTK_WINDOW (xplayer->win), name);
	} else {
		xplayer_statusbar_set_time_and_length (XPLAYER_STATUSBAR
				(xplayer->statusbar), 0, 0);
		xplayer_statusbar_set_text (XPLAYER_STATUSBAR (xplayer->statusbar),
				_("Stopped"));

		g_object_notify (G_OBJECT (xplayer), "stream-length");

		/* Update the mrl label */
		xplayer_fullscreen_set_title (xplayer->fs, NULL);

		/* Title */
		gtk_window_set_title (GTK_WINDOW (xplayer->win), _("Media Player"));
	}
}

/**
 * xplayer_action_set_mrl_with_warning:
 * @xplayer: a #XplayerObject
 * @mrl: the MRL to play
 * @subtitle: a subtitle file to load, or %NULL
 * @warn: %TRUE if error dialogs should be displayed
 *
 * Loads the specified @mrl and optionally the specified subtitle
 * file. If @subtitle is %NULL Xplayer will attempt to auto-locate
 * any subtitle files for @mrl.
 *
 * If a stream is already playing, it will be stopped and closed.
 *
 * If any errors are encountered, error dialogs will only be displayed
 * if @warn is %TRUE.
 *
 * Return value: %TRUE on success
 **/
gboolean
xplayer_action_set_mrl_with_warning (XplayerObject *xplayer,
				   const char *mrl,
				   const char *subtitle,
				   gboolean warn)
{
	gboolean retval = TRUE;

	if (xplayer->mrl != NULL) {
		xplayer->seek_to = 0;
		xplayer->seek_to_start = 0;

		xplayer_save_position (xplayer);
		g_free (xplayer->mrl);
		xplayer->mrl = NULL;
		bacon_video_widget_close (xplayer->bvw);
		xplayer_file_closed (xplayer);
		xplayer->has_played_emitted = FALSE;
		play_pause_set_label (xplayer, STATE_STOPPED);
		update_fill (xplayer, -1.0);
	}

	if (mrl == NULL) {
		retval = FALSE;

		play_pause_set_label (xplayer, STATE_STOPPED);

		/* Play/Pause */
		xplayer_action_set_sensitivity ("play", FALSE);

		/* Volume */
		xplayer_main_set_sensitivity ("tmw_volume_button", FALSE);
		xplayer_action_set_sensitivity ("volume-up", FALSE);
		xplayer_action_set_sensitivity ("volume-down", FALSE);
		xplayer->volume_sensitive = FALSE;

		/* Control popup */
		xplayer_fullscreen_set_can_set_volume (xplayer->fs, FALSE);
		xplayer_fullscreen_set_seekable (xplayer->fs, FALSE);
		xplayer_action_set_sensitivity ("next-chapter", FALSE);
		xplayer_action_set_sensitivity ("previous-chapter", FALSE);

		/* Clear the playlist */
		xplayer_action_set_sensitivity ("clear-playlist", FALSE);

		/* Subtitle selection */
		xplayer_action_set_sensitivity ("select-subtitle", FALSE);

		/* Fullscreen */
		xplayer_action_set_sensitivity ("fullscreen", FALSE);

		/* Set the logo */
		bacon_video_widget_set_logo_mode (xplayer->bvw, TRUE);
		update_mrl_label (xplayer, NULL);

		/* Unset the drag */
		gtk_drag_source_unset (GTK_WIDGET (xplayer->bvw));

		g_object_notify (G_OBJECT (xplayer), "playing");
	} else {
		gboolean caps;
		gdouble volume;
		char *user_agent;
		char *autoload_sub;
		GdkWindowState window_state;
		/* cast is to shut gcc up */
		const GtkTargetEntry source_table[] = {
			{ (gchar*) "text/uri-list", 0, 0 }
		};

		bacon_video_widget_set_logo_mode (xplayer->bvw, FALSE);

		autoload_sub = NULL;
		if (subtitle == NULL)
			g_signal_emit (G_OBJECT (xplayer), xplayer_table_signals[GET_TEXT_SUBTITLE], 0, mrl, &autoload_sub);

		user_agent = NULL;
		g_signal_emit (G_OBJECT (xplayer), xplayer_table_signals[GET_USER_AGENT], 0, mrl, &user_agent);
		bacon_video_widget_set_user_agent (xplayer->bvw, user_agent);
		g_free (user_agent);

		xplayer_gdk_window_set_waiting_cursor (gtk_widget_get_window (xplayer->win));
		xplayer_try_restore_position (xplayer, mrl);
		bacon_video_widget_open (xplayer->bvw, mrl);
		bacon_video_widget_set_text_subtitle (xplayer->bvw, subtitle ? subtitle : autoload_sub);
		g_free (autoload_sub);
		gdk_window_set_cursor (gtk_widget_get_window (xplayer->win), NULL);
		xplayer->mrl = g_strdup (mrl);

		/* Play/Pause */
		xplayer_action_set_sensitivity ("play", TRUE);

		/* Volume */
		caps = bacon_video_widget_can_set_volume (xplayer->bvw);
		xplayer_main_set_sensitivity ("tmw_volume_button", caps);
		xplayer_fullscreen_set_can_set_volume (xplayer->fs, caps);
		volume = bacon_video_widget_get_volume (xplayer->bvw);
		xplayer_action_set_sensitivity ("volume-up", caps && volume < (1.0 - VOLUME_EPSILON));
		xplayer_action_set_sensitivity ("volume-down", caps && volume > VOLUME_EPSILON);
		xplayer->volume_sensitive = caps;

		/* Clear the playlist */
		xplayer_action_set_sensitivity ("clear-playlist", TRUE);

		/* Subtitle selection */
		xplayer_action_set_sensitivity ("select-subtitle", !xplayer_is_special_mrl (mrl));

		/* Fullscreen */
		window_state = gdk_window_get_state (gtk_widget_get_window (xplayer->win));
		xplayer_action_set_sensitivity ("fullscreen", !(window_state & GDK_WINDOW_STATE_FULLSCREEN));

		/* Set the playlist */
		play_pause_set_label (xplayer, STATE_PAUSED);

		xplayer_file_opened (xplayer, xplayer->mrl);

		/* Set the drag source */
		gtk_drag_source_set (GTK_WIDGET (xplayer->bvw),
				     GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
				     source_table, G_N_ELEMENTS (source_table),
				     GDK_ACTION_COPY);

		/* Hide subtitles if user did not select the respective option in preferences */
		if (g_settings_get_boolean(xplayer->settings, "autodisplay-subtitles") == FALSE)
			/* Must be called with -1 (no subtitles) to remove the GST_PLAY_FLAG_TEXT from the flags, which is responsible for the subtitle display */
			bacon_video_widget_set_subtitle(xplayer->bvw, -1);
	}
	update_buttons (xplayer);
	update_media_menu_items (xplayer);

	return retval;
}

/**
 * xplayer_action_set_mrl:
 * @xplayer: a #XplayerObject
 * @mrl: the MRL to load
 * @subtitle: a subtitle file to load, or %NULL
 *
 * Calls xplayer_action_set_mrl_with_warning() with warnings enabled.
 * For more information, see the documentation for xplayer_action_set_mrl_with_warning().
 *
 * Return value: %TRUE on success
 **/
gboolean
xplayer_action_set_mrl (XplayerObject *xplayer, const char *mrl, const char *subtitle)
{
	return xplayer_action_set_mrl_with_warning (xplayer, mrl, subtitle, TRUE);
}

static gboolean
xplayer_time_within_seconds (XplayerObject *xplayer)
{
	gint64 _time;

	_time = bacon_video_widget_get_current_time (xplayer->bvw);

	return (_time < REWIND_OR_PREVIOUS);
}

static void
xplayer_action_direction (XplayerObject *xplayer, XplayerPlaylistDirection dir)
{
	if (bacon_video_widget_has_next_track (xplayer->bvw) == FALSE &&
	    xplayer_playlist_has_direction (xplayer->playlist, dir) == FALSE &&
	    xplayer_playlist_get_repeat (xplayer->playlist) == FALSE)
		return;

	if (bacon_video_widget_has_next_track (xplayer->bvw) != FALSE) {
		BvwDVDEvent event;
		event = (dir == XPLAYER_PLAYLIST_DIRECTION_NEXT ? BVW_DVD_NEXT_CHAPTER : BVW_DVD_PREV_CHAPTER);
		bacon_video_widget_dvd_event (xplayer->bvw, event);
		return;
	}

	if (dir == XPLAYER_PLAYLIST_DIRECTION_NEXT ||
	    bacon_video_widget_is_seekable (xplayer->bvw) == FALSE ||
	    xplayer_time_within_seconds (xplayer) != FALSE) {
		char *mrl, *subtitle;

		xplayer_playlist_set_direction (xplayer->playlist, dir);
		mrl = xplayer_playlist_get_current_mrl (xplayer->playlist, &subtitle);
		xplayer_action_set_mrl_and_play (xplayer, mrl, subtitle);

		g_free (subtitle);
		g_free (mrl);
	} else {
		xplayer_action_seek (xplayer, 0);
	}
}

/**
 * xplayer_object_action_previous:
 * @xplayer: a #XplayerObject
 *
 * If a DVD is being played, goes to the previous chapter. If a normal stream
 * is being played, goes to the start of the stream if possible. If seeking is
 * not possible, plays the previous entry in the playlist.
 **/
void
xplayer_object_action_previous (XplayerObject *xplayer)
{
	xplayer_action_direction (xplayer, XPLAYER_PLAYLIST_DIRECTION_PREVIOUS);
}

/**
 * xplayer_object_action_next:
 * @xplayer: a #XplayerObject
 *
 * If a DVD is being played, goes to the next chapter. If a normal stream
 * is being played, plays the next entry in the playlist.
 **/
void
xplayer_object_action_next (XplayerObject *xplayer)
{
	xplayer_action_direction (xplayer, XPLAYER_PLAYLIST_DIRECTION_NEXT);
}

static void
xplayer_seek_time_rel (XplayerObject *xplayer, gint64 _time, gboolean relative, gboolean accurate)
{
	GError *err = NULL;
	gint64 sec;

	if (xplayer->mrl == NULL)
		return;
	if (bacon_video_widget_is_seekable (xplayer->bvw) == FALSE)
		return;

	xplayer_statusbar_set_seeking (XPLAYER_STATUSBAR (xplayer->statusbar), TRUE);
	xplayer_time_label_set_seeking (XPLAYER_TIME_LABEL (xplayer->fs->time_label), TRUE);

	if (relative != FALSE) {
		gint64 oldmsec;
		oldmsec = bacon_video_widget_get_current_time (xplayer->bvw);
		sec = MAX (0, oldmsec + _time);
	} else {
		sec = _time;
	}

	bacon_video_widget_seek_time (xplayer->bvw, sec, accurate, &err);

	xplayer_statusbar_set_seeking (XPLAYER_STATUSBAR (xplayer->statusbar), FALSE);
	xplayer_time_label_set_seeking (XPLAYER_TIME_LABEL (xplayer->fs->time_label), FALSE);

	if (err != NULL)
	{
		char *msg, *disp;

		disp = xplayer_uri_escape_for_display (xplayer->mrl);
		msg = g_strdup_printf(_("Xplayer could not play '%s'."), disp);
		g_free (disp);

		xplayer_action_stop (xplayer);
		xplayer_action_error (xplayer, msg, err->message);
		g_free (msg);
		g_error_free (err);
	}
	else
	{
		char *position, *length, *time_string;

		position = xplayer_time_to_string (bacon_video_widget_get_current_time (xplayer->bvw));
		length = xplayer_time_to_string (bacon_video_widget_get_stream_length (xplayer->bvw));
		time_string = g_strdup_printf ("%s / %s", position, length);

		bacon_video_widget_show_osd (xplayer->bvw, NULL, time_string);

		g_free (position);
		g_free (length);
		g_free (time_string);
	}
}

/**
 * xplayer_action_seek_relative:
 * @xplayer: a #XplayerObject
 * @offset: the time offset to seek to
 * @accurate: whether to use accurate seek, an accurate seek might be slower for some formats (see GStreamer docs)
 *
 * Seeks to an @offset from the current position in the stream,
 * or displays an error dialog if that's not possible.
 **/
void
xplayer_action_seek_relative (XplayerObject *xplayer, gint64 offset, gboolean accurate)
{
	xplayer_seek_time_rel (xplayer, offset, TRUE, accurate);
}

/**
 * xplayer_object_action_seek_time:
 * @xplayer: a #XplayerObject
 * @msec: the time to seek to
 * @accurate: whether to use accurate seek, an accurate seek might be slower for some formats (see GStreamer docs)
 *
 * Seeks to an absolute time in the stream, or displays an
 * error dialog if that's not possible.
 **/
void
xplayer_object_action_seek_time (XplayerObject *xplayer, gint64 msec, gboolean accurate)
{
	xplayer_seek_time_rel (xplayer, msec, FALSE, accurate);
}

void
xplayer_action_set_zoom (XplayerObject *xplayer,
		       gboolean     zoom)
{
	GtkAction *action;

	action = gtk_action_group_get_action (xplayer->main_action_group, "zoom-toggle");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), zoom);
}

/**
 * xplayer_object_get_volume:
 * @xplayer: a #XplayerObject
 *
 * Gets the current volume level, as a value between <code class="literal">0.0</code> and <code class="literal">1.0</code>.
 *
 * Return value: the volume level
 **/
double
xplayer_object_get_volume (XplayerObject *xplayer)
{
	return bacon_video_widget_get_volume (xplayer->bvw);
}

/**
 * xplayer_object_action_volume:
 * @xplayer: a #XplayerObject
 * @volume: the new absolute volume value
 *
 * Sets the volume, with <code class="literal">1.0</code> being the maximum, and <code class="literal">0.0</code> being the minimum level.
 **/
void
xplayer_object_action_volume (XplayerObject *xplayer, double volume)
{
	if (bacon_video_widget_can_set_volume (xplayer->bvw) == FALSE)
		return;

	bacon_video_widget_set_volume (xplayer->bvw, volume);
}

/**
 * xplayer_action_volume_relative:
 * @xplayer: a #XplayerObject
 * @off_pct: the value by which to increase or decrease the volume
 *
 * Sets the volume relative to its current level, with <code class="literal">1.0</code> being the
 * maximum, and <code class="literal">0.0</code> being the minimum level.
 **/
void
xplayer_action_volume_relative (XplayerObject *xplayer, double off_pct)
{
	double vol;

	if (bacon_video_widget_can_set_volume (xplayer->bvw) == FALSE)
		return;
	if (xplayer->muted != FALSE)
		xplayer_action_volume_toggle_mute (xplayer);

	vol = bacon_video_widget_get_volume (xplayer->bvw);
	bacon_video_widget_set_volume (xplayer->bvw, vol + off_pct);
}

/**
 * xplayer_action_volume_toggle_mute:
 * @xplayer: a #XplayerObject
 *
 * Toggles the mute status.
 **/
void
xplayer_action_volume_toggle_mute (XplayerObject *xplayer)
{
	if (xplayer->muted == FALSE) {
		xplayer->muted = TRUE;
		xplayer->prev_volume = bacon_video_widget_get_volume (xplayer->bvw);
		bacon_video_widget_set_volume (xplayer->bvw, 0.0);
	} else {
		xplayer->muted = FALSE;
		bacon_video_widget_set_volume (xplayer->bvw, xplayer->prev_volume);
	}
}

/**
 * xplayer_action_toggle_aspect_ratio:
 * @xplayer: a #XplayerObject
 *
 * Toggles the aspect ratio selected in the menu to the
 * next one in the list.
 **/
void
xplayer_action_toggle_aspect_ratio (XplayerObject *xplayer)
{
	GtkAction *action;
	int tmp;

	tmp = xplayer_action_get_aspect_ratio (xplayer);
	tmp++;
	if (tmp > BVW_RATIO_DVB)
		tmp = BVW_RATIO_AUTO;

	action = gtk_action_group_get_action (xplayer->main_action_group, "aspect-ratio-auto");
	gtk_radio_action_set_current_value (GTK_RADIO_ACTION (action), tmp);
}

/**
 * xplayer_action_set_aspect_ratio:
 * @xplayer: a #XplayerObject
 * @ratio: the aspect ratio to use
 *
 * Sets the aspect ratio selected in the menu to @ratio,
 * as defined in #BvwAspectRatio.
 **/
void
xplayer_action_set_aspect_ratio (XplayerObject *xplayer, int ratio)
{
	bacon_video_widget_set_aspect_ratio (xplayer->bvw, ratio);
}

/**
 * xplayer_action_get_aspect_ratio:
 * @xplayer: a #XplayerObject
 *
 * Gets the current aspect ratio as defined in #BvwAspectRatio.
 *
 * Return value: the current aspect ratio
 **/
int
xplayer_action_get_aspect_ratio (XplayerObject *xplayer)
{
	return (bacon_video_widget_get_aspect_ratio (xplayer->bvw));
}

/**
 * xplayer_action_set_scale_ratio:
 * @xplayer: a #XplayerObject
 * @ratio: the scale ratio to use
 *
 * Sets the video scale ratio, as a float where, for example,
 * 1.0 is 1:1 and 2.0 is 2:1.
 **/
void
xplayer_action_set_scale_ratio (XplayerObject *xplayer, gfloat ratio)
{
	bacon_video_widget_set_scale_ratio (xplayer->bvw, ratio);
}

void
xplayer_action_show_help (XplayerObject *xplayer)
{
	GError *error = NULL;

	if (gtk_show_uri (gtk_widget_get_screen (xplayer->win), "help:xplayer", gtk_get_current_event_time (), &error) == FALSE) {
		xplayer_action_error (xplayer, _("Xplayer could not display the help contents."), error->message);
		g_error_free (error);
	}
}

/* This is called in the main thread */
static void
xplayer_action_drop_files_finished (XplayerPlaylist *playlist, GAsyncResult *result, XplayerObject *xplayer)
{
	char *mrl, *subtitle;

	/* Reconnect the playlist's changed signal (which was disconnected below in xplayer_action_drop_files(). */
	g_signal_connect (G_OBJECT (playlist), "changed", G_CALLBACK (playlist_changed_cb), xplayer);
	mrl = xplayer_playlist_get_current_mrl (playlist, &subtitle);
	xplayer_action_set_mrl_and_play (xplayer, mrl, subtitle);
	g_free (mrl);
	g_free (subtitle);

	g_object_unref (xplayer);
}

static gboolean
xplayer_action_drop_files (XplayerObject *xplayer, GtkSelectionData *data,
		int drop_type, gboolean empty_pl)
{
	char **list;
	guint i, len;
	GList *p, *file_list, *mrl_list = NULL;
	gboolean cleared = FALSE;

	list = g_uri_list_extract_uris ((const char *) gtk_selection_data_get_data (data));
	file_list = NULL;

	for (i = 0; list[i] != NULL; i++) {
		char *filename;

		if (list[i] == NULL)
			continue;

		filename = xplayer_create_full_path (list[i]);
		file_list = g_list_prepend (file_list,
					    filename ? filename : g_strdup (list[i]));
	}
	g_strfreev (list);

	if (file_list == NULL)
		return FALSE;

	if (drop_type != 1)
		file_list = g_list_sort (file_list, (GCompareFunc) strcmp);
	else
		file_list = g_list_reverse (file_list);

	/* How many files? Check whether those could be subtitles */
	len = g_list_length (file_list);
	if (len == 1 || (len == 2 && drop_type == 1)) {
		if (xplayer_uri_is_subtitle (file_list->data) != FALSE) {
			xplayer_playlist_set_current_subtitle (xplayer->playlist, file_list->data);
			goto bail;
		}
	}

	if (empty_pl != FALSE) {
		/* The function that calls us knows better if we should be doing something with the changed playlist... */
		g_signal_handlers_disconnect_by_func (G_OBJECT (xplayer->playlist), playlist_changed_cb, xplayer);
		xplayer_playlist_clear (xplayer->playlist);
		cleared = TRUE;
	}

	/* Add each MRL to the playlist asynchronously */
	for (p = file_list; p != NULL; p = p->next) {
		const char *filename, *title;

		filename = p->data;
		title = NULL;

		/* Super _NETSCAPE_URL trick */
		if (drop_type == 1) {
			p = p->next;
			if (p != NULL) {
				if (g_str_has_prefix (p->data, "File:") != FALSE)
					title = (char *)p->data + 5;
				else
					title = p->data;
			}
		}

		/* Add the MRL data to the list of MRLs to add to the playlist */
		mrl_list = g_list_prepend (mrl_list, xplayer_playlist_mrl_data_new (filename, title));
	}

	/* Add the MRLs to the playlist asynchronously and in order. We need to reconnect playlist's "changed" signal once all of the add-MRL
	 * operations have completed. If we haven't cleared the playlist, there's no need to do this. */
	if (mrl_list != NULL && cleared == TRUE) {
		xplayer_playlist_add_mrls (xplayer->playlist, g_list_reverse (mrl_list), TRUE, NULL,
		                         (GAsyncReadyCallback) xplayer_action_drop_files_finished, g_object_ref (xplayer));
	} else if (mrl_list != NULL) {
		xplayer_playlist_add_mrls (xplayer->playlist, g_list_reverse (mrl_list), TRUE, NULL, NULL, NULL);
	}

bail:
	g_list_foreach (file_list, (GFunc) g_free, NULL);
	g_list_free (file_list);

	return TRUE;
}

static void
drop_video_cb (GtkWidget     *widget,
	 GdkDragContext     *context,
	 gint                x,
	 gint                y,
	 GtkSelectionData   *data,
	 guint               info,
	 guint               _time,
	 Xplayer              *xplayer)
{
	GtkWidget *source_widget;
	gboolean empty_pl;
	GdkDragAction action = gdk_drag_context_get_selected_action (context);

	source_widget = gtk_drag_get_source_widget (context);

	/* Drop of video on itself */
	if (source_widget && widget == source_widget && action == GDK_ACTION_MOVE) {
		gtk_drag_finish (context, FALSE, FALSE, _time);
		return;
	}

	if (action == GDK_ACTION_ASK) {
		action = xplayer_drag_ask (xplayer_get_playlist_length (xplayer) > 0);
		gdk_drag_status (context, action, GDK_CURRENT_TIME);
	}

	/* User selected cancel */
	if (action == GDK_ACTION_DEFAULT) {
		gtk_drag_finish (context, FALSE, FALSE, _time);
		return;
	}

	empty_pl = (action == GDK_ACTION_MOVE);
	xplayer_action_drop_files (xplayer, data, info, empty_pl);
	gtk_drag_finish (context, TRUE, FALSE, _time);
	return;
}

static void
drag_motion_video_cb (GtkWidget      *widget,
                      GdkDragContext *context,
                      gint            x,
                      gint            y,
                      guint           _time,
                      Xplayer          *xplayer)
{
	GdkModifierType mask;

	gdk_window_get_pointer (gtk_widget_get_window (widget), NULL, NULL, &mask);
	if (mask & GDK_CONTROL_MASK) {
		gdk_drag_status (context, GDK_ACTION_COPY, _time);
	} else if (mask & GDK_MOD1_MASK || gdk_drag_context_get_suggested_action (context) == GDK_ACTION_ASK) {
		gdk_drag_status (context, GDK_ACTION_ASK, _time);
	} else {
		gdk_drag_status (context, GDK_ACTION_MOVE, _time);
	}
}

static void
drop_playlist_cb (GtkWidget     *widget,
	       GdkDragContext     *context,
	       gint                x,
	       gint                y,
	       GtkSelectionData   *data,
	       guint               info,
	       guint               _time,
	       Xplayer              *xplayer)
{
	gboolean empty_pl;
	GdkDragAction action = gdk_drag_context_get_selected_action (context);

	if (action == GDK_ACTION_ASK) {
		action = xplayer_drag_ask (xplayer_get_playlist_length (xplayer) > 0);
		gdk_drag_status (context, action, GDK_CURRENT_TIME);
	}

	if (action == GDK_ACTION_DEFAULT) {
		gtk_drag_finish (context, FALSE, FALSE, _time);
		return;
	}

	empty_pl = (action == GDK_ACTION_MOVE);

	xplayer_action_drop_files (xplayer, data, info, empty_pl);
	gtk_drag_finish (context, TRUE, FALSE, _time);
}

static void
drag_motion_playlist_cb (GtkWidget      *widget,
			 GdkDragContext *context,
			 gint            x,
			 gint            y,
			 guint           _time,
			 Xplayer          *xplayer)
{
	GdkModifierType mask;

	gdk_window_get_pointer (gtk_widget_get_window (widget), NULL, NULL, &mask);

	if (mask & GDK_MOD1_MASK || gdk_drag_context_get_suggested_action (context) == GDK_ACTION_ASK)
		gdk_drag_status (context, GDK_ACTION_ASK, _time);
}
static void
drag_video_cb (GtkWidget *widget,
	       GdkDragContext *context,
	       GtkSelectionData *selection_data,
	       guint info,
	       guint32 _time,
	       gpointer callback_data)
{
	XplayerObject *xplayer = XPLAYER_OBJECT (callback_data);
	char *text;
	int len;
	GFile *file;

	g_assert (selection_data != NULL);

	if (xplayer->mrl == NULL)
		return;

	/* Canonicalise the MRL as a proper URI */
	file = g_file_new_for_commandline_arg (xplayer->mrl);
	text = g_file_get_uri (file);
	g_object_unref (file);

	g_return_if_fail (text != NULL);

	len = strlen (text);

	gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data),
				8, (guchar *) text, len);

	g_free (text);
}

static void
on_got_redirect (BaconVideoWidget *bvw, const char *mrl, XplayerObject *xplayer)
{
	char *new_mrl;

	if (strstr (mrl, "://") != NULL) {
		new_mrl = NULL;
	} else {
		GFile *old_file, *parent, *new_file;
		char *old_mrl;

		/* Get the parent for the current MRL, that's our base */
		old_mrl = xplayer_playlist_get_current_mrl (XPLAYER_PLAYLIST (xplayer->playlist), NULL);
		old_file = g_file_new_for_uri (old_mrl);
		g_free (old_mrl);
		parent = g_file_get_parent (old_file);
		g_object_unref (old_file);

		/* Resolve the URL */
		new_file = g_file_get_child (parent, mrl);
		g_object_unref (parent);

		new_mrl = g_file_get_uri (new_file);
		g_object_unref (new_file);
	}

	bacon_video_widget_close (xplayer->bvw);
	xplayer_file_closed (xplayer);
	xplayer->has_played_emitted = FALSE;
	xplayer_gdk_window_set_waiting_cursor (gtk_widget_get_window (xplayer->win));
	bacon_video_widget_open (xplayer->bvw, new_mrl ? new_mrl : mrl);
	xplayer_file_opened (xplayer, new_mrl ? new_mrl : mrl);
	gdk_window_set_cursor (gtk_widget_get_window (xplayer->win), NULL);
	if (bacon_video_widget_play (bvw, NULL) != FALSE) {
		xplayer_file_has_played (xplayer, xplayer->mrl);
		xplayer->has_played_emitted = TRUE;
	}
	g_free (new_mrl);
}

static void
on_channels_change_event (BaconVideoWidget *bvw, XplayerObject *xplayer)
{
	gchar *name;

	xplayer_sublang_update (xplayer);
	update_media_menu_items (xplayer);

	/* updated stream info (new song) */
	name = xplayer_get_nice_name_for_stream (xplayer);

	if (name != NULL) {
		update_mrl_label (xplayer, name);
		xplayer_playlist_set_title
			(XPLAYER_PLAYLIST (xplayer->playlist), name);
		g_free (name);
	}
}

static void
on_playlist_change_name (XplayerPlaylist *playlist, XplayerObject *xplayer)
{
	char *name;

	name = xplayer_playlist_get_current_title (playlist);
	if (name != NULL) {
		update_mrl_label (xplayer, name);
		g_free (name);
	}
}

static void
on_got_metadata_event (BaconVideoWidget *bvw, XplayerObject *xplayer)
{
        char *name = NULL;

	name = xplayer_get_nice_name_for_stream (xplayer);

	if (name != NULL) {
		xplayer_playlist_set_title
			(XPLAYER_PLAYLIST (xplayer->playlist), name);
		g_free (name);
	}

	on_playlist_change_name (XPLAYER_PLAYLIST (xplayer->playlist), xplayer);
}

static void
on_error_event (BaconVideoWidget *bvw, char *message,
                gboolean playback_stopped, XplayerObject *xplayer)
{
	/* Clear the seek if it's there, we only want to try and seek
	 * the first file, even if it's not there */
	xplayer->seek_to = 0;
	xplayer->seek_to_start = 0;

	if (playback_stopped)
		play_pause_set_label (xplayer, STATE_STOPPED);

	xplayer_action_error (xplayer, _("An error occurred"), message);
}

static void
on_buffering_event (BaconVideoWidget *bvw, gdouble percentage, XplayerObject *xplayer)
{
	xplayer_statusbar_push (XPLAYER_STATUSBAR (xplayer->statusbar), percentage);
}

static void
on_download_buffering_event (BaconVideoWidget *bvw, gdouble level, XplayerObject *xplayer)
{
	update_fill (xplayer, level);
}

static void
update_fill (XplayerObject *xplayer, gdouble level)
{
	if (level < 0.0) {
		gtk_range_set_show_fill_level (GTK_RANGE (xplayer->seek), FALSE);
		gtk_range_set_show_fill_level (GTK_RANGE (xplayer->fs->seek), FALSE);
	} else {
		gtk_range_set_fill_level (GTK_RANGE (xplayer->seek), level * 65535.0f);
		gtk_range_set_show_fill_level (GTK_RANGE (xplayer->seek), TRUE);

		gtk_range_set_fill_level (GTK_RANGE (xplayer->fs->seek), level * 65535.0f);
		gtk_range_set_show_fill_level (GTK_RANGE (xplayer->fs->seek), TRUE);
	}
}

static void
update_seekable (XplayerObject *xplayer)
{
	GtkAction *action;
	GtkActionGroup *action_group;
	gboolean seekable;

	seekable = bacon_video_widget_is_seekable (xplayer->bvw);
	if (xplayer->seekable == seekable)
		return;
	xplayer->seekable = seekable;

	/* Check if the stream is seekable */
	gtk_widget_set_sensitive (xplayer->seek, seekable);

	xplayer_main_set_sensitivity ("tmw_seek_hbox", seekable);

	xplayer_fullscreen_set_seekable (xplayer->fs, seekable);

	/* FIXME: We can use this code again once bug #457631 is fixed and
	 * skip-* are back in the main action group. */
	/*xplayer_action_set_sensitivity ("skip-forward", seekable);
	xplayer_action_set_sensitivity ("skip-backwards", seekable);*/
	action_group = GTK_ACTION_GROUP (gtk_builder_get_object (xplayer->xml, "skip-action-group"));

	action = gtk_action_group_get_action (action_group, "skip-forward");
	gtk_action_set_sensitive (action, seekable);

	action = gtk_action_group_get_action (action_group, "skip-backwards");
	gtk_action_set_sensitive (action, seekable);

	/* This is for the session restore and the position saving
	 * to seek to the saved time */
	if (seekable != FALSE) {
		if (xplayer->seek_to_start != 0) {
			bacon_video_widget_seek_time (xplayer->bvw,
						      xplayer->seek_to_start, FALSE, NULL);
			xplayer_action_pause (xplayer);
		} else if (xplayer->seek_to != 0) {
			bacon_video_widget_seek_time (xplayer->bvw,
						      xplayer->seek_to, FALSE, NULL);
		}
	}
	xplayer->seek_to = 0;
	xplayer->seek_to_start = 0;

	g_object_notify (G_OBJECT (xplayer), "seekable");
}

static void
update_slider_visibility (XplayerObject *xplayer,
			  gint64 stream_length)
{
	if (xplayer->stream_length == stream_length)
		return;
	if (xplayer->stream_length > 0 &&
	    stream_length > 0)
		return;
	if (stream_length != 0) {
		gtk_range_set_range (GTK_RANGE (xplayer->seek), 0., 65535.);
		gtk_range_set_range (GTK_RANGE (xplayer->fs->seek), 0., 65535.);
	} else {
		gtk_range_set_range (GTK_RANGE (xplayer->seek), 0., 0.);
		gtk_range_set_range (GTK_RANGE (xplayer->fs->seek), 0., 0.);
	}
}

static void
update_current_time (BaconVideoWidget *bvw,
		gint64 current_time,
		gint64 stream_length,
		double current_position,
		gboolean seekable, XplayerObject *xplayer)
{
	update_slider_visibility (xplayer, stream_length);

	if (xplayer->seek_lock == FALSE) {
		gtk_adjustment_set_value (xplayer->seekadj,
					  current_position * 65535);

		if (stream_length == 0 && xplayer->mrl != NULL)
		{
			xplayer_statusbar_set_time_and_length
				(XPLAYER_STATUSBAR (xplayer->statusbar),
				(int) (current_time / 1000), -1);
		} else {
			xplayer_statusbar_set_time_and_length
				(XPLAYER_STATUSBAR (xplayer->statusbar),
				(int) (current_time / 1000),
				(int) (stream_length / 1000));
		}

		xplayer_time_label_set_time
			(XPLAYER_TIME_LABEL (xplayer->fs->time_label),
			 current_time, stream_length);
	}

	if (xplayer->stream_length != stream_length) {
		g_object_notify (G_OBJECT (xplayer), "stream-length");
		xplayer->stream_length = stream_length;
	}
}

void
volume_button_value_changed_cb (GtkScaleButton *button, gdouble value, XplayerObject *xplayer)
{
	xplayer->muted = FALSE;
	bacon_video_widget_set_volume (xplayer->bvw, value);
}

static void
update_volume_sliders (XplayerObject *xplayer)
{
	double volume;
	GtkAction *action;

	volume = bacon_video_widget_get_volume (xplayer->bvw);

	g_signal_handlers_block_by_func (xplayer->volume, volume_button_value_changed_cb, xplayer);
	gtk_scale_button_set_value (GTK_SCALE_BUTTON (xplayer->volume), volume);
	g_signal_handlers_unblock_by_func (xplayer->volume, volume_button_value_changed_cb, xplayer);
  
	action = gtk_action_group_get_action (xplayer->main_action_group, "volume-down");
	gtk_action_set_sensitive (action, volume > VOLUME_EPSILON && xplayer->volume_sensitive);

	action = gtk_action_group_get_action (xplayer->main_action_group, "volume-up");
	gtk_action_set_sensitive (action, volume < (1.0 - VOLUME_EPSILON) && xplayer->volume_sensitive);
}

static void
property_notify_cb_volume (BaconVideoWidget *bvw, GParamSpec *spec, XplayerObject *xplayer)
{
	update_volume_sliders (xplayer);
}

static void
property_notify_cb_seekable (BaconVideoWidget *bvw, GParamSpec *spec, XplayerObject *xplayer)
{
	update_seekable (xplayer);
}

gboolean
seek_slider_pressed_cb (GtkWidget *widget, GdkEventButton *event, XplayerObject *xplayer)
{
	/* HACK: we want the behaviour you get with the left button, so we
	 * mangle the event.  clicking with other buttons moves the slider in
	 * step increments, clicking with the left button moves the slider to
	 * the location of the click.
	 */
	event->button = GDK_BUTTON_PRIMARY;

	xplayer->seek_lock = TRUE;
	if (bacon_video_widget_can_direct_seek (xplayer->bvw) == FALSE) {
		xplayer_statusbar_set_seeking (XPLAYER_STATUSBAR (xplayer->statusbar), TRUE);
		xplayer_time_label_set_seeking (XPLAYER_TIME_LABEL (xplayer->fs->time_label), TRUE);
	}

	return FALSE;
}

void
seek_slider_changed_cb (GtkAdjustment *adj, XplayerObject *xplayer)
{
	double pos;
	gint _time;

	if (xplayer->seek_lock == FALSE)
		return;

	pos = gtk_adjustment_get_value (adj) / 65535;
	_time = bacon_video_widget_get_stream_length (xplayer->bvw);
	xplayer_statusbar_set_time_and_length (XPLAYER_STATUSBAR (xplayer->statusbar),
			(int) (pos * _time / 1000), _time / 1000);
	xplayer_time_label_set_time
			(XPLAYER_TIME_LABEL (xplayer->fs->time_label),
			 (int) (pos * _time), _time);

	if (bacon_video_widget_can_direct_seek (xplayer->bvw) != FALSE)
		xplayer_action_seek (xplayer, pos);
}

gboolean
seek_slider_released_cb (GtkWidget *widget, GdkEventButton *event, XplayerObject *xplayer)
{
	GtkAdjustment *adj;
	gdouble val;

	/* HACK: see seek_slider_pressed_cb */
	event->button = GDK_BUTTON_PRIMARY;

	/* set to FALSE here to avoid triggering a final seek when
	 * syncing the adjustments while being in direct seek mode */
	xplayer->seek_lock = FALSE;

	/* sync both adjustments */
	adj = gtk_range_get_adjustment (GTK_RANGE (widget));
	val = gtk_adjustment_get_value (adj);

	if (bacon_video_widget_can_direct_seek (xplayer->bvw) == FALSE)
		xplayer_action_seek (xplayer, val / 65535.0);

	xplayer_statusbar_set_seeking (XPLAYER_STATUSBAR (xplayer->statusbar), FALSE);
	xplayer_time_label_set_seeking (XPLAYER_TIME_LABEL (xplayer->fs->time_label),
			FALSE);
	return FALSE;
}

gboolean
xplayer_action_open_files (XplayerObject *xplayer, char **list)
{
	GSList *slist = NULL;
	int i, retval;

	for (i = 0 ; list[i] != NULL; i++)
		slist = g_slist_prepend (slist, list[i]);

	slist = g_slist_reverse (slist);
	retval = xplayer_action_open_files_list (xplayer, slist);
	g_slist_free (slist);

	return retval;
}

static gboolean
xplayer_action_open_files_list (XplayerObject *xplayer, GSList *list)
{
	GSList *l;
	GList *mrl_list = NULL;
	gboolean changed;
	gboolean cleared;

	changed = FALSE;
	cleared = FALSE;

	if (list == NULL)
		return changed;

	xplayer_gdk_window_set_waiting_cursor (gtk_widget_get_window (xplayer->win));

	for (l = list ; l != NULL; l = l->next)
	{
		char *filename;
		char *data = l->data;

		if (data == NULL)
			continue;

		/* Ignore relatives paths that start with "--", tough luck */
		if (data[0] == '-' && data[1] == '-')
			continue;

		/* Get the subtitle part out for our tests */
		filename = xplayer_create_full_path (data);
		if (filename == NULL)
			filename = g_strdup (data);

		if (g_file_test (filename, G_FILE_TEST_IS_REGULAR)
				|| strstr (filename, "#") != NULL
				|| strstr (filename, "://") != NULL
				|| g_str_has_prefix (filename, "dvd:") != FALSE
				|| g_str_has_prefix (filename, "vcd:") != FALSE
				|| g_str_has_prefix (filename, "dvb:") != FALSE)
		{
			if (cleared == FALSE)
			{
				/* The function that calls us knows better
				 * if we should be doing something with the 
				 * changed playlist ... */
				g_signal_handlers_disconnect_by_func
					(G_OBJECT (xplayer->playlist),
					 playlist_changed_cb, xplayer);
				changed = xplayer_playlist_clear (xplayer->playlist);
				bacon_video_widget_close (xplayer->bvw);
				xplayer_file_closed (xplayer);
				xplayer->has_played_emitted = FALSE;
				cleared = TRUE;
			}

			if (g_str_has_prefix (filename, "dvb:/") != FALSE) {
				mrl_list = g_list_prepend (mrl_list, xplayer_playlist_mrl_data_new (data, NULL));
				changed = TRUE;
			} else {
				mrl_list = g_list_prepend (mrl_list, xplayer_playlist_mrl_data_new (filename, NULL));
				changed = TRUE;
			}
		}

		g_free (filename);
	}

	/* Add the MRLs to the playlist asynchronously and in order */
	if (mrl_list != NULL)
		xplayer_playlist_add_mrls (xplayer->playlist, g_list_reverse (mrl_list), FALSE, NULL, NULL, NULL);

	gdk_window_set_cursor (gtk_widget_get_window (xplayer->win), NULL);

	/* ... and reconnect because we're nice people */
	if (cleared != FALSE)
	{
		g_signal_connect (G_OBJECT (xplayer->playlist),
				"changed", G_CALLBACK (playlist_changed_cb),
				xplayer);
	}

	return changed;
}

void
show_controls (XplayerObject *xplayer, gboolean was_fullscreen)
{
	GtkAction *action;
	GtkWidget *menubar, *controlbar, *statusbar, *bvw_box, *widget;
	GtkAllocation allocation;
	int width = 0, height = 0;

	if (xplayer->bvw == NULL)
		return;

	menubar = GTK_WIDGET (gtk_builder_get_object (xplayer->xml, "tmw_menubar_box"));
	controlbar = GTK_WIDGET (gtk_builder_get_object (xplayer->xml, "tmw_controls_vbox"));
	statusbar = GTK_WIDGET (gtk_builder_get_object (xplayer->xml, "tmw_statusbar"));
	bvw_box = GTK_WIDGET (gtk_builder_get_object (xplayer->xml, "tmw_bvw_box"));
	widget = GTK_WIDGET (xplayer->bvw);

	action = gtk_action_group_get_action (xplayer->main_action_group, "show-controls");
	gtk_action_set_sensitive (action, !xplayer_is_fullscreen (xplayer));
	gtk_widget_get_allocation (widget, &allocation);

	if (xplayer->controls_visibility == XPLAYER_CONTROLS_VISIBLE) {
		if (was_fullscreen == FALSE) {
			height = allocation.height;
			width = allocation.width;
		}

		gtk_widget_set_sensitive (menubar, TRUE);
		gtk_widget_show (menubar);
		gtk_widget_show (controlbar);
		gtk_widget_show (statusbar);
		if (xplayer_sidebar_is_visible (xplayer) != FALSE) {
			/* This is uglier then you might expect because of the
			   resize handle between the video and sidebar. There
			   is no convenience method to get the handle's width.
			   */
			GValue value = { 0, };
			GtkWidget *pane;
			GtkAllocation allocation_sidebar;
			int handle_size;

			g_value_init (&value, G_TYPE_INT);
			pane = GTK_WIDGET (gtk_builder_get_object (xplayer->xml,
					"tmw_main_pane"));
			gtk_widget_style_get_property (pane, "handle-size",
					&value);
			handle_size = g_value_get_int (&value);
			g_value_unset (&value);

			gtk_widget_show (xplayer->sidebar);
			gtk_widget_get_allocation (xplayer->sidebar, &allocation_sidebar);
			width += allocation_sidebar.width + handle_size;
		} else {
			xplayer_action_save_size (xplayer);
			gtk_widget_hide (xplayer->sidebar);
		}

		if (was_fullscreen == FALSE) {
			GtkAllocation allocation_menubar;
			GtkAllocation allocation_controlbar;
			GtkAllocation allocation_statusbar;

			gtk_widget_get_allocation (menubar, &allocation_menubar);
			gtk_widget_get_allocation (controlbar, &allocation_controlbar);
			gtk_widget_get_allocation (statusbar, &allocation_statusbar);
			height += allocation_menubar.height
				+ allocation_controlbar.height
				+ allocation_statusbar.height;
			gtk_window_resize (GTK_WINDOW(xplayer->win),
					width, height);
		}
	} else {
		if (xplayer->controls_visibility == XPLAYER_CONTROLS_HIDDEN) {
			width = allocation.width;
			height = allocation.height;
		}

		/* Hide and make the menubar unsensitive */
		gtk_widget_set_sensitive (menubar, FALSE);
		gtk_widget_hide (menubar);

		gtk_widget_hide (controlbar);
		gtk_widget_hide (statusbar);
		gtk_widget_hide (xplayer->sidebar);

		 /* We won't show controls in fullscreen */
		gtk_container_set_border_width (GTK_CONTAINER (bvw_box), 0);

		if (xplayer->controls_visibility == XPLAYER_CONTROLS_HIDDEN) {
			gtk_window_resize (GTK_WINDOW(xplayer->win),
					width, height);
		}
	}
}

/**
 * xplayer_action_toggle_controls:
 * @xplayer: a #XplayerObject
 *
 * If Xplayer's not fullscreened, this toggles the state of the "Show Controls"
 * menu entry, and consequently shows or hides the controls in the UI.
 **/
void
xplayer_action_toggle_controls (XplayerObject *xplayer)
{
	GtkAction *action;
	gboolean state;

	if (xplayer_is_fullscreen (xplayer) != FALSE)
		return;

 	action = gtk_action_group_get_action (xplayer->main_action_group,
 		"show-controls");
 	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
 	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), !state);
}

/**
 * xplayer_action_next_angle:
 * @xplayer: a #XplayerObject
 *
 * Switches to the next angle, if watching a DVD. If not watching a DVD, this is a
 * no-op.
 **/
void
xplayer_action_next_angle (XplayerObject *xplayer)
{
	bacon_video_widget_set_next_angle (xplayer->bvw);
}

/**
 * xplayer_action_set_playlist_index:
 * @xplayer: a #XplayerObject
 * @index: the new playlist index
 *
 * Sets the <code class="literal">0</code>-based playlist index to @index, causing Xplayer to load and
 * start playing that playlist entry.
 *
 * If @index is higher than the current length of the playlist, this
 * has the effect of restarting the current playlist entry.
 **/
void
xplayer_action_set_playlist_index (XplayerObject *xplayer, guint playlist_index)
{
	char *mrl, *subtitle;

	xplayer_playlist_set_current (xplayer->playlist, playlist_index);
	mrl = xplayer_playlist_get_current_mrl (xplayer->playlist, &subtitle);
	xplayer_action_set_mrl_and_play (xplayer, mrl, subtitle);
	g_free (mrl);
	g_free (subtitle);
}

/**
 * xplayer_object_action_remote:
 * @xplayer: a #XplayerObject
 * @cmd: a #XplayerRemoteCommand
 * @url: an MRL to play, or %NULL
 *
 * Executes the specified @cmd on this instance of Xplayer. If @cmd
 * is an operation requiring an MRL, @url is required; it can be %NULL
 * otherwise.
 *
 * If Xplayer's fullscreened and the operation is executed correctly,
 * the controls will appear as if the user had moved the mouse.
 **/
void
xplayer_object_action_remote (XplayerObject *xplayer, XplayerRemoteCommand cmd, const char *url)
{
	const char *icon_name;
	gboolean handled;

	icon_name = NULL;
	handled = TRUE;

	switch (cmd) {
	case XPLAYER_REMOTE_COMMAND_PLAY:
		xplayer_action_play (xplayer);
		icon_name = xplayer_get_rtl_icon_name ("media-playback-start");
		break;
	case XPLAYER_REMOTE_COMMAND_PLAYPAUSE:
		if (bacon_video_widget_is_playing (xplayer->bvw) == FALSE)
			icon_name = xplayer_get_rtl_icon_name ("media-playback-start");
		else
			icon_name = "media-playback-pause-symbolic";
		xplayer_action_play_pause (xplayer);
		break;
	case XPLAYER_REMOTE_COMMAND_PAUSE:
		xplayer_action_pause (xplayer);
		icon_name = "media-playback-pause-symbolic";
		break;
	case XPLAYER_REMOTE_COMMAND_STOP: {
		char *mrl, *subtitle;

		xplayer_playlist_set_at_start (xplayer->playlist);
		update_buttons (xplayer);
		xplayer_action_stop (xplayer);
		mrl = xplayer_playlist_get_current_mrl (xplayer->playlist, &subtitle);
		if (mrl != NULL) {
			xplayer_action_set_mrl_with_warning (xplayer, mrl, subtitle, FALSE);
			bacon_video_widget_pause (xplayer->bvw);
			g_free (mrl);
			g_free (subtitle);
		}
		icon_name = "media-playback-stop-symbolic";
		break;
	};
	case XPLAYER_REMOTE_COMMAND_SEEK_FORWARD: {
		double offset = 0;

		if (url != NULL)
			offset = g_ascii_strtod (url, NULL);
		if (offset == 0) {
			xplayer_action_seek_relative (xplayer, SEEK_FORWARD_OFFSET * 1000, FALSE);
		} else {
			xplayer_action_seek_relative (xplayer, offset * 1000, FALSE);
		}
		icon_name = xplayer_get_rtl_icon_name ("media-seek-forward");
		break;
	}
	case XPLAYER_REMOTE_COMMAND_SEEK_BACKWARD: {
		double offset = 0;

		if (url != NULL)
			offset = g_ascii_strtod (url, NULL);
		if (offset == 0)
			xplayer_action_seek_relative (xplayer, SEEK_BACKWARD_OFFSET * 1000, FALSE);
		else
			xplayer_action_seek_relative (xplayer,  - (offset * 1000), FALSE);
		icon_name = xplayer_get_rtl_icon_name ("media-seek-backward");
		break;
	}
	case XPLAYER_REMOTE_COMMAND_VOLUME_UP:
		xplayer_action_volume_relative (xplayer, VOLUME_UP_OFFSET);
		break;
	case XPLAYER_REMOTE_COMMAND_VOLUME_DOWN:
		xplayer_action_volume_relative (xplayer, VOLUME_DOWN_OFFSET);
		break;
	case XPLAYER_REMOTE_COMMAND_NEXT:
		xplayer_action_next (xplayer);
		icon_name = xplayer_get_rtl_icon_name ("media-skip-forward");
		break;
	case XPLAYER_REMOTE_COMMAND_PREVIOUS:
		xplayer_action_previous (xplayer);
		icon_name = xplayer_get_rtl_icon_name ("media-skip-backward");
		break;
	case XPLAYER_REMOTE_COMMAND_FULLSCREEN:
		xplayer_action_fullscreen_toggle (xplayer);
		break;
	case XPLAYER_REMOTE_COMMAND_QUIT:
		xplayer_action_exit (xplayer);
		break;
	case XPLAYER_REMOTE_COMMAND_ENQUEUE:
		g_assert (url != NULL);
		xplayer_playlist_add_mrl (xplayer->playlist, url, NULL, TRUE, NULL, NULL, NULL);
		break;
	case XPLAYER_REMOTE_COMMAND_REPLACE:
		xplayer_playlist_clear (xplayer->playlist);
		if (url == NULL) {
			bacon_video_widget_close (xplayer->bvw);
			xplayer_file_closed (xplayer);
			xplayer->has_played_emitted = FALSE;
			xplayer_action_set_mrl (xplayer, NULL, NULL);
			break;
		}
		xplayer_playlist_add_mrl (xplayer->playlist, url, NULL, TRUE, NULL, NULL, NULL);
		break;
	case XPLAYER_REMOTE_COMMAND_SHOW:
		gtk_window_present_with_time (GTK_WINDOW (xplayer->win), GDK_CURRENT_TIME);
		break;
	case XPLAYER_REMOTE_COMMAND_TOGGLE_CONTROLS:
		if (xplayer->controls_visibility != XPLAYER_CONTROLS_FULLSCREEN)
		{
			GtkToggleAction *action;
			gboolean state;

			action = GTK_TOGGLE_ACTION (gtk_action_group_get_action
					(xplayer->main_action_group,
					 "show-controls"));
			state = gtk_toggle_action_get_active (action);
			gtk_toggle_action_set_active (action, !state);
		}
		break;
	case XPLAYER_REMOTE_COMMAND_UP:
		bacon_video_widget_dvd_event (xplayer->bvw,
				BVW_DVD_ROOT_MENU_UP);
		break;
	case XPLAYER_REMOTE_COMMAND_DOWN:
		bacon_video_widget_dvd_event (xplayer->bvw,
				BVW_DVD_ROOT_MENU_DOWN);
		break;
	case XPLAYER_REMOTE_COMMAND_LEFT:
		bacon_video_widget_dvd_event (xplayer->bvw,
				BVW_DVD_ROOT_MENU_LEFT);
		break;
	case XPLAYER_REMOTE_COMMAND_RIGHT:
		bacon_video_widget_dvd_event (xplayer->bvw,
				BVW_DVD_ROOT_MENU_RIGHT);
		break;
	case XPLAYER_REMOTE_COMMAND_SELECT:
		bacon_video_widget_dvd_event (xplayer->bvw,
				BVW_DVD_ROOT_MENU_SELECT);
		break;
	case XPLAYER_REMOTE_COMMAND_DVD_MENU:
		bacon_video_widget_dvd_event (xplayer->bvw,
				BVW_DVD_ROOT_MENU);
		break;
	case XPLAYER_REMOTE_COMMAND_ZOOM_UP:
		xplayer_action_set_zoom (xplayer, TRUE);
		break;
	case XPLAYER_REMOTE_COMMAND_ZOOM_DOWN:
		xplayer_action_set_zoom (xplayer, FALSE);
		break;
	case XPLAYER_REMOTE_COMMAND_EJECT:
		xplayer_action_eject (xplayer);
		icon_name = "media-eject";
		break;
	case XPLAYER_REMOTE_COMMAND_PLAY_DVD:
		/* FIXME - focus the "Optical Media" section in Grilo */
		break;
	case XPLAYER_REMOTE_COMMAND_MUTE:
		xplayer_action_volume_toggle_mute (xplayer);
		break;
	case XPLAYER_REMOTE_COMMAND_TOGGLE_ASPECT:
		xplayer_action_toggle_aspect_ratio (xplayer);
		break;
	case XPLAYER_REMOTE_COMMAND_UNKNOWN:
	default:
		handled = FALSE;
		break;
	}

	if (handled != FALSE
	    && gtk_window_is_active (GTK_WINDOW (xplayer->win))) {
		xplayer_fullscreen_show_popups_or_osd (xplayer->fs, icon_name, TRUE);
	}
}

/**
 * xplayer_object_action_remote_set_setting:
 * @xplayer: a #XplayerObject
 * @setting: a #XplayerRemoteSetting
 * @value: the new value for the setting
 *
 * Sets @setting to @value on this instance of Xplayer.
 **/
void xplayer_object_action_remote_set_setting (XplayerObject *xplayer,
					     XplayerRemoteSetting setting,
					     gboolean value)
{
	GtkAction *action;

	action = NULL;

	switch (setting) {
	case XPLAYER_REMOTE_SETTING_SHUFFLE:
		action = gtk_action_group_get_action (xplayer->main_action_group, "shuffle-mode");
		break;
	case XPLAYER_REMOTE_SETTING_REPEAT:
		action = gtk_action_group_get_action (xplayer->main_action_group, "repeat-mode");
		break;
	default:
		g_assert_not_reached ();
	}

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), value);
}

/**
 * xplayer_object_action_remote_get_setting:
 * @xplayer: a #XplayerObject
 * @setting: a #XplayerRemoteSetting
 *
 * Returns the value of @setting for this instance of Xplayer.
 *
 * Return value: %TRUE if the setting is enabled, %FALSE otherwise
 **/
gboolean xplayer_object_action_remote_get_setting (XplayerObject *xplayer,
						 XplayerRemoteSetting setting)
{
	GtkAction *action;

	action = NULL;

	switch (setting) {
	case XPLAYER_REMOTE_SETTING_SHUFFLE:
		action = gtk_action_group_get_action (xplayer->main_action_group, "shuffle-mode");
		break;
	case XPLAYER_REMOTE_SETTING_REPEAT:
		action = gtk_action_group_get_action (xplayer->main_action_group, "repeat-mode");
		break;
	default:
		g_assert_not_reached ();
	}

	return gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
}

static void
playlist_changed_cb (GtkWidget *playlist, XplayerObject *xplayer)
{
	char *mrl, *subtitle;

	update_buttons (xplayer);
	mrl = xplayer_playlist_get_current_mrl (xplayer->playlist, &subtitle);

	if (mrl == NULL)
		return;

	if (xplayer_playlist_get_playing (xplayer->playlist) == XPLAYER_PLAYLIST_STATUS_NONE)
		xplayer_action_set_mrl_and_play (xplayer, mrl, subtitle);

	g_free (mrl);
	g_free (subtitle);
}

static void
item_activated_cb (GtkWidget *playlist, XplayerObject *xplayer)
{
	xplayer_action_seek (xplayer, 0);
}

static void
current_removed_cb (GtkWidget *playlist, XplayerObject *xplayer)
{
	char *mrl, *subtitle;

	/* Set play button status */
	play_pause_set_label (xplayer, STATE_STOPPED);
	mrl = xplayer_playlist_get_current_mrl (xplayer->playlist, &subtitle);

	if (mrl == NULL) {
		g_free (subtitle);
		subtitle = NULL;
		xplayer_playlist_set_at_start (xplayer->playlist);
		update_buttons (xplayer);
		mrl = xplayer_playlist_get_current_mrl (xplayer->playlist, &subtitle);
	} else {
		update_buttons (xplayer);
	}

	xplayer_action_set_mrl_and_play (xplayer, mrl, subtitle);
	g_free (mrl);
	g_free (subtitle);
}

static void
subtitle_changed_cb (GtkWidget *playlist, XplayerObject *xplayer)
{
	char *mrl, *subtitle;

	mrl = xplayer_playlist_get_current_mrl (xplayer->playlist, &subtitle);
	bacon_video_widget_set_text_subtitle (xplayer->bvw, subtitle);

	g_free (mrl);
	g_free (subtitle);
}

static void
playlist_repeat_toggle_cb (XplayerPlaylist *playlist, gboolean repeat, XplayerObject *xplayer)
{
	GtkAction *action;

	action = gtk_action_group_get_action (xplayer->main_action_group, "repeat-mode");

	g_signal_handlers_block_matched (G_OBJECT (action), G_SIGNAL_MATCH_DATA, 0, 0,
			NULL, NULL, xplayer);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), repeat);

	g_signal_handlers_unblock_matched (G_OBJECT (action), G_SIGNAL_MATCH_DATA, 0, 0,
			NULL, NULL, xplayer);
}

static void
playlist_shuffle_toggle_cb (XplayerPlaylist *playlist, gboolean shuffle, XplayerObject *xplayer)
{
	GtkAction *action;

	action = gtk_action_group_get_action (xplayer->main_action_group, "shuffle-mode");

	g_signal_handlers_block_matched (G_OBJECT (action), G_SIGNAL_MATCH_DATA, 0, 0,
			NULL, NULL, xplayer);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), shuffle);

	g_signal_handlers_unblock_matched (G_OBJECT (action), G_SIGNAL_MATCH_DATA, 0, 0,
			NULL, NULL, xplayer);
}

/**
 * xplayer_is_fullscreen:
 * @xplayer: a #XplayerObject
 *
 * Returns %TRUE if Xplayer is fullscreened.
 *
 * Return value: %TRUE if Xplayer is fullscreened
 **/
gboolean
xplayer_is_fullscreen (XplayerObject *xplayer)
{
	g_return_val_if_fail (XPLAYER_IS_OBJECT (xplayer), FALSE);

	return (xplayer->controls_visibility == XPLAYER_CONTROLS_FULLSCREEN);
}

/**
 * xplayer_object_is_playing:
 * @xplayer: a #XplayerObject
 *
 * Returns %TRUE if Xplayer is playing a stream.
 *
 * Return value: %TRUE if Xplayer is playing a stream
 **/
gboolean
xplayer_object_is_playing (XplayerObject *xplayer)
{
	g_return_val_if_fail (XPLAYER_IS_OBJECT (xplayer), FALSE);

	if (xplayer->bvw == NULL)
		return FALSE;

	return bacon_video_widget_is_playing (xplayer->bvw) != FALSE;
}

/**
 * xplayer_object_is_paused:
 * @xplayer: a #XplayerObject
 *
 * Returns %TRUE if playback is paused.
 *
 * Return value: %TRUE if playback is paused, %FALSE otherwise
 **/
gboolean
xplayer_object_is_paused (XplayerObject *xplayer)
{
	g_return_val_if_fail (XPLAYER_IS_OBJECT (xplayer), FALSE);

	return xplayer->state == STATE_PAUSED;
}

/**
 * xplayer_object_is_seekable:
 * @xplayer: a #XplayerObject
 *
 * Returns %TRUE if the current stream is seekable.
 *
 * Return value: %TRUE if the current stream is seekable
 **/
gboolean
xplayer_object_is_seekable (XplayerObject *xplayer)
{
	g_return_val_if_fail (XPLAYER_IS_OBJECT (xplayer), FALSE);

	if (xplayer->bvw == NULL)
		return FALSE;

	return bacon_video_widget_is_seekable (xplayer->bvw) != FALSE;
}

static void
on_mouse_click_fullscreen (GtkWidget *widget, XplayerObject *xplayer)
{
	if (xplayer_fullscreen_is_fullscreen (xplayer->fs) != FALSE)
		xplayer_fullscreen_show_popups (xplayer->fs, TRUE);
}

static gboolean
on_video_button_press_event (BaconVideoWidget *bvw, GdkEventButton *event,
		XplayerObject *xplayer)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
		gtk_widget_grab_focus (GTK_WIDGET (bvw));
		return TRUE;
	} else if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {
		xplayer_action_fullscreen_toggle(xplayer);
		return TRUE;
	} else if (event->type == GDK_BUTTON_PRESS && event->button == 2) {
		const char *icon_name;
		if (bacon_video_widget_is_playing (xplayer->bvw) == FALSE)
			icon_name = xplayer_get_rtl_icon_name ("media-playback-start");
		else
			icon_name = "media-playback-pause-symbolic";
		xplayer_fullscreen_show_popups_or_osd (xplayer->fs, icon_name, FALSE);
		xplayer_action_play_pause (xplayer);
		return TRUE;
	} else if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		xplayer_action_menu_popup (xplayer, event->button);
		return TRUE;
	}

	return FALSE;
}

static gboolean
on_eos_event (GtkWidget *widget, XplayerObject *xplayer)
{
	reset_seek_status (xplayer);

	if (bacon_video_widget_get_logo_mode (xplayer->bvw) != FALSE)
		return FALSE;

	if (xplayer_playlist_has_next_mrl (xplayer->playlist) == FALSE &&
	    xplayer_playlist_get_repeat (xplayer->playlist) == FALSE &&
	    (xplayer_playlist_get_last (xplayer->playlist) != 0 ||
	     xplayer_is_seekable (xplayer) == FALSE)) {
		char *mrl, *subtitle;

		/* Set play button status */
		xplayer_playlist_set_at_start (xplayer->playlist);
		update_buttons (xplayer);
		xplayer_action_stop (xplayer);
		mrl = xplayer_playlist_get_current_mrl (xplayer->playlist, &subtitle);
		xplayer_action_set_mrl_with_warning (xplayer, mrl, subtitle, FALSE);
		bacon_video_widget_pause (xplayer->bvw);
		g_free (mrl);
		g_free (subtitle);
	} else {
		if (xplayer_playlist_get_last (xplayer->playlist) == 0 &&
		    xplayer_is_seekable (xplayer)) {
			if (xplayer_playlist_get_repeat (xplayer->playlist) != FALSE) {
				xplayer_action_seek_time (xplayer, 0, FALSE);
				xplayer_action_play (xplayer);
			} else {
				xplayer_action_pause (xplayer);
				xplayer_action_seek_time (xplayer, 0, FALSE);
			}
		} else {
			xplayer_action_next (xplayer);
		}
	}

	return FALSE;
}

static gboolean
xplayer_action_handle_key_release (XplayerObject *xplayer, GdkEventKey *event)
{
	gboolean retval = TRUE;

	switch (event->keyval) {
	case GDK_KEY_Left:
	case GDK_KEY_Right:
		xplayer_statusbar_set_seeking (XPLAYER_STATUSBAR (xplayer->statusbar), FALSE);
		xplayer_time_label_set_seeking (XPLAYER_TIME_LABEL (xplayer->fs->time_label), FALSE);
		break;
	default:
		retval = FALSE;
	}

	return retval;
}

static void
xplayer_action_handle_seek (XplayerObject *xplayer, GdkEventKey *event, gboolean is_forward)
{
	if (is_forward != FALSE) {
		if (event->state & GDK_SHIFT_MASK)
			xplayer_action_seek_relative (xplayer, SEEK_FORWARD_SHORT_OFFSET * 1000, FALSE);
		else if (event->state & GDK_CONTROL_MASK)
			xplayer_action_seek_relative (xplayer, SEEK_FORWARD_LONG_OFFSET * 1000, FALSE);
		else
			xplayer_action_seek_relative (xplayer, SEEK_FORWARD_OFFSET * 1000, FALSE);
	} else {
		if (event->state & GDK_SHIFT_MASK)
			xplayer_action_seek_relative (xplayer, SEEK_BACKWARD_SHORT_OFFSET * 1000, FALSE);
		else if (event->state & GDK_CONTROL_MASK)
			xplayer_action_seek_relative (xplayer, SEEK_BACKWARD_LONG_OFFSET * 1000, FALSE);
		else
			xplayer_action_seek_relative (xplayer, SEEK_BACKWARD_OFFSET * 1000, FALSE);
	}
}

static gboolean
xplayer_action_handle_key_press (XplayerObject *xplayer, GdkEventKey *event)
{
	gboolean retval;
	const char *icon_name;
	gfloat rate;
	char * speed;

	retval = TRUE;
	icon_name = NULL;

	switch (event->keyval) {
	case GDK_KEY_BackSpace:
		bacon_video_widget_set_rate (xplayer->bvw, 1.0);
		bacon_video_widget_show_osd (xplayer->bvw, "preferences-system-time-symbolic", "100%");
		break;
	case GDK_KEY_bracketleft:
		rate = bacon_video_widget_get_rate (xplayer->bvw) - 0.1;
		bacon_video_widget_set_rate (xplayer->bvw, rate);
		speed = g_strdup_printf ("%d%%", (int)(rate * 100));
		bacon_video_widget_show_osd (xplayer->bvw, "preferences-system-time-symbolic", speed);
		g_free (speed);
		break;
	case GDK_KEY_bracketright:
		rate = bacon_video_widget_get_rate (xplayer->bvw) + 0.1;
		bacon_video_widget_set_rate (xplayer->bvw, rate);
		speed = g_strdup_printf ("%d%%", (int)(rate * 100));
		bacon_video_widget_show_osd (xplayer->bvw, "preferences-system-time-symbolic", speed);
		g_free (speed);
		break;
	case GDK_KEY_braceleft:
		rate = bacon_video_widget_get_rate (xplayer->bvw) /2;
		bacon_video_widget_set_rate (xplayer->bvw, rate);
		speed = g_strdup_printf ("%d%%", (int)(rate * 100));
		bacon_video_widget_show_osd (xplayer->bvw, "preferences-system-time-symbolic", speed);
		g_free (speed);
		break;
	case GDK_KEY_braceright:
		rate = bacon_video_widget_get_rate (xplayer->bvw) *2;
		bacon_video_widget_set_rate (xplayer->bvw, rate);
		speed = g_strdup_printf ("%d%%", (int)(rate * 100));
		bacon_video_widget_show_osd (xplayer->bvw, "preferences-system-time-symbolic", speed);
		g_free (speed);
		break;
	case GDK_KEY_A:
	case GDK_KEY_a:
		xplayer_action_toggle_aspect_ratio (xplayer);
		break;
	case GDK_KEY_AudioPrev:
	case GDK_KEY_Back:
	case GDK_KEY_B:
	case GDK_KEY_b:
		xplayer_action_previous (xplayer);
		icon_name = xplayer_get_rtl_icon_name ("media-skip-backward");
		break;
	case GDK_KEY_C:
	case GDK_KEY_c:
		bacon_video_widget_dvd_event (xplayer->bvw,
				BVW_DVD_CHAPTER_MENU);
		break;
	case GDK_KEY_F11:
	case GDK_KEY_f:
	case GDK_KEY_F:
		xplayer_action_fullscreen_toggle (xplayer);
		break;
	case GDK_KEY_g:
	case GDK_KEY_G:
		xplayer_action_next_angle (xplayer);
		break;
	case GDK_KEY_h:
	case GDK_KEY_H:
		xplayer_action_toggle_controls (xplayer);
		break;
	case GDK_KEY_l:
	case GDK_KEY_L:
		xplayer_action_cycle_language (xplayer);
		break;
	case GDK_KEY_M:
	case GDK_KEY_m:
		bacon_video_widget_dvd_event (xplayer->bvw, BVW_DVD_ROOT_MENU);
		break;
	case GDK_KEY_AudioNext:
	case GDK_KEY_Forward:
	case GDK_KEY_N:
	case GDK_KEY_n:
	case GDK_KEY_End:
		xplayer_action_next (xplayer);
		icon_name = xplayer_get_rtl_icon_name ("media-skip-forward");
		break;
	case GDK_KEY_OpenURL:
		xplayer_action_fullscreen (xplayer, FALSE);
		xplayer_action_open_location (xplayer);
		break;
	case GDK_KEY_O:
	case GDK_KEY_o:
	case GDK_KEY_Open:
		xplayer_action_fullscreen (xplayer, FALSE);
		xplayer_action_open (xplayer);
		break;
	case GDK_KEY_AudioPlay:
	case GDK_KEY_p:
	case GDK_KEY_P:
		if (event->state & GDK_CONTROL_MASK) {
			xplayer_action_show_properties (xplayer);
		} else {
			if (bacon_video_widget_is_playing (xplayer->bvw) == FALSE)
				icon_name = xplayer_get_rtl_icon_name ("media-playback-start");
			else
				icon_name = "media-playback-pause-symbolic";
			xplayer_action_play_pause (xplayer);
		}
		break;
	case GDK_KEY_comma:
		xplayer_action_pause (xplayer);
		bacon_video_widget_step (xplayer->bvw, FALSE, NULL);
		break;
	case GDK_KEY_period:
		xplayer_action_pause (xplayer);
		bacon_video_widget_step (xplayer->bvw, TRUE, NULL);
		break;
	case GDK_KEY_AudioPause:
	case GDK_KEY_Pause:
	case GDK_KEY_AudioStop:
		xplayer_action_pause (xplayer);
		icon_name = "media-playback-pause-symbolic";
		break;
	case GDK_KEY_q:
	case GDK_KEY_Q:
		xplayer_action_exit (xplayer);
		break;
	case GDK_KEY_r:
	case GDK_KEY_R:
	case GDK_KEY_ZoomIn:
		xplayer_action_set_zoom (xplayer, TRUE);
		break;
	case GDK_KEY_s:
	case GDK_KEY_S:
		xplayer_action_cycle_subtitle (xplayer);
		break;
	case GDK_KEY_t:
	case GDK_KEY_T:
	case GDK_KEY_ZoomOut:
		xplayer_action_set_zoom (xplayer, FALSE);
		break;
	case GDK_KEY_Eject:
		xplayer_action_eject (xplayer);
		icon_name = "media-eject";
		break;
	case GDK_KEY_Escape:
		if (event->state & GDK_SUPER_MASK)
			bacon_video_widget_dvd_event (xplayer->bvw, BVW_DVD_ROOT_MENU);
		else
			xplayer_action_fullscreen (xplayer, FALSE);
		break;
	case GDK_KEY_space:
	case GDK_KEY_Return:
		if (!(event->state & GDK_CONTROL_MASK)) {
			GtkWidget *focus = gtk_window_get_focus (GTK_WINDOW (xplayer->win));
			if (xplayer_is_fullscreen (xplayer) != FALSE || focus == NULL ||
			    focus == GTK_WIDGET (xplayer->bvw) || focus == xplayer->seek) {
				if (event->keyval == GDK_KEY_space) {
					if (bacon_video_widget_is_playing (xplayer->bvw) == FALSE)
						icon_name = xplayer_get_rtl_icon_name ("media-playback-start");
					else
						icon_name = "media-playback-pause-symbolic";
					xplayer_action_play_pause (xplayer);
				} else if (bacon_video_widget_has_menus (xplayer->bvw) != FALSE) {
					bacon_video_widget_dvd_event (xplayer->bvw, BVW_DVD_ROOT_MENU_SELECT);
				}
			} else
				retval = FALSE;
		}
		break;
	case GDK_KEY_Left:
	case GDK_KEY_Right:
		if (bacon_video_widget_has_menus (xplayer->bvw) == FALSE) {
			gboolean is_forward;

			is_forward = (event->keyval == GDK_KEY_Right);
			/* Switch direction in RTL environment */
			if (gtk_widget_get_direction (xplayer->win) == GTK_TEXT_DIR_RTL)
				is_forward = !is_forward;
			icon_name = xplayer_get_rtl_icon_name (is_forward ? "media-seek-forward" : "media-seek-backward");

			xplayer_action_handle_seek (xplayer, event, is_forward);
		} else {
			if (event->keyval == GDK_KEY_Left)
				bacon_video_widget_dvd_event (xplayer->bvw, BVW_DVD_ROOT_MENU_LEFT);
			else
				bacon_video_widget_dvd_event (xplayer->bvw, BVW_DVD_ROOT_MENU_RIGHT);
		}
		break;
	case GDK_KEY_Home:
		xplayer_action_seek (xplayer, 0);
		icon_name = xplayer_get_rtl_icon_name ("media-seek-backward");
		break;
	case GDK_KEY_Up:
		if (bacon_video_widget_has_menus (xplayer->bvw) != FALSE)
			bacon_video_widget_dvd_event (xplayer->bvw, BVW_DVD_ROOT_MENU_UP);
		else if (event->state & GDK_SHIFT_MASK)
			xplayer_action_volume_relative (xplayer, VOLUME_UP_SHORT_OFFSET);
		else
			xplayer_action_volume_relative (xplayer, VOLUME_UP_OFFSET);
		break;
	case GDK_KEY_Down:
		if (bacon_video_widget_has_menus (xplayer->bvw) != FALSE)
			bacon_video_widget_dvd_event (xplayer->bvw, BVW_DVD_ROOT_MENU_DOWN);
		else if (event->state & GDK_SHIFT_MASK)
			xplayer_action_volume_relative (xplayer, VOLUME_DOWN_SHORT_OFFSET);
		else
			xplayer_action_volume_relative (xplayer, VOLUME_DOWN_OFFSET);
		break;
	case GDK_KEY_0:
		if (event->state & GDK_CONTROL_MASK)
			xplayer_action_set_zoom (xplayer, FALSE);
		else
			xplayer_action_set_scale_ratio (xplayer, 0.5);
		break;
	case GDK_KEY_onehalf:
		xplayer_action_set_scale_ratio (xplayer, 0.5);
		break;
	case GDK_KEY_1:
		xplayer_action_set_scale_ratio (xplayer, 1);
		break;
	case GDK_KEY_2:
		xplayer_action_set_scale_ratio (xplayer, 2);
		break;
	case GDK_KEY_Menu:
		xplayer_action_menu_popup (xplayer, 0);
		break;
	case GDK_KEY_F10:
		if (!(event->state & GDK_SHIFT_MASK))
			return FALSE;

		xplayer_action_menu_popup (xplayer, 0);
		break;
	case GDK_KEY_equal:
		if (event->state & GDK_CONTROL_MASK)
			xplayer_action_set_zoom (xplayer, TRUE);
		break;
	case GDK_KEY_hyphen:
		if (event->state & GDK_CONTROL_MASK)
			xplayer_action_set_zoom (xplayer, FALSE);
		break;
	case GDK_KEY_plus:
	case GDK_KEY_KP_Add:
		if (!(event->state & GDK_CONTROL_MASK)) {
			xplayer_action_next (xplayer);
		} else {
			xplayer_action_set_zoom (xplayer, TRUE);
		}
		break;
	case GDK_KEY_minus:
	case GDK_KEY_KP_Subtract:
		if (!(event->state & GDK_CONTROL_MASK)) {
			xplayer_action_previous (xplayer);
		} else {
			xplayer_action_set_zoom (xplayer, FALSE);
		}
		break;
	case GDK_KEY_KP_Up:
	case GDK_KEY_KP_8:
		bacon_video_widget_dvd_event (xplayer->bvw, 
					      BVW_DVD_ROOT_MENU_UP);
		break;
	case GDK_KEY_KP_Down:
	case GDK_KEY_KP_2:
		bacon_video_widget_dvd_event (xplayer->bvw, 
					      BVW_DVD_ROOT_MENU_DOWN);
		break;
	case GDK_KEY_KP_Right:
	case GDK_KEY_KP_6:
		bacon_video_widget_dvd_event (xplayer->bvw, 
					      BVW_DVD_ROOT_MENU_RIGHT);
		break;
	case GDK_KEY_KP_Left:
	case GDK_KEY_KP_4:
		bacon_video_widget_dvd_event (xplayer->bvw, 
					      BVW_DVD_ROOT_MENU_LEFT);
		break;
	case GDK_KEY_KP_Begin:
	case GDK_KEY_KP_5:
		bacon_video_widget_dvd_event (xplayer->bvw,
					      BVW_DVD_ROOT_MENU_SELECT);
	default:
		retval = FALSE;
	}

	if (icon_name != NULL)
		xplayer_fullscreen_show_popups_or_osd (xplayer->fs,
						     icon_name,
						     FALSE);

	return retval;
}

static gboolean
xplayer_action_handle_scroll (XplayerObject    *xplayer,
			    const GdkEvent *event)
{
	gboolean retval = TRUE;
	GdkEventScroll *sevent = (GdkEventScroll *) event;
	GdkScrollDirection direction;

	direction = sevent->direction;

	if (xplayer_fullscreen_is_fullscreen (xplayer->fs) != FALSE)
		xplayer_fullscreen_show_popups (xplayer->fs, TRUE);


    switch (direction) {
    case GDK_SCROLL_UP:
        xplayer_action_seek_relative (xplayer, xplayer->stream_length / 500, FALSE);
        break;
    case GDK_SCROLL_DOWN:
        xplayer_action_seek_relative (xplayer, -xplayer->stream_length / 500, FALSE);
        break;
    default:
        retval = FALSE;
    }

	return retval;
}

gboolean
window_key_press_event_cb (GtkWidget *win, GdkEventKey *event, XplayerObject *xplayer)
{
	gboolean sidebar_handles_kbd;

	/* Shortcuts disabled? */
	if (xplayer->disable_kbd_shortcuts != FALSE)
		return FALSE;

	/* Check whether the sidebar needs the key events */
	if (event->type == GDK_KEY_PRESS) {
		if (xplayer_sidebar_is_focused (xplayer, &sidebar_handles_kbd) != FALSE) {
			/* Make Escape pass the focus to the video widget */
			if (sidebar_handles_kbd == FALSE &&
			    event->keyval == GDK_KEY_Escape)
				gtk_widget_grab_focus (GTK_WIDGET (xplayer->bvw));
			return FALSE;
		}
	} else {
		if (xplayer_sidebar_is_focused (xplayer, NULL) != FALSE)
			return FALSE;
	}

	/* Special case Eject, Open, Open URI,
	 * seeking and zoom keyboard shortcuts */
	if (event->state != 0
			&& (event->state & GDK_CONTROL_MASK))
	{
		switch (event->keyval) {
		case GDK_KEY_E:
		case GDK_KEY_e:
		case GDK_KEY_O:
		case GDK_KEY_o:
		case GDK_KEY_L:
		case GDK_KEY_l:
		case GDK_KEY_q:
		case GDK_KEY_Q:
		case GDK_KEY_Right:
		case GDK_KEY_Left:
		case GDK_KEY_plus:
		case GDK_KEY_KP_Add:
		case GDK_KEY_minus:
		case GDK_KEY_KP_Subtract:
		case GDK_KEY_0:
		case GDK_KEY_equal:
		case GDK_KEY_hyphen:
			if (event->type == GDK_KEY_PRESS)
				return xplayer_action_handle_key_press (xplayer, event);
			else
				return xplayer_action_handle_key_release (xplayer, event);
		default:
			break;
		}
	}

	if (event->state != 0 && (event->state & GDK_SUPER_MASK)) {
		switch (event->keyval) {
		case GDK_KEY_Escape:
			if (event->type == GDK_KEY_PRESS)
				return xplayer_action_handle_key_press (xplayer, event);
			else
				return xplayer_action_handle_key_release (xplayer, event);
		default:
			break;
		}
	}


	/* If we have modifiers, and either Ctrl, Mod1 (Alt), or any
	 * of Mod3 to Mod5 (Mod2 is num-lock...) are pressed, we
	 * let Gtk+ handle the key */
	if (event->state != 0
			&& ((event->state & GDK_CONTROL_MASK)
			|| (event->state & GDK_MOD1_MASK)
			|| (event->state & GDK_MOD3_MASK)
			|| (event->state & GDK_MOD4_MASK)))
		return FALSE;

	if (event->type == GDK_KEY_PRESS) {
		return xplayer_action_handle_key_press (xplayer, event);
	} else {
		return xplayer_action_handle_key_release (xplayer, event);
	}
}

gboolean
window_scroll_event_cb (GtkWidget *win, GdkEvent *event, XplayerObject *xplayer)
{
	return xplayer_action_handle_scroll (xplayer, event);
}

static void
update_media_menu_items (XplayerObject *xplayer)
{
	GMount *mount;
	gboolean playing;

	playing = xplayer_playing_dvd (xplayer->mrl);

	xplayer_action_set_sensitivity ("dvd-root-menu", playing);
	xplayer_action_set_sensitivity ("dvd-title-menu", playing);
	xplayer_action_set_sensitivity ("dvd-audio-menu", playing);
	xplayer_action_set_sensitivity ("dvd-angle-menu", playing);
	xplayer_action_set_sensitivity ("dvd-chapter-menu", playing);

	xplayer_action_set_sensitivity ("next-angle",
				      bacon_video_widget_has_angles (xplayer->bvw));

	mount = xplayer_get_mount_for_media (xplayer->mrl);
	xplayer_action_set_sensitivity ("eject", mount != NULL);
	if (mount != NULL)
		g_object_unref (mount);
}

static void
update_buttons (XplayerObject *xplayer)
{
	gboolean has_item;

	/* Previous */
	has_item = bacon_video_widget_has_previous_track (xplayer->bvw) ||
		xplayer_playlist_has_previous_mrl (xplayer->playlist) ||
		xplayer_playlist_get_repeat (xplayer->playlist);
	xplayer_action_set_sensitivity ("previous-chapter", has_item);

	/* Next */
	has_item = bacon_video_widget_has_next_track (xplayer->bvw) ||
		xplayer_playlist_has_next_mrl (xplayer->playlist) ||
		xplayer_playlist_get_repeat (xplayer->playlist);
	xplayer_action_set_sensitivity ("next-chapter", has_item);
}

void
main_pane_size_allocated (GtkWidget *main_pane, GtkAllocation *allocation, XplayerObject *xplayer)
{
	gulong handler_id;

	if (!xplayer->maximised || gtk_widget_get_mapped (xplayer->win)) {
		handler_id = g_signal_handler_find (main_pane, 
				G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
				0, 0, NULL,
				main_pane_size_allocated, xplayer);
		g_signal_handler_disconnect (main_pane, handler_id);

		gtk_paned_set_position (GTK_PANED (main_pane), allocation->width - xplayer->sidebar_w);
	}
}

char *
xplayer_setup_window (XplayerObject *xplayer)
{
	GKeyFile *keyfile;
	int w, h;
	gboolean show_sidebar;
	char *filename, *page_id;
	GError *err = NULL;
	GtkWidget *vbox;
	GdkRGBA black;

	filename = g_build_filename (xplayer_dot_dir (), "state.ini", NULL);
	keyfile = g_key_file_new ();
	if (g_key_file_load_from_file (keyfile, filename,
			G_KEY_FILE_NONE, NULL) == FALSE) {
		xplayer->sidebar_w = 0;
		w = DEFAULT_WINDOW_W;
		h = DEFAULT_WINDOW_H;
		show_sidebar = FALSE;
		page_id = NULL;
		g_free (filename);
	} else {
		g_free (filename);

		w = g_key_file_get_integer (keyfile, "State", "window_w", &err);
		if (err != NULL) {
			w = 0;
			g_error_free (err);
			err = NULL;
		}

		h = g_key_file_get_integer (keyfile, "State", "window_h", &err);
		if (err != NULL) {
			h = 0;
			g_error_free (err);
			err = NULL;
		}

		show_sidebar = g_key_file_get_boolean (keyfile, "State",
				"show_sidebar", &err);
		if (err != NULL) {
			show_sidebar = TRUE;
			g_error_free (err);
			err = NULL;
		}

		xplayer->maximised = g_key_file_get_boolean (keyfile, "State",
				"maximised", &err);
		if (err != NULL) {
			g_error_free (err);
			err = NULL;
		}

		page_id = g_key_file_get_string (keyfile, "State",
				"sidebar_page", &err);
		if (err != NULL) {
			g_error_free (err);
			page_id = NULL;
			err = NULL;
		}

		xplayer->sidebar_w = g_key_file_get_integer (keyfile, "State",
				"sidebar_w", &err);
		if (err != NULL) {
			g_error_free (err);
			xplayer->sidebar_w = 0;
		}
		g_key_file_free (keyfile);
	}

	if (w > 0 && h > 0 && xplayer->maximised == FALSE) {
		gtk_window_set_default_size (GTK_WINDOW (xplayer->win),
				w, h);
		xplayer->window_w = w;
		xplayer->window_h = h;
	} else if (xplayer->maximised != FALSE) {
		gtk_window_maximize (GTK_WINDOW (xplayer->win));
	}

	/* Set the vbox to be completely black */
	vbox = GTK_WIDGET (gtk_builder_get_object (xplayer->xml, "tmw_bvw_box"));
	gdk_rgba_parse (&black, "Black");
	gtk_widget_override_background_color (vbox, (GTK_STATE_FLAG_FOCUSED << 1), &black);

	xplayer_sidebar_setup (xplayer, show_sidebar);
	return page_id;
}

void
xplayer_callback_connect (XplayerObject *xplayer)
{
	GtkWidget *item, *image, *label;
	GIcon *icon;
	GtkAction *action;
	GtkActionGroup *action_group;
	GtkBox *box;

	/* Menu items */
	action = gtk_action_group_get_action (xplayer->main_action_group, "repeat-mode");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
		xplayer_playlist_get_repeat (xplayer->playlist));
	action = gtk_action_group_get_action (xplayer->main_action_group, "shuffle-mode");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
		xplayer_playlist_get_shuffle (xplayer->playlist));

	/* Controls */
	box = GTK_BOX (gtk_builder_get_object (xplayer->xml, "tmw_buttons_hbox"));

	/* Previous */
	action = gtk_action_group_get_action (xplayer->main_action_group,
			"previous-chapter");
	item = gtk_action_create_tool_item (action);
	gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (item), 
					_("Previous Chapter/Movie"));
	atk_object_set_name (gtk_widget_get_accessible (item),
			_("Previous Chapter/Movie"));
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Play/Pause */
	action = gtk_action_group_get_action (xplayer->main_action_group, "play");
	item = gtk_action_create_tool_item (action);
	atk_object_set_name (gtk_widget_get_accessible (item),
			_("Play / Pause"));
	gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (item),
 					_("Play / Pause"));
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Next */
	action = gtk_action_group_get_action (xplayer->main_action_group,
			"next-chapter");
	item = gtk_action_create_tool_item (action);
	gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (item), 
					_("Next Chapter/Movie"));
	atk_object_set_name (gtk_widget_get_accessible (item),
			_("Next Chapter/Movie"));
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Separator */
	item = GTK_WIDGET(gtk_separator_tool_item_new ());
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Fullscreen button */
	action = gtk_action_group_get_action (xplayer->main_action_group,
			"fullscreen");
	item = gtk_action_create_tool_item (action);
	/* Translators: this is the tooltip text for the fullscreen button in the controls box in Xplayer's main window. */
	gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (item), _("Fullscreen"));
	/* Translators: this is the accessibility text for the fullscreen button in the controls box in Xplayer's main window. */
	atk_object_set_name (gtk_widget_get_accessible (item), _("Fullscreen"));
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Sidebar button (Drag'n'Drop) */
	box = GTK_BOX (gtk_builder_get_object (xplayer->xml, "tmw_sidebar_button_hbox"));
	action = gtk_action_group_get_action (xplayer->main_action_group, "sidebar");
	item = gtk_toggle_button_new ();
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (item), action);

	/* Remove the label */
	label = gtk_bin_get_child (GTK_BIN (item));
	gtk_widget_destroy (label);

	/* Force add an icon, so it doesn't follow the
	 * gtk-button-images setting */
	icon = g_themed_icon_new_with_default_fallbacks ("xplayer-view-sidebar-symbolic");
	image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (image);
	gtk_container_add (GTK_CONTAINER (item), image);
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (item), "drag_data_received",
			G_CALLBACK (drop_playlist_cb), xplayer);
	g_signal_connect (G_OBJECT (item), "drag_motion",
			G_CALLBACK (drag_motion_playlist_cb), xplayer);
	gtk_drag_dest_set (item, GTK_DEST_DEFAULT_ALL,
			target_table, G_N_ELEMENTS (target_table),
			GDK_ACTION_COPY | GDK_ACTION_MOVE);

	/* Fullscreen window buttons */
	g_signal_connect (G_OBJECT (xplayer->fs->exit_button), "clicked",
			  G_CALLBACK (fs_exit1_activate_cb), xplayer);

	g_signal_connect (G_OBJECT (xplayer->fs->blank_button), "toggled",
			  G_CALLBACK (fs_blank1_activate_cb), xplayer);

	action = gtk_action_group_get_action (xplayer->main_action_group, "play");
	item = gtk_action_create_tool_item (action);
	gtk_box_pack_start (GTK_BOX (xplayer->fs->buttons_box), item, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (item), "clicked",
			G_CALLBACK (on_mouse_click_fullscreen), xplayer);

	action = gtk_action_group_get_action (xplayer->main_action_group, "previous-chapter");
	item = gtk_action_create_tool_item (action);
	gtk_box_pack_start (GTK_BOX (xplayer->fs->buttons_box), item, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (item), "clicked",
			G_CALLBACK (on_mouse_click_fullscreen), xplayer);

	action = gtk_action_group_get_action (xplayer->main_action_group, "next-chapter");
	item = gtk_action_create_tool_item (action);
	gtk_box_pack_start (GTK_BOX (xplayer->fs->buttons_box), item, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (item), "clicked",
			G_CALLBACK (on_mouse_click_fullscreen), xplayer);

	/* Connect the keys */
	gtk_widget_add_events (xplayer->win, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

	/* Connect the mouse wheel */
	gtk_widget_add_events (GTK_WIDGET (gtk_builder_get_object (xplayer->xml, "tmw_main_vbox")), GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);
	gtk_widget_add_events (xplayer->seek, GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);
	gtk_widget_add_events (xplayer->fs->seek, GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);

	/* FIXME Hack to fix bug #462286 and #563894 */
	g_signal_connect (G_OBJECT (xplayer->fs->seek), "button-press-event",
			G_CALLBACK (seek_slider_pressed_cb), xplayer);
	g_signal_connect (G_OBJECT (xplayer->fs->seek), "button-release-event",
			G_CALLBACK (seek_slider_released_cb), xplayer);
	g_signal_connect (G_OBJECT (xplayer->fs->seek), "scroll-event",
			  G_CALLBACK (window_scroll_event_cb), xplayer);


	/* Set sensitivity of the toolbar buttons */
	xplayer_action_set_sensitivity ("play", FALSE);
	xplayer_action_set_sensitivity ("next-chapter", FALSE);
	xplayer_action_set_sensitivity ("previous-chapter", FALSE);
	/* FIXME: We can use this code again once bug #457631 is fixed
	 * and skip-* are back in the main action group. */
	/*xplayer_action_set_sensitivity ("skip-forward", FALSE);
	xplayer_action_set_sensitivity ("skip-backwards", FALSE);*/
	xplayer_action_set_sensitivity ("fullscreen", FALSE);

	action_group = GTK_ACTION_GROUP (gtk_builder_get_object (xplayer->xml, "skip-action-group"));

	action = gtk_action_group_get_action (action_group, "skip-forward");
	gtk_action_set_sensitive (action, FALSE);

	action = gtk_action_group_get_action (action_group, "skip-backwards");
	gtk_action_set_sensitive (action, FALSE);
}

void
playlist_widget_setup (XplayerObject *xplayer)
{
	xplayer->playlist = XPLAYER_PLAYLIST (xplayer_playlist_new ());

	if (xplayer->playlist == NULL)
		xplayer_action_exit (xplayer);

	gtk_widget_show_all (GTK_WIDGET (xplayer->playlist));

	g_signal_connect (G_OBJECT (xplayer->playlist), "active-name-changed",
			  G_CALLBACK (on_playlist_change_name), xplayer);
	g_signal_connect (G_OBJECT (xplayer->playlist), "item-activated",
			  G_CALLBACK (item_activated_cb), xplayer);
	g_signal_connect (G_OBJECT (xplayer->playlist),
			  "changed", G_CALLBACK (playlist_changed_cb),
			  xplayer);
	g_signal_connect (G_OBJECT (xplayer->playlist),
			  "current-removed", G_CALLBACK (current_removed_cb),
			  xplayer);
	g_signal_connect (G_OBJECT (xplayer->playlist),
			  "repeat-toggled",
			  G_CALLBACK (playlist_repeat_toggle_cb),
			  xplayer);
	g_signal_connect (G_OBJECT (xplayer->playlist),
			  "shuffle-toggled",
			  G_CALLBACK (playlist_shuffle_toggle_cb),
			  xplayer);
	g_signal_connect (G_OBJECT (xplayer->playlist),
			  "subtitle-changed",
			  G_CALLBACK (subtitle_changed_cb),
			  xplayer);
}

void
video_widget_create (XplayerObject *xplayer)
{
	GError *err = NULL;
	GtkContainer *container;
	BaconVideoWidget **bvw;

	xplayer->bvw = BACON_VIDEO_WIDGET (bacon_video_widget_new (&err));

	if (xplayer->bvw == NULL) {
		xplayer_action_error_and_exit (_("Xplayer could not startup."), err != NULL ? err->message : _("No reason."), xplayer);
		if (err != NULL)
			g_error_free (err);
	}

	g_signal_connect_after (G_OBJECT (xplayer->bvw),
			"button-press-event",
			G_CALLBACK (on_video_button_press_event),
			xplayer);
	g_signal_connect (G_OBJECT (xplayer->bvw),
			"eos",
			G_CALLBACK (on_eos_event),
			xplayer);
	g_signal_connect (G_OBJECT (xplayer->bvw),
			"got-redirect",
			G_CALLBACK (on_got_redirect),
			xplayer);
	g_signal_connect (G_OBJECT(xplayer->bvw),
			"channels-change",
			G_CALLBACK (on_channels_change_event),
			xplayer);
	g_signal_connect (G_OBJECT (xplayer->bvw),
			"tick",
			G_CALLBACK (update_current_time),
			xplayer);
	g_signal_connect (G_OBJECT (xplayer->bvw),
			"got-metadata",
			G_CALLBACK (on_got_metadata_event),
			xplayer);
	g_signal_connect (G_OBJECT (xplayer->bvw),
			"buffering",
			G_CALLBACK (on_buffering_event),
			xplayer);
	g_signal_connect (G_OBJECT (xplayer->bvw),
			"download-buffering",
			G_CALLBACK (on_download_buffering_event),
			xplayer);
	g_signal_connect (G_OBJECT (xplayer->bvw),
			"error",
			G_CALLBACK (on_error_event),
			xplayer);

	container = GTK_CONTAINER (gtk_builder_get_object (xplayer->xml, "tmw_bvw_box"));
	gtk_container_add (container,
			GTK_WIDGET (xplayer->bvw));

	/* Events for the widget video window as well */
	gtk_widget_add_events (GTK_WIDGET (xplayer->bvw),
			GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
	g_signal_connect (G_OBJECT(xplayer->bvw), "key_press_event",
			G_CALLBACK (window_key_press_event_cb), xplayer);
	g_signal_connect (G_OBJECT(xplayer->bvw), "key_release_event",
			G_CALLBACK (window_key_press_event_cb), xplayer);

	g_signal_connect (G_OBJECT (xplayer->bvw), "drag_data_received",
			G_CALLBACK (drop_video_cb), xplayer);
	g_signal_connect (G_OBJECT (xplayer->bvw), "drag_motion",
			G_CALLBACK (drag_motion_video_cb), xplayer);
	gtk_drag_dest_set (GTK_WIDGET (xplayer->bvw), GTK_DEST_DEFAULT_ALL,
			target_table, G_N_ELEMENTS (target_table),
			GDK_ACTION_COPY | GDK_ACTION_MOVE);

	g_signal_connect (G_OBJECT (xplayer->bvw), "drag_data_get",
			G_CALLBACK (drag_video_cb), xplayer);

	bvw = &(xplayer->bvw);
	g_object_add_weak_pointer (G_OBJECT (xplayer->bvw),
				   (gpointer *) bvw);

	gtk_widget_realize (GTK_WIDGET (xplayer->bvw));
	gtk_widget_show (GTK_WIDGET (xplayer->bvw));

	xplayer_preferences_visuals_setup (xplayer);

	g_signal_connect (G_OBJECT (xplayer->bvw), "notify::volume",
			G_CALLBACK (property_notify_cb_volume), xplayer);
	g_signal_connect (G_OBJECT (xplayer->bvw), "notify::seekable",
			G_CALLBACK (property_notify_cb_seekable), xplayer);
	update_volume_sliders (xplayer);
}

/**
 * xplayer_object_get_supported_content_types:
 *
 * Get the full list of file content types which Xplayer supports playing.
 *
 * Return value: (array zero-terminated=1) (transfer none): a %NULL-terminated array of the content types Xplayer supports
 * Since: 3.1.5
 */
const gchar * const *
xplayer_object_get_supported_content_types (void)
{
	return mime_types;
}

/**
 * xplayer_object_get_supported_uri_schemes:
 *
 * Get the full list of URI schemes which Xplayer supports accessing.
 *
 * Return value: (array zero-terminated=1) (transfer none): a %NULL-terminated array of the URI schemes Xplayer supports
 * Since: 3.1.5
 */
const gchar * const *
xplayer_object_get_supported_uri_schemes (void)
{
	return uri_schemes;
}
