/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* xplayer-playlist.c

   Copyright (C) 2002, 2003, 2004, 2005 Bastien Nocera

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Author: Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"
#include "xplayer-playlist.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gio.h>
#include <string.h>

#include "eggfileformatchooser.h"
#include "xplayer-dnd-menu.h"
#include "xplayer-uri.h"
#include "xplayer-interface.h"
#include "xplayer-rtl-helpers.h"
#include "video-utils.h"

#define PL_LEN (gtk_tree_model_iter_n_children (playlist->priv->model, NULL))

static void ensure_shuffled (XplayerPlaylist *playlist);
static gboolean xplayer_playlist_add_one_mrl (XplayerPlaylist *playlist,
					    const char    *mrl,
					    const char    *display_name,
					    const char    *content_type);

typedef gboolean (*ClearComparisonFunc) (XplayerPlaylist *playlist, GtkTreeIter *iter, gconstpointer data);

static void xplayer_playlist_clear_with_compare (XplayerPlaylist *playlist,
					       ClearComparisonFunc func,
					       gconstpointer data);

/* Callback function for GtkBuilder */
G_MODULE_EXPORT void xplayer_playlist_save_files (GtkWidget *widget, XplayerPlaylist *playlist);
G_MODULE_EXPORT void xplayer_playlist_add_files (GtkWidget *widget, XplayerPlaylist *playlist);
G_MODULE_EXPORT void playlist_remove_button_clicked (GtkWidget *button, XplayerPlaylist *playlist);
G_MODULE_EXPORT void xplayer_playlist_up_files (GtkWidget *widget, XplayerPlaylist *playlist);
G_MODULE_EXPORT void xplayer_playlist_down_files (GtkWidget *widget, XplayerPlaylist *playlist);
G_MODULE_EXPORT void playlist_copy_location_action_callback (GtkAction *action, XplayerPlaylist *playlist);
G_MODULE_EXPORT void playlist_select_subtitle_action_callback (GtkAction *action, XplayerPlaylist *playlist);
G_MODULE_EXPORT void playlist_remove_action_callback (GtkAction *action, XplayerPlaylist *playlist);


typedef struct {
	XplayerPlaylist *playlist;
	XplayerPlaylistForeachFunc callback;
	gpointer user_data;
} PlaylistForeachContext;

struct XplayerPlaylistPrivate
{
	GtkWidget *treeview;
	GtkTreeModel *model;
	GtkTreePath *current;
	GtkTreeSelection *selection;
	XplayerPlParser *parser;

	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;

	/* Widgets */
	GtkWidget *save_button;
	GtkWidget *remove_button;
	GtkWidget *up_button;
	GtkWidget *down_button;
	GtkWidget *toolbar;

	/* These is the current paths for the file selectors */
	char *path;
	char *save_path;
	guint save_format;
	GtkWidget *file_chooser;

	/* Shuffle mode */
	int *shuffled;
	int current_shuffled, shuffle_len;

	GSettings *settings;
	GSettings *lockdown_settings;

	/* Used to know the position for drops */
	GtkTreePath *tree_path;
	GtkTreeViewDropPosition drop_pos;

	/* Cursor ref: 0 if the cursor is unbusy; positive numbers indicate the number of nested calls to set_waiting_cursor() */
	guint cursor_ref;

	/* This is a scratch list for when we're removing files */
	GList *list;
	guint current_to_be_removed : 1;

	guint disable_save_to_disk : 1;

	/* Repeat mode */
	guint repeat : 1;

	/* Reorder Flag */
	guint drag_started : 1;

	/* Drop disabled flag */
	guint drop_disabled : 1;

	/* Shuffle mode */
	guint shuffle : 1;
};

/* Signals */
enum {
	CHANGED,
	ITEM_ACTIVATED,
	ACTIVE_NAME_CHANGED,
	CURRENT_REMOVED,
	REPEAT_TOGGLED,
	SHUFFLE_TOGGLED,
	SUBTITLE_CHANGED,
	ITEM_ADDED,
	ITEM_REMOVED,
	LAST_SIGNAL
};

enum {
	PLAYING_COL,
	FILENAME_COL,
	FILENAME_ESCAPED_COL,
	URI_COL,
	TITLE_CUSTOM_COL,
	SUBTITLE_URI_COL,
	FILE_MONITOR_COL,
	MOUNT_COL,
	MIME_TYPE_COL,
	NUM_COLS
};

typedef struct {
	const char *name;
	const char *suffix;
	XplayerPlParserType type;
} PlaylistSaveType;

static const PlaylistSaveType save_types [] = {
	{ NULL, NULL, -1 }, /* By extension entry */
	{ N_("MP3 ShoutCast playlist"), "pls", XPLAYER_PL_PARSER_PLS },
	{ N_("MP3 audio (streamed)"), "m3u", XPLAYER_PL_PARSER_M3U },
	{ N_("MP3 audio (streamed, DOS format)"), "m3u", XPLAYER_PL_PARSER_M3U_DOS },
	{ N_("XML Shareable Playlist"), "xspf", XPLAYER_PL_PARSER_XSPF }
};

static int xplayer_playlist_table_signals[LAST_SIGNAL];

/* casts are to shut gcc up */
static const GtkTargetEntry target_table[] = {
	{ (gchar*) "text/uri-list", 0, 0 },
	{ (gchar*) "_NETSCAPE_URL", 0, 1 }
};

static void init_treeview (GtkWidget *treeview, XplayerPlaylist *playlist);

#define xplayer_playlist_unset_playing(x) xplayer_playlist_set_playing(x, XPLAYER_PLAYLIST_STATUS_NONE)

G_DEFINE_TYPE (XplayerPlaylist, xplayer_playlist, GTK_TYPE_BOX)

/* Helper functions */
static gboolean
xplayer_playlist_gtk_tree_path_equals (GtkTreePath *path1, GtkTreePath *path2)
{
	char *str1, *str2;
	gboolean retval;

	if (path1 == NULL && path2 == NULL)
		return TRUE;
	if (path1 == NULL || path2 == NULL)
		return FALSE;

	str1 = gtk_tree_path_to_string (path1);
	str2 = gtk_tree_path_to_string (path2);

	if (strcmp (str1, str2) == 0)
		retval = TRUE;
	else
		retval = FALSE;

	g_free (str1);
	g_free (str2);

	return retval;
}

static GtkWindow *
xplayer_playlist_get_toplevel (XplayerPlaylist *playlist)
{
	return GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (playlist)));
}

static void
set_waiting_cursor (XplayerPlaylist *playlist)
{
	xplayer_gdk_window_set_waiting_cursor (gtk_widget_get_window (GTK_WIDGET (xplayer_playlist_get_toplevel (playlist))));
	playlist->priv->cursor_ref++;
}

static void
unset_waiting_cursor (XplayerPlaylist *playlist)
{
	if (--playlist->priv->cursor_ref < 1)
		gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (xplayer_playlist_get_toplevel (playlist))), NULL);
}

static void
xplayer_playlist_error (char *title, char *reason, XplayerPlaylist *playlist)
{
	GtkWidget *error_dialog;

	error_dialog =
		gtk_message_dialog_new (xplayer_playlist_get_toplevel (playlist),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				"%s", title);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (error_dialog),
						  "%s", reason);

	gtk_container_set_border_width (GTK_CONTAINER (error_dialog), 5);
	gtk_dialog_set_default_response (GTK_DIALOG (error_dialog),
			GTK_RESPONSE_OK);
	g_signal_connect (G_OBJECT (error_dialog), "destroy", G_CALLBACK
			(gtk_widget_destroy), error_dialog);
	g_signal_connect (G_OBJECT (error_dialog), "response", G_CALLBACK
			(gtk_widget_destroy), error_dialog);
	gtk_window_set_modal (GTK_WINDOW (error_dialog), TRUE);

	gtk_widget_show (error_dialog);
}

void
xplayer_playlist_select_subtitle_dialog(XplayerPlaylist *playlist, XplayerPlaylistSelectDialog mode)
{
	char *subtitle, *current, *uri;
	GFile *file, *dir;
	XplayerPlaylistStatus playing;
	GtkTreeIter iter;

	if (mode == XPLAYER_PLAYLIST_DIALOG_PLAYING) {
		/* Set subtitle file for the currently playing movie */
		gtk_tree_model_get_iter (playlist->priv->model, &iter, playlist->priv->current);
	} else if (mode == XPLAYER_PLAYLIST_DIALOG_SELECTED) {
		/* Set subtitle file in for the first selected playlist item */
		GList *l;

		l = gtk_tree_selection_get_selected_rows (playlist->priv->selection, NULL);
		gtk_tree_model_get_iter (playlist->priv->model, &iter, l->data);
		g_list_foreach (l, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (l);
	} else {
		g_assert_not_reached ();
	}

	/* Look for the directory of the current movie */
	gtk_tree_model_get (playlist->priv->model, &iter,
			    URI_COL, &current,
			    -1);

	if (current == NULL)
		return;

	uri = NULL;
	file = g_file_new_for_uri (current);
	dir = g_file_get_parent (file);
	g_object_unref (file);
	if (dir != NULL) {
		uri = g_file_get_uri (dir);
		g_object_unref (dir);
	}

	subtitle = xplayer_add_subtitle (xplayer_playlist_get_toplevel (playlist), uri);
	g_free (uri);

	if (subtitle == NULL)
		return;

	gtk_tree_model_get (playlist->priv->model, &iter,
			    PLAYING_COL, &playing,
			    -1);

	gtk_list_store_set (GTK_LIST_STORE(playlist->priv->model), &iter,
			    SUBTITLE_URI_COL, subtitle,
			    -1);

	if (playing != XPLAYER_PLAYLIST_STATUS_NONE) {
		g_signal_emit (G_OBJECT (playlist),
			       xplayer_playlist_table_signals[SUBTITLE_CHANGED], 0,
			       NULL);
	}

	g_free(subtitle);
}

void
xplayer_playlist_set_current_subtitle (XplayerPlaylist *playlist, const char *subtitle_uri)
{
	GtkTreeIter iter;

	if (playlist->priv->current == NULL)
		return;

	gtk_tree_model_get_iter (playlist->priv->model, &iter, playlist->priv->current);

	gtk_list_store_set (GTK_LIST_STORE(playlist->priv->model), &iter,
			    SUBTITLE_URI_COL, subtitle_uri,
			    -1);

	g_signal_emit (G_OBJECT (playlist),
		       xplayer_playlist_table_signals[SUBTITLE_CHANGED], 0,
		       NULL);
}

/* This one returns a new string, in UTF8 even if the MRL is encoded
 * in the locale's encoding
 */
static char *
xplayer_playlist_mrl_to_title (const gchar *mrl)
{
	GFile *file;
	char *filename_for_display, *unescaped;

	if (g_str_has_prefix (mrl, "dvd://") != FALSE) {
		/* This is "Title 3", where title is a DVD title
		 * Note: NOT a DVD chapter */
		return g_strdup_printf (_("Title %d"), (int) g_strtod (mrl + 6, NULL));
	} else if (g_str_has_prefix (mrl, "dvb://") != FALSE) {
		/* This is "BBC ONE(BBC)" for "dvb://BBC ONE(BBC)" */
		return g_strdup (mrl + 6);
	}

	file = g_file_new_for_uri (mrl);
	unescaped = g_file_get_basename (file);
	g_object_unref (file);

	filename_for_display = g_filename_to_utf8 (unescaped,
			-1,             /* length */
			NULL,           /* bytes_read */
			NULL,           /* bytes_written */
			NULL);          /* error */

	if (filename_for_display == NULL)
	{
		filename_for_display = g_locale_to_utf8 (unescaped,
				-1, NULL, NULL, NULL);
		if (filename_for_display == NULL) {
			filename_for_display = g_filename_display_name
				(unescaped);
		}
		g_free (unescaped);
		return filename_for_display;
	}

	g_free (unescaped);

	return filename_for_display;
}

static void
xplayer_playlist_update_save_button (XplayerPlaylist *playlist)
{
	gboolean state;

	state = (!playlist->priv->disable_save_to_disk) && (PL_LEN != 0);
	gtk_widget_set_sensitive (playlist->priv->save_button, state);
}

static gboolean
xplayer_playlist_save_iter_foreach (GtkTreeModel *model,
				  GtkTreePath  *path,
				  GtkTreeIter  *iter,
				  gpointer      user_data)
{
	XplayerPlPlaylist *playlist = user_data;
	XplayerPlPlaylistIter pl_iter;
	gchar *uri, *title;
	gboolean custom_title;

	gtk_tree_model_get (model, iter,
			    URI_COL, &uri,
			    FILENAME_COL, &title,
			    TITLE_CUSTOM_COL, &custom_title,
			    -1);

	xplayer_pl_playlist_append (playlist, &pl_iter);
	xplayer_pl_playlist_set (playlist, &pl_iter,
			       XPLAYER_PL_PARSER_FIELD_URI, uri,
			       XPLAYER_PL_PARSER_FIELD_TITLE, (custom_title) ? title : NULL,
			       NULL);

	g_free (uri);
	g_free (title);

	return FALSE;
}

void
xplayer_playlist_save_current_playlist (XplayerPlaylist *playlist, const char *output)
{
	xplayer_playlist_save_current_playlist_ext (playlist, output, XPLAYER_PL_PARSER_PLS);
}

void
xplayer_playlist_save_current_playlist_ext (XplayerPlaylist *playlist, const char *output, XplayerPlParserType type)
{
	XplayerPlPlaylist *pl_playlist;
	GError *error = NULL;
	GFile *output_file;
	gboolean retval;

	pl_playlist = xplayer_pl_playlist_new ();
	output_file = g_file_new_for_commandline_arg (output);

	gtk_tree_model_foreach (playlist->priv->model,
				xplayer_playlist_save_iter_foreach,
				pl_playlist);

	retval = xplayer_pl_parser_save (playlist->priv->parser,
				       pl_playlist,
				       output_file,
				       NULL, type, &error);

	if (retval == FALSE)
	{
		xplayer_playlist_error (_("Could not save the playlist"),
				error->message, playlist);
		g_error_free (error);
	}

	g_object_unref (pl_playlist);
	g_object_unref (output_file);
}

static void
gtk_tree_selection_has_selected_foreach (GtkTreeModel *model,
		GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	int *retval = (gboolean *)user_data;
	*retval = TRUE;
}

static gboolean
gtk_tree_selection_has_selected (GtkTreeSelection *selection)
{
	int retval, *boolean;

	retval = FALSE;
	boolean = &retval;
	gtk_tree_selection_selected_foreach (selection,
			gtk_tree_selection_has_selected_foreach,
			(gpointer) (boolean));

	return retval;
}

static void
drop_finished_cb (XplayerPlaylist *playlist, GAsyncResult *result, gpointer user_data)
{
	xplayer_playlist_add_mrls_finish (playlist, result, NULL);

	g_clear_pointer (&playlist->priv->tree_path, gtk_tree_path_free);

	/* Emit the "changed" signal once the last dropped MRL has been added to the playlist */
	g_signal_emit (G_OBJECT (playlist),
	               xplayer_playlist_table_signals[CHANGED], 0,
	               NULL);
}

static void
drop_cb (GtkWidget        *widget,
         GdkDragContext   *context,
	 gint              x,
	 gint              y,
	 GtkSelectionData *data,
	 guint             info,
	 guint             _time,
	 XplayerPlaylist    *playlist)
{
	char **list;
	GList *p, *file_list, *mrl_list = NULL;
	guint i;
	GdkDragAction action;

	if (gdk_drag_context_get_suggested_action (context) == GDK_ACTION_ASK) {
		action = xplayer_drag_ask (PL_LEN != 0);
		gdk_drag_status (context, action, GDK_CURRENT_TIME);
		if (action == GDK_ACTION_DEFAULT) {
			gtk_drag_finish (context, FALSE, FALSE, _time);
			return;
		}
	}

	action = gdk_drag_context_get_selected_action (context);
	if (action == GDK_ACTION_MOVE)
		xplayer_playlist_clear (playlist);

	list = g_uri_list_extract_uris ((char *) gtk_selection_data_get_data (data));
	file_list = NULL;

	for (i = 0; list[i] != NULL; i++) {
		/* We get the list in the wrong order here,
		 * so when we insert the files at the same position
		 * in the tree, they are in the right order.*/
		file_list = g_list_prepend (file_list, list[i]);
	}

	if (file_list == NULL) {
		gtk_drag_finish (context, FALSE, FALSE, _time);
		return;
	}

	playlist->priv->tree_path = gtk_tree_path_new ();
	gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (playlist->priv->treeview),
					   x, y,
					   &playlist->priv->tree_path,
					   &playlist->priv->drop_pos);

	/* But we reverse the list if we don't have any items in the
	 * list, as we insert new items at the end */
	if (playlist->priv->tree_path == NULL)
		file_list = g_list_reverse (file_list);

	for (p = file_list; p != NULL; p = p->next) {
		char *filename, *title;

		if (p->data == NULL)
			continue;

		filename = xplayer_create_full_path (p->data);
		if (filename == NULL)
			filename = g_strdup (p->data);
		title = NULL;

		if (info == 1) {
			p = p->next;
			if (p != NULL) {
				if (g_str_has_prefix (p->data, "file:") != FALSE)
					title = (char *)p->data + 5;
				else
					title = p->data;
			}
		}

		/* Add the MRL to the list of MRLs to be added to the playlist */
		mrl_list = g_list_prepend (mrl_list, xplayer_playlist_mrl_data_new (filename, title));
		g_free (filename);
	}

	/* Add all the MRLs to the playlist asynchronously, emitting the "changed" signal once we're done.
	 * Note that this takes ownership of @mrl_list. */
	if (mrl_list != NULL)
		xplayer_playlist_add_mrls (playlist, g_list_reverse (mrl_list), TRUE, NULL, (GAsyncReadyCallback) drop_finished_cb, NULL);

	g_strfreev (list);
	g_list_free (file_list);
	gtk_drag_finish (context, TRUE, FALSE, _time);
}

void
playlist_select_subtitle_action_callback (GtkAction *action, XplayerPlaylist *playlist)
{
	xplayer_playlist_select_subtitle_dialog (playlist, XPLAYER_PLAYLIST_DIALOG_SELECTED);
}

void
playlist_copy_location_action_callback (GtkAction *action, XplayerPlaylist *playlist)
{
	GList *l;
	GtkClipboard *clip;
	char *url;
	GtkTreeIter iter;

	l = gtk_tree_selection_get_selected_rows (playlist->priv->selection,
			NULL);
	gtk_tree_model_get_iter (playlist->priv->model, &iter, l->data);
	g_list_foreach (l, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (l);

	gtk_tree_model_get (playlist->priv->model,
			&iter,
			URI_COL, &url,
			-1);

	/* Set both the middle-click and the super-paste buffers */
	clip = gtk_clipboard_get_for_display
		(gdk_display_get_default(), GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text (clip, url, -1);
	clip = gtk_clipboard_get_for_display
		(gdk_display_get_default(), GDK_SELECTION_PRIMARY);
	gtk_clipboard_set_text (clip, url, -1);
	g_free (url);

}

static gboolean
playlist_show_popup_menu (XplayerPlaylist *playlist, GdkEventButton *event)
{
	guint button = 0;
	guint32 _time;
	GtkTreePath *path;
	gint count;
	GtkWidget *menu;
	GtkAction *copy_location;
	GtkAction *select_subtitle;

	if (event != NULL) {
		button = event->button;
		_time = event->time;

		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (playlist->priv->treeview),
				 event->x, event->y, &path, NULL, NULL, NULL)) {
			if (!gtk_tree_selection_path_is_selected (playlist->priv->selection, path)) {
				gtk_tree_selection_unselect_all (playlist->priv->selection);
				gtk_tree_selection_select_path (playlist->priv->selection, path);
			}
			gtk_tree_path_free (path);
		} else {
			gtk_tree_selection_unselect_all (playlist->priv->selection);
		}
	} else {
		_time = gtk_get_current_event_time ();
	}

	count = gtk_tree_selection_count_selected_rows (playlist->priv->selection);

	if (count == 0) {
		return FALSE;
	}

	copy_location = gtk_action_group_get_action (playlist->priv->action_group, "copy-location");
	select_subtitle = gtk_action_group_get_action (playlist->priv->action_group, "select-subtitle");
	gtk_action_set_sensitive (copy_location, count == 1);
	gtk_action_set_sensitive (select_subtitle, count == 1);

	menu = gtk_ui_manager_get_widget (playlist->priv->ui_manager, "/xplayer-playlist-popup");

	gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), FALSE);

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
			button, _time);

	return TRUE;
}

static gboolean
treeview_button_pressed (GtkTreeView *treeview, GdkEventButton *event,
		XplayerPlaylist *playlist)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		return playlist_show_popup_menu (playlist, event);
	}

	return FALSE;
}

static gboolean
playlist_treeview_popup_menu (GtkTreeView *treeview, XplayerPlaylist *playlist)
{
	return playlist_show_popup_menu (playlist, NULL);
}

static void
xplayer_playlist_set_reorderable (XplayerPlaylist *playlist, gboolean set)
{
	guint num_items, i;

	gtk_tree_view_set_reorderable
		(GTK_TREE_VIEW (playlist->priv->treeview), set);

	if (set != FALSE)
		return;

	num_items = PL_LEN;
	for (i = 0; i < num_items; i++)
	{
		GtkTreeIter iter;
		char *playlist_index;
		GtkTreePath *path;
		XplayerPlaylistStatus playing;

		playlist_index = g_strdup_printf ("%d", i);
		if (gtk_tree_model_get_iter_from_string
				(playlist->priv->model,
				 &iter, playlist_index) == FALSE)
		{
			g_free (playlist_index);
			continue;
		}
		g_free (playlist_index);

		gtk_tree_model_get (playlist->priv->model, &iter, PLAYING_COL, &playing, -1);
		if (playing == XPLAYER_PLAYLIST_STATUS_NONE)
			continue;

		/* Only emit the changed signal if we changed the ->current */
		path = gtk_tree_path_new_from_indices (i, -1);
		if (gtk_tree_path_compare (path, playlist->priv->current) == 0) {
			gtk_tree_path_free (path);
		} else {
			gtk_tree_path_free (playlist->priv->current);
			playlist->priv->current = path;
			g_signal_emit (G_OBJECT (playlist),
					xplayer_playlist_table_signals[CHANGED],
					0, NULL);
		}

		break;
	}
}

static gboolean
button_press_cb (GtkWidget *treeview, GdkEventButton *event, gpointer data)
{
	XplayerPlaylist *playlist = (XplayerPlaylist *)data;

	if (playlist->priv->drop_disabled)
		return FALSE;

	playlist->priv->drop_disabled = TRUE;
	gtk_drag_dest_unset (treeview);
	g_signal_handlers_block_by_func (treeview, (GFunc) drop_cb, data);

	xplayer_playlist_set_reorderable (playlist, TRUE);

	return FALSE;
}

static gboolean
button_release_cb (GtkWidget *treeview, GdkEventButton *event, gpointer data)
{
	XplayerPlaylist *playlist = (XplayerPlaylist *)data;

	if (!playlist->priv->drag_started && playlist->priv->drop_disabled)
	{
		playlist->priv->drop_disabled = FALSE;
		xplayer_playlist_set_reorderable (playlist, FALSE);
		gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (treeview),
						      target_table, G_N_ELEMENTS (target_table),
						      GDK_ACTION_COPY | GDK_ACTION_MOVE);

		g_signal_handlers_unblock_by_func (treeview,
				(GFunc) drop_cb, data);
	}

	return FALSE;
}

static void
drag_begin_cb (GtkWidget *treeview, GdkDragContext *context, gpointer data)
{
	XplayerPlaylist *playlist = (XplayerPlaylist *)data;

	playlist->priv->drag_started = TRUE;

	return;
}

static void
drag_end_cb (GtkWidget *treeview, GdkDragContext *context, gpointer data)
{
	XplayerPlaylist *playlist = (XplayerPlaylist *)data;

	playlist->priv->drop_disabled = FALSE;
	playlist->priv->drag_started = FALSE;
	xplayer_playlist_set_reorderable (playlist, FALSE);

	gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (treeview),
					      target_table, G_N_ELEMENTS (target_table),
					      GDK_ACTION_COPY | GDK_ACTION_MOVE);

	g_signal_handlers_unblock_by_func (treeview, (GFunc) drop_cb, data);

	return;
}

static void
selection_changed (GtkTreeSelection *treeselection, XplayerPlaylist *playlist)
{
	gboolean sensitivity;

	if (gtk_tree_selection_has_selected (treeselection))
		sensitivity = TRUE;
	else
		sensitivity = FALSE;

	gtk_widget_set_sensitive (playlist->priv->remove_button, sensitivity);
	gtk_widget_set_sensitive (playlist->priv->up_button, sensitivity);
	gtk_widget_set_sensitive (playlist->priv->down_button, sensitivity);
}

/* This function checks if the current item is NULL, and try to update it
 * as the first item of the playlist if so. It returns TRUE if there is a
 * current item */
static gboolean
update_current_from_playlist (XplayerPlaylist *playlist)
{
	int indice;

	if (playlist->priv->current != NULL)
		return TRUE;

	if (PL_LEN != 0)
	{
		if (playlist->priv->shuffle == FALSE)
		{
			indice = 0;
		} else {
			indice = playlist->priv->shuffled[0];
			playlist->priv->current_shuffled = 0;
		}

		playlist->priv->current = gtk_tree_path_new_from_indices
			(indice, -1);
	} else {
		return FALSE;
	}

	return TRUE;
}

void
xplayer_playlist_add_files (GtkWidget *widget, XplayerPlaylist *playlist)
{
	GSList *filenames, *l;
	GList *mrl_list = NULL;

	filenames = xplayer_add_files (xplayer_playlist_get_toplevel (playlist), NULL);
	if (filenames == NULL)
		return;

	for (l = filenames; l != NULL; l = l->next) {
		char *mrl = l->data;
		mrl_list = g_list_prepend (mrl_list, xplayer_playlist_mrl_data_new (mrl, NULL));
		g_free (mrl);
	}

	g_slist_free (filenames);

	if (mrl_list != NULL)
		xplayer_playlist_add_mrls (playlist, g_list_reverse (mrl_list), TRUE, NULL, NULL, NULL);
}

static void
xplayer_playlist_foreach_selected (GtkTreeModel *model, GtkTreePath *path,
		GtkTreeIter *iter, gpointer data)
{
	XplayerPlaylist *playlist = (XplayerPlaylist *)data;
	GtkTreeRowReference *ref;

	/* We can't use gtk_list_store_remove() here
	 * So we build a list a RowReferences */
	ref = gtk_tree_row_reference_new (playlist->priv->model, path);
	playlist->priv->list = g_list_prepend
		(playlist->priv->list, (gpointer) ref);
	if (playlist->priv->current_to_be_removed == FALSE
	    && playlist->priv->current != NULL
	    && gtk_tree_path_compare (path, playlist->priv->current) == 0)
		playlist->priv->current_to_be_removed = TRUE;
}

static void
xplayer_playlist_emit_item_removed (XplayerPlaylist *playlist,
				  GtkTreeIter   *iter)
{
	gchar *filename = NULL;
	gchar *uri = NULL;

	gtk_tree_model_get (playlist->priv->model, iter,
			    URI_COL, &uri, FILENAME_COL, &filename, -1);

	g_signal_emit (playlist,
		       xplayer_playlist_table_signals[ITEM_REMOVED],
		       0, filename, uri);

	g_free (filename);
	g_free (uri);
}

static void
playlist_remove_files (XplayerPlaylist *playlist)
{
	xplayer_playlist_clear_with_compare (playlist, NULL, NULL);
}

void
playlist_remove_button_clicked (GtkWidget *button, XplayerPlaylist *playlist)
{
	playlist_remove_files (playlist);
}

void
playlist_remove_action_callback (GtkAction *action, XplayerPlaylist *playlist)
{
	playlist_remove_files (playlist);
}

static void
xplayer_playlist_save_playlist (XplayerPlaylist *playlist, char *filename, gint active_format)
{
	if (active_format == 0)
		active_format = 1;

	xplayer_playlist_save_current_playlist_ext (playlist, filename,
						  save_types[active_format].type);
}

static char *
suffix_match_replace (const char *fname, guint old_format, guint new_format)
{
	char *ext;

	ext = g_strdup_printf (".%s", save_types[old_format].suffix);
	if (g_str_has_suffix (fname, ext) != FALSE) {
		char *no_suffix, *new_fname;

		no_suffix = g_strndup (fname, strlen (fname) - strlen (ext));
		new_fname = g_strconcat (no_suffix, ".", save_types[new_format].suffix, NULL);
		g_free (no_suffix);
		g_free (ext);

		return new_fname;
	}
	g_free (ext);

	return NULL;
}

static void
format_selection_changed (EggFileFormatChooser *chooser, XplayerPlaylist *playlist)
{
	guint format;

	format = egg_file_format_chooser_get_format (chooser, NULL);

	if (format != playlist->priv->save_format) {
		char *fname, *new_fname;

		new_fname = NULL;
		fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (playlist->priv->file_chooser));

		if (format == 0) {
			/* The new format is "By extension" don't touch anything */
		} else if (playlist->priv->save_format == 0) {
			guint i;

			for (i = 1; i < G_N_ELEMENTS (save_types); i++) {
				new_fname = suffix_match_replace (fname, i, format);
				if (new_fname != NULL)
					break;
			}
		} else {
			new_fname = suffix_match_replace (fname, playlist->priv->save_format, format);
		}
		if (new_fname != NULL) {
			char *basename;

			basename = g_path_get_basename (new_fname);
			g_free (new_fname);
			gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (playlist->priv->file_chooser), basename);
			g_free (basename);
		}
		playlist->priv->save_format = format;
	}
}

static GtkWidget *
xplayer_playlist_save_add_format_chooser (GtkFileChooser *fc, XplayerPlaylist *playlist)
{
	GtkWidget *format_chooser;
	guint i;

	format_chooser = egg_file_format_chooser_new ();

	playlist->priv->save_format = 0;

	for (i = 1; i < G_N_ELEMENTS (save_types) ; i++) {
		egg_file_format_chooser_add_format (
		    EGG_FILE_FORMAT_CHOOSER (format_chooser), 0, _(save_types[i].name),
		    "gnome-mime-audio", save_types[i].suffix, NULL);
	}

	g_signal_connect (format_chooser, "selection-changed",
			  G_CALLBACK (format_selection_changed), playlist);

	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (fc),
					   format_chooser);

	return format_chooser;
}

void
xplayer_playlist_save_files (GtkWidget *widget, XplayerPlaylist *playlist)
{
	GtkWidget *fs, *format_chooser;
	char *filename;
	int response;

	g_assert (playlist->priv->file_chooser == NULL);

	fs = gtk_file_chooser_dialog_new (_("Save Playlist"),
					  xplayer_playlist_get_toplevel (playlist),
					  GTK_FILE_CHOOSER_ACTION_SAVE,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					  NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (fs), GTK_RESPONSE_ACCEPT);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (fs), FALSE);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (fs), TRUE);

	/* translators: Playlist is the default saved playlist filename,
	 * without the suffix */
	filename = g_strconcat (_("Playlist"), ".", save_types[1].suffix, NULL);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (fs), filename);
	g_free (filename);
	format_chooser = xplayer_playlist_save_add_format_chooser (GTK_FILE_CHOOSER (fs), playlist);

	if (playlist->priv->save_path != NULL) {
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (fs),
				playlist->priv->save_path);
	}

	playlist->priv->file_chooser = fs;

	response = gtk_dialog_run (GTK_DIALOG (fs));
	gtk_widget_hide (fs);

	if (response == GTK_RESPONSE_ACCEPT) {
		char *fname;
		guint active_format;

		fname = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (fs));
		active_format = egg_file_format_chooser_get_format (EGG_FILE_FORMAT_CHOOSER (format_chooser),
								    fname);

		playlist->priv->file_chooser = NULL;
		gtk_widget_destroy (fs);

		if (fname == NULL)
			return;

		g_free (playlist->priv->save_path);
		playlist->priv->save_path = g_path_get_dirname (fname);

		xplayer_playlist_save_playlist (playlist, fname, active_format);
		g_free (fname);
	} else {
		playlist->priv->file_chooser = NULL;
		gtk_widget_destroy (fs);
	}
}

static void
xplayer_playlist_move_files (XplayerPlaylist *playlist, gboolean direction_up)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreeRowReference *current;
	GList *paths, *refs, *l;
	int pos;

	selection = gtk_tree_view_get_selection
		(GTK_TREE_VIEW (playlist->priv->treeview));
	if (selection == NULL)
		return;

	model = gtk_tree_view_get_model
		(GTK_TREE_VIEW (playlist->priv->treeview));
	store = GTK_LIST_STORE (model);
	pos = -2;
	refs = NULL;

	if (playlist->priv->current != NULL) {
		current = gtk_tree_row_reference_new (model,
				playlist->priv->current);
	} else {
		current = NULL;
	}

	/* Build a list of tree references */
	paths = gtk_tree_selection_get_selected_rows (selection, NULL);
	for (l = paths; l != NULL; l = l->next) {
		GtkTreePath *path = l->data;
		int cur_pos, *indices;

		refs = g_list_prepend (refs,
				gtk_tree_row_reference_new (model, path));
		indices = gtk_tree_path_get_indices (path);
		cur_pos = indices[0];
		if (pos == -2)
		{
			pos = cur_pos;
		} else {
			if (direction_up == FALSE)
				pos = MAX (cur_pos, pos);
			else
				pos = MIN (cur_pos, pos);
		}
	}
	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);

	/* Otherwise we reverse the items when moving down */
	if (direction_up != FALSE)
		refs = g_list_reverse (refs);

	if (direction_up == FALSE)
		pos = pos + 2;
	else
		pos = pos - 2;

	for (l = refs; l != NULL; l = l->next) {
		GtkTreeIter *position, cur;
		GtkTreeRowReference *ref = l->data;
		GtkTreePath *path;

		if (pos < 0) {
			position = NULL;
		} else {
			char *str;

			str = g_strdup_printf ("%d", pos);
			if (gtk_tree_model_get_iter_from_string (model,
					&iter, str))
				position = &iter;
			else
				position = NULL;

			g_free (str);
		}

		path = gtk_tree_row_reference_get_path (ref);
		gtk_tree_model_get_iter (model, &cur, path);
		gtk_tree_path_free (path);

		if (direction_up == FALSE)
		{
			pos--;
			gtk_list_store_move_before (store, &cur, position);
		} else {
			gtk_list_store_move_after (store, &cur, position);
			pos++;
		}
	}

	g_list_foreach (refs, (GFunc) gtk_tree_row_reference_free, NULL);
	g_list_free (refs);

	/* Update the current path */
	if (current != NULL) {
		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current = gtk_tree_row_reference_get_path
			(current);
		gtk_tree_row_reference_free (current);
	}

	g_signal_emit (G_OBJECT (playlist),
			xplayer_playlist_table_signals[CHANGED], 0,
			NULL);
}

void
xplayer_playlist_up_files (GtkWidget *widget, XplayerPlaylist *playlist)
{
	xplayer_playlist_move_files (playlist, TRUE);
}

void
xplayer_playlist_down_files (GtkWidget *widget, XplayerPlaylist *playlist)
{
	xplayer_playlist_move_files (playlist, FALSE);
}

static int
xplayer_playlist_key_press (GtkWidget *win, GdkEventKey *event, XplayerPlaylist *playlist)
{
	/* Special case some shortcuts */
	if (event->state != 0) {
		if ((event->state & GDK_CONTROL_MASK)
		    && event->keyval == GDK_KEY_a) {
			gtk_tree_selection_select_all
				(playlist->priv->selection);
			return TRUE;
		}
	}

	/* If we have modifiers, and either Ctrl, Mod1 (Alt), or any
	 * of Mod3 to Mod5 (Mod2 is num-lock...) are pressed, we
	 * let Gtk+ handle the key */
	if (event->state != 0
			&& ((event->state & GDK_CONTROL_MASK)
			|| (event->state & GDK_MOD1_MASK)
			|| (event->state & GDK_MOD3_MASK)
			|| (event->state & GDK_MOD4_MASK)
			|| (event->state & GDK_MOD5_MASK)))
		return FALSE;

	if (event->keyval == GDK_KEY_Delete)
	{
		playlist_remove_files (playlist);
		return TRUE;
	}

	return FALSE;
}

static void
set_playing_icon (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
		  GtkTreeModel *model, GtkTreeIter *iter, XplayerPlaylist *playlist)
{
	XplayerPlaylistStatus playing;
	const char *icon_name;

	gtk_tree_model_get (model, iter, PLAYING_COL, &playing, -1);

	switch (playing) {
		case XPLAYER_PLAYLIST_STATUS_PLAYING:
			icon_name = xplayer_get_rtl_icon_name ("media-playback-start");
			break;
		case XPLAYER_PLAYLIST_STATUS_PAUSED:
			icon_name = "media-playback-pause-symbolic";
			break;
		case XPLAYER_PLAYLIST_STATUS_NONE:
		default:
			icon_name = NULL;
	}

	g_object_set (renderer, "icon-name", icon_name, NULL);
}

static void
init_columns (GtkTreeView *treeview, XplayerPlaylist *playlist)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* Playing pix */
	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new ();
	g_object_set (G_OBJECT (column), "title", _("Playlist"), NULL);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
			(GtkTreeCellDataFunc) set_playing_icon, playlist, NULL);
	g_object_set (renderer, "stock-size", GTK_ICON_SIZE_MENU, NULL);
	gtk_tree_view_append_column (treeview, column);

	/* Labels */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
			"text", FILENAME_COL, NULL);
}

static void
treeview_row_changed (GtkTreeView *treeview, GtkTreePath *arg1,
		GtkTreeViewColumn *arg2, XplayerPlaylist *playlist)
{
	if (xplayer_playlist_gtk_tree_path_equals
	    (arg1, playlist->priv->current) != FALSE) {
		g_signal_emit (G_OBJECT (playlist),
				xplayer_playlist_table_signals[ITEM_ACTIVATED], 0,
				NULL);
		return;
	}

	if (playlist->priv->current != NULL) {
		xplayer_playlist_unset_playing (playlist);
		gtk_tree_path_free (playlist->priv->current);
	}

	playlist->priv->current = gtk_tree_path_copy (arg1);

	if (playlist->priv->shuffle != FALSE) {
		int *indices, indice, i;

		indices = gtk_tree_path_get_indices (playlist->priv->current);
		indice = indices[0];

		for (i = 0; i < PL_LEN; i++)
		{
			if (playlist->priv->shuffled[i] == indice)
			{
				playlist->priv->current_shuffled = i;
				break;
			}
		}
	}
	g_signal_emit (G_OBJECT (playlist),
			xplayer_playlist_table_signals[CHANGED], 0,
			NULL);

	if (playlist->priv->drop_disabled) {
		playlist->priv->drop_disabled = FALSE;
		xplayer_playlist_set_reorderable (playlist, FALSE);

		gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (treeview),
						      target_table, G_N_ELEMENTS (target_table),
						      GDK_ACTION_COPY | GDK_ACTION_MOVE);

		g_signal_handlers_unblock_by_func (treeview,
				(GFunc) drop_cb, playlist);
	}
}

static gboolean
search_equal_is_match (const gchar * s, const gchar * lc_key)
{
	gboolean match = FALSE;

	if (s != NULL) {
		gchar *lc_s;

		/* maybe also normalize both strings? */
		lc_s = g_utf8_strdown (s, -1);
		match = (lc_s != NULL && strstr (lc_s, lc_key) != NULL);
		g_free (lc_s);
	}

	return match;
}

static gboolean
search_equal_func (GtkTreeModel *model, gint col, const gchar *key,
                   GtkTreeIter *iter, gpointer userdata)
{
	gboolean match;
	gchar *lc_key, *fn = NULL;

	lc_key = g_utf8_strdown (key, -1);

        /* type-ahead search: first check display filename / title, then URI */
	gtk_tree_model_get (model, iter, FILENAME_COL, &fn, -1);
	match = search_equal_is_match (fn, lc_key);
	g_free (fn);

	if (!match) {
		gchar *uri = NULL;

		gtk_tree_model_get (model, iter, URI_COL, &uri, -1);
		fn = g_filename_from_uri (uri, NULL, NULL);
		match = search_equal_is_match (fn, lc_key);
		g_free (fn);
		g_free (uri);
	}

	g_free (lc_key);
	return !match; /* needs to return FALSE if row matches */
}

static void
init_treeview (GtkWidget *treeview, XplayerPlaylist *playlist)
{
	GtkTreeSelection *selection;

	init_columns (GTK_TREE_VIEW (treeview), playlist);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect (G_OBJECT (selection), "changed",
			G_CALLBACK (selection_changed), playlist);
	g_signal_connect (G_OBJECT (treeview), "row-activated",
			G_CALLBACK (treeview_row_changed), playlist);
	g_signal_connect (G_OBJECT (treeview), "button-press-event",
			G_CALLBACK (treeview_button_pressed), playlist);
	g_signal_connect (G_OBJECT (treeview), "popup-menu",
			G_CALLBACK (playlist_treeview_popup_menu), playlist);

	/* Drag'n'Drop */
	g_signal_connect (G_OBJECT (treeview), "drag_data_received",
			G_CALLBACK (drop_cb), playlist);
        g_signal_connect (G_OBJECT (treeview), "button_press_event",
			G_CALLBACK (button_press_cb), playlist);
        g_signal_connect (G_OBJECT (treeview), "button_release_event",
			G_CALLBACK (button_release_cb), playlist);
	g_signal_connect (G_OBJECT (treeview), "drag_begin",
                        G_CALLBACK (drag_begin_cb), playlist);
	g_signal_connect (G_OBJECT (treeview), "drag_end",
                        G_CALLBACK (drag_end_cb), playlist);

	gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (treeview),
					      target_table, G_N_ELEMENTS (target_table),
					      GDK_ACTION_COPY | GDK_ACTION_MOVE);

	playlist->priv->selection = selection;

	/* make type-ahead search work in the playlist */
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (treeview),
	                                     search_equal_func, NULL, NULL);

	gtk_widget_show (treeview);
}

static void
update_repeat_cb (GSettings *settings, const gchar *key, XplayerPlaylist *playlist)
{
	playlist->priv->repeat = g_settings_get_boolean (settings, "repeat");

	g_signal_emit (G_OBJECT (playlist),
			xplayer_playlist_table_signals[CHANGED], 0,
			NULL);
	g_signal_emit (G_OBJECT (playlist),
			xplayer_playlist_table_signals[REPEAT_TOGGLED], 0,
			playlist->priv->repeat, NULL);
}

typedef struct {
	int random;
	int index;
} RandomData;

static int
compare_random (gconstpointer ptr_a, gconstpointer ptr_b)
{
	RandomData *a = (RandomData *) ptr_a;
	RandomData *b = (RandomData *) ptr_b;

	if (a->random < b->random)
		return -1;
	else if (a->random > b->random)
		return 1;
	else
		return 0;
}

static void
ensure_shuffled (XplayerPlaylist *playlist)
{
	RandomData data;
	GArray *array;
	int i, current, current_new;
	int *indices;

	if (playlist->priv->shuffled == NULL)
		playlist->priv->shuffled = g_new (int, PL_LEN);
	else if (PL_LEN != playlist->priv->shuffle_len)
		playlist->priv->shuffled = g_renew (int, playlist->priv->shuffled, PL_LEN);
	playlist->priv->shuffle_len = PL_LEN;

	if (PL_LEN == 0)
		return;

	if (playlist->priv->current != NULL) {
		indices = gtk_tree_path_get_indices (playlist->priv->current);
		current = indices[0];
	} else {
		current = -1;
	}

	current_new = -1;

	array = g_array_sized_new (FALSE, FALSE, sizeof (RandomData), PL_LEN);

	for (i = 0; i < PL_LEN; i++) {
		data.random = g_random_int_range (0, PL_LEN);
		data.index = i;

		g_array_append_val (array, data);
	}

	g_array_sort (array, compare_random);

	for (i = 0; i < PL_LEN; i++) {
		playlist->priv->shuffled[i] = g_array_index (array, RandomData, i).index;

		if (playlist->priv->current != NULL && playlist->priv->shuffled[i] == current)
			current_new = i;
	}

	if (current_new > -1) {
		playlist->priv->shuffled[current_new] = playlist->priv->shuffled[0];
		playlist->priv->shuffled[0] = current;
		playlist->priv->current_shuffled = 0;
	}

	g_array_free (array, TRUE);
}

static void
update_shuffle_cb (GSettings *settings, const gchar *key, XplayerPlaylist *playlist)
{
	playlist->priv->shuffle = g_settings_get_boolean (settings, "shuffle");

	if (playlist->priv->shuffle == FALSE) {
		g_free (playlist->priv->shuffled);
		playlist->priv->shuffled = NULL;
		playlist->priv->shuffle_len = 0;
	} else {
		ensure_shuffled (playlist);
	}

	g_signal_emit (G_OBJECT (playlist),
			xplayer_playlist_table_signals[CHANGED], 0,
			NULL);
	g_signal_emit (G_OBJECT (playlist),
			xplayer_playlist_table_signals[SHUFFLE_TOGGLED], 0,
			playlist->priv->shuffle, NULL);
}

static void
update_lockdown_cb (GSettings *settings, const gchar *key, XplayerPlaylist *playlist)
{
	playlist->priv->disable_save_to_disk = g_settings_get_boolean (settings, "disable-save-to-disk");
	xplayer_playlist_update_save_button (playlist);
}

static void
init_config (XplayerPlaylist *playlist)
{
	playlist->priv->settings = g_settings_new (XPLAYER_GSETTINGS_SCHEMA);
	playlist->priv->lockdown_settings = g_settings_new ("org.gnome.desktop.lockdown");

	playlist->priv->disable_save_to_disk = g_settings_get_boolean (playlist->priv->lockdown_settings, "disable-save-to-disk");
	xplayer_playlist_update_save_button (playlist);

	g_signal_connect (playlist->priv->lockdown_settings, "changed::disable-save-to-disk", (GCallback) update_lockdown_cb, playlist);

	playlist->priv->repeat = g_settings_get_boolean (playlist->priv->settings, "repeat");
	playlist->priv->shuffle = g_settings_get_boolean (playlist->priv->settings, "shuffle");

	g_signal_connect (playlist->priv->settings, "changed::repeat", (GCallback) update_repeat_cb, playlist);
	g_signal_connect (playlist->priv->settings, "changed::shuffle", (GCallback) update_shuffle_cb, playlist);
}

static void
xplayer_playlist_entry_parsed (XplayerPlParser *parser,
			     const char *uri,
			     GHashTable *metadata,
			     XplayerPlaylist *playlist)
{
	const char *title, *content_type;
	gint64 duration;

	/* We ignore 0-length items in playlists, they're usually just banners */
	duration = xplayer_pl_parser_parse_duration
		(g_hash_table_lookup (metadata, XPLAYER_PL_PARSER_FIELD_DURATION), FALSE);
	if (duration == 0)
		return;
	title = g_hash_table_lookup (metadata, XPLAYER_PL_PARSER_FIELD_TITLE);
	content_type = g_hash_table_lookup (metadata, XPLAYER_PL_PARSER_FIELD_CONTENT_TYPE);
	xplayer_playlist_add_one_mrl (playlist, uri, title, content_type);
}

static gboolean
xplayer_playlist_compare_with_monitor (XplayerPlaylist *playlist, GtkTreeIter *iter, gconstpointer data)
{
	GFileMonitor *monitor = (GFileMonitor *) data;
	GFileMonitor *_monitor;
	gboolean retval = FALSE;

	gtk_tree_model_get (playlist->priv->model, iter,
			    FILE_MONITOR_COL, &_monitor, -1);

	if (_monitor == monitor)
		retval = TRUE;

	if (_monitor != NULL)
		g_object_unref (_monitor);

	return retval;
}

static void
xplayer_playlist_file_changed (GFileMonitor *monitor,
			     GFile *file,
			     GFile *other_file,
			     GFileMonitorEvent event_type,
			     XplayerPlaylist *playlist)
{
	if (event_type == G_FILE_MONITOR_EVENT_PRE_UNMOUNT ||
	    event_type == G_FILE_MONITOR_EVENT_UNMOUNTED) {
		xplayer_playlist_clear_with_compare (playlist,
						   (ClearComparisonFunc) xplayer_playlist_compare_with_monitor,
						   monitor);
	}
}

static void
xplayer_playlist_dispose (GObject *object)
{
	XplayerPlaylist *playlist = XPLAYER_PLAYLIST (object);

	if (playlist->priv->parser != NULL) {
		g_object_unref (playlist->priv->parser);
		playlist->priv->parser = NULL;
	}

	if (playlist->priv->ui_manager != NULL) {
		g_object_unref (G_OBJECT (playlist->priv->ui_manager));
		playlist->priv->ui_manager = NULL;
	}

	if (playlist->priv->action_group != NULL) {
		g_object_unref (G_OBJECT (playlist->priv->action_group));
		playlist->priv->action_group = NULL;
	}

	if (playlist->priv->settings != NULL)
		g_object_unref (playlist->priv->settings);
	playlist->priv->settings = NULL;

	if (playlist->priv->lockdown_settings != NULL)
		g_object_unref (playlist->priv->lockdown_settings);
	playlist->priv->lockdown_settings = NULL;

	G_OBJECT_CLASS (xplayer_playlist_parent_class)->dispose (object);
}

static void
xplayer_playlist_finalize (GObject *object)
{
	XplayerPlaylist *playlist = XPLAYER_PLAYLIST (object);

	if (playlist->priv->current != NULL)
		gtk_tree_path_free (playlist->priv->current);

	g_clear_pointer (&playlist->priv->tree_path, gtk_tree_path_free);

	G_OBJECT_CLASS (xplayer_playlist_parent_class)->finalize (object);
}

static void
xplayer_playlist_init (XplayerPlaylist *playlist)
{
	GtkWidget *container;
	GtkBuilder *xml;
	GtkWidget *widget;
	GtkStyleContext *context;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (playlist),
					GTK_ORIENTATION_VERTICAL);

	playlist->priv = G_TYPE_INSTANCE_GET_PRIVATE (playlist, XPLAYER_TYPE_PLAYLIST, XplayerPlaylistPrivate);
	playlist->priv->parser = xplayer_pl_parser_new ();

	xplayer_pl_parser_add_ignored_scheme (playlist->priv->parser, "dvd:");
	xplayer_pl_parser_add_ignored_scheme (playlist->priv->parser, "vcd:");
	xplayer_pl_parser_add_ignored_scheme (playlist->priv->parser, "cd:");
	xplayer_pl_parser_add_ignored_scheme (playlist->priv->parser, "dvb:");
	xplayer_pl_parser_add_ignored_mimetype (playlist->priv->parser, "application/x-trash");

	g_signal_connect (G_OBJECT (playlist->priv->parser),
			"entry-parsed",
			G_CALLBACK (xplayer_playlist_entry_parsed),
			playlist);

	xml = xplayer_interface_load ("playlist.ui", TRUE, NULL, playlist);

	if (xml == NULL)
		return;

	/* popup menu */
	playlist->priv->action_group = GTK_ACTION_GROUP (gtk_builder_get_object (xml, "playlist-action-group"));
	g_object_ref (playlist->priv->action_group);
	playlist->priv->ui_manager = GTK_UI_MANAGER (gtk_builder_get_object (xml, "xplayer-playlist-ui-manager"));
	g_object_ref (playlist->priv->ui_manager);

	gtk_widget_add_events (GTK_WIDGET (playlist), GDK_KEY_PRESS_MASK);
	g_signal_connect (G_OBJECT (playlist), "key_press_event",
			  G_CALLBACK (xplayer_playlist_key_press), playlist);

	/* Buttons */
	playlist->priv->save_button = GTK_WIDGET (gtk_builder_get_object (xml, "save_button"));;
	playlist->priv->remove_button = GTK_WIDGET (gtk_builder_get_object (xml, "remove_button"));
	playlist->priv->up_button = GTK_WIDGET (gtk_builder_get_object (xml, "up_button"));
	playlist->priv->down_button = GTK_WIDGET (gtk_builder_get_object (xml, "down_button"));

	/* Join treeview and buttons */
	widget = GTK_WIDGET (gtk_builder_get_object (xml, ("scrolledwindow1")));
	context = gtk_widget_get_style_context (widget);
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_BOTTOM);
	playlist->priv->toolbar = GTK_WIDGET (gtk_builder_get_object (xml, ("toolbar1")));
	context = gtk_widget_get_style_context (playlist->priv->toolbar);
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_TOP);

	/* Reparent the vbox */
	container = GTK_WIDGET (gtk_builder_get_object (xml, "vbox4"));
	g_object_ref (container);
	gtk_box_pack_start (GTK_BOX (playlist),
			container,
			TRUE,       /* expand */
			TRUE,       /* fill */
			0);         /* padding */
	g_object_unref (container);

	playlist->priv->treeview = GTK_WIDGET (gtk_builder_get_object (xml, "treeview1"));
	init_treeview (playlist->priv->treeview, playlist);
	playlist->priv->model = gtk_tree_view_get_model
		(GTK_TREE_VIEW (playlist->priv->treeview));

	/* tooltips */
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(playlist->priv->treeview),
					 FILENAME_ESCAPED_COL);

	/* The configuration */
	init_config (playlist);

	gtk_widget_show_all (GTK_WIDGET (playlist));

	g_object_unref (xml);
}

GtkWidget*
xplayer_playlist_new (void)
{
	XplayerPlaylist *playlist;

	playlist = XPLAYER_PLAYLIST (g_object_new (XPLAYER_TYPE_PLAYLIST, NULL));
	if (playlist->priv->ui_manager == NULL) {
		g_object_unref (playlist);
		return NULL;
	}

	return GTK_WIDGET (playlist);
}

static gboolean
xplayer_playlist_add_one_mrl (XplayerPlaylist *playlist,
			    const char *mrl,
			    const char *display_name,
			    const char *content_type)
{
	GtkListStore *store;
	GtkTreeIter iter;
	char *filename_for_display, *uri, *escaped_filename;
	GtkTreeRowReference *ref;
	GFileMonitor *monitor;
	GMount *mount;
	GFile *file;
	int pos;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), FALSE);
	g_return_val_if_fail (mrl != NULL, FALSE);

	if (display_name == NULL || *display_name == '\0')
		filename_for_display = xplayer_playlist_mrl_to_title (mrl);
	else
		filename_for_display = g_strdup (display_name);

	ref = NULL;
	uri = xplayer_create_full_path (mrl);

	g_debug ("xplayer_playlist_add_one_mrl (): %s %s %s\n", filename_for_display, uri, display_name);

	if (playlist->priv->tree_path != NULL && playlist->priv->current != NULL) {
		int *indices;
		indices = gtk_tree_path_get_indices (playlist->priv->tree_path);
		pos = indices[0];
		ref = gtk_tree_row_reference_new (playlist->priv->model, playlist->priv->current);
	} else {
		pos = G_MAXINT;
	}

	store = GTK_LIST_STORE (playlist->priv->model);

	/* Get the file monitor */
	file = g_file_new_for_uri (uri ? uri : mrl);
	if (g_file_is_native (file) != FALSE) {
		monitor = g_file_monitor_file (file,
					       G_FILE_MONITOR_NONE,
					       NULL,
					       NULL);
		g_signal_connect (G_OBJECT (monitor),
				  "changed",
				  G_CALLBACK (xplayer_playlist_file_changed),
				  playlist);
		mount = NULL;
	} else {
		mount = xplayer_get_mount_for_media (uri ? uri : mrl);
		monitor = NULL;
	}

	escaped_filename = g_markup_escape_text (filename_for_display, -1);
	gtk_list_store_insert_with_values (store, &iter, pos,
					   PLAYING_COL, XPLAYER_PLAYLIST_STATUS_NONE,
					   FILENAME_COL, filename_for_display,
					   FILENAME_ESCAPED_COL, escaped_filename,
					   URI_COL, uri ? uri : mrl,
					   TITLE_CUSTOM_COL, display_name ? TRUE : FALSE,
					   FILE_MONITOR_COL, monitor,
					   MOUNT_COL, mount,
					   MIME_TYPE_COL, content_type,
					   -1);
	g_free (escaped_filename);

	g_signal_emit (playlist,
		       xplayer_playlist_table_signals[ITEM_ADDED],
		       0, filename_for_display, uri ? uri : mrl);

	g_free (filename_for_display);
	g_free (uri);

	if (playlist->priv->current == NULL && playlist->priv->shuffle == FALSE)
		playlist->priv->current = gtk_tree_model_get_path (playlist->priv->model, &iter);
	if (playlist->priv->shuffle)
		ensure_shuffled (playlist);

	/* And update current to point to the right file again */
	if (ref != NULL) {
		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current = gtk_tree_row_reference_get_path (ref);
		gtk_tree_row_reference_free (ref);
	}

	g_signal_emit (G_OBJECT (playlist),
			xplayer_playlist_table_signals[CHANGED], 0,
			NULL);
	xplayer_playlist_update_save_button (playlist);

	return TRUE;
}

typedef struct {
	GAsyncReadyCallback callback;
	gpointer user_data;
	gboolean cursor;
	XplayerPlaylist *playlist;
	gchar *mrl;
	gchar *display_name;
} AddMrlData;

static void
add_mrl_data_free (AddMrlData *data)
{
	g_object_unref (data->playlist);
	g_free (data->mrl);
	g_free (data->display_name);
	g_slice_free (AddMrlData, data);
}

static gboolean
handle_parse_result (XplayerPlParserResult res, XplayerPlaylist *playlist, const gchar *mrl, const gchar *display_name)
{
	if (res == XPLAYER_PL_PARSER_RESULT_UNHANDLED)
		return xplayer_playlist_add_one_mrl (playlist, mrl, display_name, NULL);
	if (res == XPLAYER_PL_PARSER_RESULT_ERROR) {
		char *msg;

		msg = g_strdup_printf (_("The playlist '%s' could not be parsed. It might be damaged."), display_name ? display_name : mrl);
		xplayer_playlist_error (_("Playlist error"), msg, playlist);
		g_free (msg);

		return FALSE;
	}
	if (res == XPLAYER_PL_PARSER_RESULT_IGNORED)
		return FALSE;

	return TRUE;
}

static void
add_mrl_cb (XplayerPlParser *parser, GAsyncResult *result, AddMrlData *data)
{
	XplayerPlParserResult res;
	GSimpleAsyncResult *async_result;
	GError *error = NULL;

	/* Finish parsing the playlist */
	res = xplayer_pl_parser_parse_finish (parser, result, &error);

	/* Remove the cursor, if one was set */
	if (data->cursor)
		unset_waiting_cursor (data->playlist);

	/* Create an async result which will return the result to the code which called xplayer_playlist_add_mrl() */
	if (error != NULL)
		async_result = g_simple_async_result_new_from_error (G_OBJECT (data->playlist), data->callback, data->user_data, error);
	else
		async_result = g_simple_async_result_new (G_OBJECT (data->playlist), data->callback, data->user_data, xplayer_playlist_add_mrl);

	/* Handle the various return cases from the playlist parser */
	g_simple_async_result_set_op_res_gboolean (async_result, handle_parse_result (res, data->playlist, data->mrl, data->display_name));

	/* Free the closure's data, now that we're finished with it */
	add_mrl_data_free (data);

	/* Synchronously call the calling code's callback function (i.e. what was passed to xplayer_playlist_add_mrl()'s @callback parameter)
	 * in the main thread to return the result */
	g_simple_async_result_complete (async_result);
}

void
xplayer_playlist_add_mrl (XplayerPlaylist *playlist, const char *mrl, const char *display_name, gboolean cursor,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	AddMrlData *data;

	g_return_if_fail (mrl != NULL);

	/* Display a waiting cursor if required */
	if (cursor)
		set_waiting_cursor (playlist);

	/* Build the data struct to pass to the callback function */
	data = g_slice_new (AddMrlData);
	data->callback = callback;
	data->user_data = user_data;
	data->cursor = cursor;
	data->playlist = g_object_ref (playlist);
	data->mrl = g_strdup (mrl);
	data->display_name = g_strdup (display_name);

	/* Start parsing the playlist. Once this is complete, add_mrl_cb() is called, which will interpret the results and call @callback to
	 * finish the process. */
	xplayer_pl_parser_parse_async (playlist->priv->parser, mrl, FALSE, cancellable, (GAsyncReadyCallback) add_mrl_cb, data);
}

gboolean
xplayer_playlist_add_mrl_finish (XplayerPlaylist *playlist, GAsyncResult *result)
{
	g_assert (g_simple_async_result_get_source_tag (G_SIMPLE_ASYNC_RESULT (result)) == xplayer_playlist_add_mrl);

	return g_simple_async_result_get_op_res_gboolean (G_SIMPLE_ASYNC_RESULT (result));
}

gboolean
xplayer_playlist_add_mrl_sync (XplayerPlaylist *playlist, const char *mrl, const char *display_name)
{
	g_return_val_if_fail (mrl != NULL, FALSE);

	return handle_parse_result (xplayer_pl_parser_parse (playlist->priv->parser, mrl, FALSE), playlist, mrl, display_name);
}

typedef struct {
	XplayerPlaylist *playlist;
	GList *mrls; /* list of XplayerPlaylistMrlDatas */
	gboolean cursor;
	GAsyncReadyCallback callback;
	gpointer user_data;

	guint next_index_to_add;
	GList *unadded_entries; /* list of XplayerPlaylistMrlDatas */
	volatile gint entries_remaining;
} AddMrlsOperationData;

static void
add_mrls_operation_data_free (AddMrlsOperationData *data)
{
	/* Remove the cursor, if one was set */
	if (data->cursor)
		unset_waiting_cursor (data->playlist);

	g_list_foreach (data->mrls, (GFunc) xplayer_playlist_mrl_data_free, NULL);
	g_list_free (data->mrls);
	g_object_unref (data->playlist);

	g_slice_free (AddMrlsOperationData, data);
}

struct XplayerPlaylistMrlData {
	gchar *mrl;
	gchar *display_name;
	XplayerPlParserResult res;

	/* Implementation details */
	AddMrlsOperationData *operation_data;
	guint index;
};

/**
 * xplayer_playlist_mrl_data_new:
 * @mrl: a MRL
 * @display_name: (allow-none): a human-readable display name for the MRL, or %NULL
 *
 * Create a new #XplayerPlaylistMrlData struct storing the given @mrl and @display_name.
 *
 * This will typically be immediately appended to a #GList to be passed to xplayer_playlist_add_mrls().
 *
 * Return value: (transfer full): a new #XplayerPlaylistMrlData; free with xplayer_playlist_mrl_data_free()
 *
 * Since: 3.0
 */
XplayerPlaylistMrlData *
xplayer_playlist_mrl_data_new (const gchar *mrl,
                             const gchar *display_name)
{
	XplayerPlaylistMrlData *data;

	g_return_val_if_fail (mrl != NULL && *mrl != '\0', NULL);

	data = g_slice_new (XplayerPlaylistMrlData);
	data->mrl = g_strdup (mrl);
	data->display_name = g_strdup (display_name);

	return data;
}

/**
 * xplayer_playlist_mrl_data_free:
 * @data: (transfer full): a #XplayerPlaylistMrlData
 *
 * Free the given #XplayerPlaylistMrlData struct. This should not generally be called by code outside #XplayerPlaylist.
 *
 * Since: 3.0
 */
void
xplayer_playlist_mrl_data_free (XplayerPlaylistMrlData *data)
{
	g_return_if_fail (data != NULL);

	/* NOTE: This doesn't call add_mrls_operation_data_free() on @data->operation_data, since it's shared with other instances of
	 * XplayerPlaylistMrlData, and not truly reference counted. */
	g_free (data->display_name);
	g_free (data->mrl);

	g_slice_free (XplayerPlaylistMrlData, data);
}

static void
add_mrls_finish_operation (AddMrlsOperationData *operation_data)
{
	/* Check whether this is the final callback invocation; iff it is, we can call the user's callback for the entire operation and free the
	 * operation data */
	if (g_atomic_int_dec_and_test (&(operation_data->entries_remaining)) == TRUE) {
		GSimpleAsyncResult *async_result;

		async_result = g_simple_async_result_new (G_OBJECT (operation_data->playlist), operation_data->callback, operation_data->user_data,
		                                          xplayer_playlist_add_mrls);
		g_simple_async_result_complete (async_result);
		g_object_unref (async_result);

		add_mrls_operation_data_free (operation_data);
	}
}

/* Called exactly once for each MRL in a xplayer_playlist_add_mrls() operation. Called in the thread running the main loop. If the MRL which has just
 * been parsed is the next one in the sequence (of entries in @mrls as passed to xplayer_playlist_add_mrls()), it's added to the playlist proper.
 * Otherwise, it's added to a sorted queue of MRLs which have had their callbacks called out of order.
 * When a MRL is added to the playlist proper, any successor MRLs which are in the sorted queue are also added to the playlist proper.
 * When add_mrls_cb() is called for the last time for a given call to xplayer_playlist_add_mrls(), it calls the user's callback for the operation
 * (passed as @callback to xplayer_playlist_add_mrls()) and frees the #AddMrlsOperationData struct. This is handled by add_mrls_finish_operation().
 * The #XplayerPlaylistMrlData for each MRL is freed by add_mrls_operation_data_free() at the end of the entire operation. */
static void
add_mrls_cb (XplayerPlParser *parser, GAsyncResult *result, XplayerPlaylistMrlData *mrl_data)
{
	AddMrlsOperationData *operation_data = mrl_data->operation_data;

	/* Finish parsing the playlist */
	mrl_data->res = xplayer_pl_parser_parse_finish (parser, result, NULL);

	g_assert (mrl_data->index >= operation_data->next_index_to_add);

	if (mrl_data->index == operation_data->next_index_to_add) {
		GList *i;

		/* The entry is the next one in the order, so doesn't need to be added to the unadded list, and can be added to playlist proper */
		operation_data->next_index_to_add++;
		handle_parse_result (mrl_data->res, operation_data->playlist, mrl_data->mrl, mrl_data->display_name);

		/* See if we can now add any other entries which have already been processed */
		for (i = operation_data->unadded_entries;
		     i != NULL && ((XplayerPlaylistMrlData*) i->data)->index == operation_data->next_index_to_add;
		     i = g_list_delete_link (i, i)) {
			XplayerPlaylistMrlData *_mrl_data = (XplayerPlaylistMrlData*) i->data;

			operation_data->next_index_to_add++;
			handle_parse_result (_mrl_data->res, operation_data->playlist, _mrl_data->mrl, _mrl_data->display_name);
		}

		operation_data->unadded_entries = i;
	} else {
		GList *i;

		/* The entry has been parsed out of order, so needs to be added (in the correct position) to the unadded list for latter addition to
		 * the playlist proper */
		for (i = operation_data->unadded_entries; i != NULL && mrl_data->index > ((XplayerPlaylistMrlData*) i->data)->index; i = i->next);
		operation_data->unadded_entries = g_list_insert_before (operation_data->unadded_entries, i, mrl_data);
	}

	/* Check whether this is the last callback; call the user's callback for the entire operation and free the operation data if appropriate */
	add_mrls_finish_operation (operation_data);
}

/**
 * xplayer_playlist_add_mrls:
 * @self: a #XplayerPlaylist
 * @mrls: (element-type XplayerPlaylistMrlData) (transfer full): a list of #XplayerPlaylistMrlData structs
 * @cursor: %TRUE to set a waiting cursor on the playlist for the duration of the operation, %FALSE otherwise
 * @cancellable: (allow-none): a #Cancellable, or %NULL
 * @callback: (scope async) (allow-none): callback to call once all the MRLs have been added to the playlist, or %NULL
 * @user_data: (closure) (allow-none): user data to pass to @callback, or %NULL
 *
 * Add the MRLs listed in @mrls to the playlist asynchronously, and ensuring that they're added to the playlist in the order they appear in the
 * input #GList.
 *
 * @mrls should be a #GList of #XplayerPlaylistMrlData structs, each created with xplayer_playlist_mrl_data_new(). This function takes ownership of both
 * the list and its elements when called, so don't free either after calling xplayer_playlist_add_mrls().
 *
 * @callback will be called after all the MRLs in @mrls have been parsed and (if they were parsed successfully) added to the playlist. In the
 * callback function, xplayer_playlist_add_mrls_finish() should be called to check for errors.
 *
 * Since: 3.0
 */
void
xplayer_playlist_add_mrls (XplayerPlaylist *self,
                         GList *mrls,
                         gboolean cursor,
                         GCancellable *cancellable,
                         GAsyncReadyCallback callback,
                         gpointer user_data)
{
	AddMrlsOperationData *operation_data;
	GList *i;
	guint mrl_index = 0;

	g_return_if_fail (XPLAYER_IS_PLAYLIST (self));
	g_return_if_fail (mrls != NULL);
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	/* Build the data struct to pass to the callback function */
	operation_data = g_slice_new (AddMrlsOperationData);
	operation_data->playlist = g_object_ref (self);
	operation_data->mrls = mrls;
	operation_data->cursor = cursor;
	operation_data->callback = callback;
	operation_data->user_data = user_data;
	operation_data->next_index_to_add = mrl_index;
	operation_data->unadded_entries = NULL;
	g_atomic_int_set (&(operation_data->entries_remaining), 1);

	/* Display a waiting cursor if required */
	if (cursor)
		set_waiting_cursor (self);

	for (i = mrls; i != NULL; i = i->next) {
		XplayerPlaylistMrlData *mrl_data = (XplayerPlaylistMrlData*) i->data;

		if (mrl_data == NULL)
			continue;

		/* Set the item's parsing index, so that it's inserted into the playlist in the position it appeared in @mrls */
		mrl_data->operation_data = operation_data;
		mrl_data->index = mrl_index++;

		g_atomic_int_inc (&(operation_data->entries_remaining));

		/* Start parsing the playlist. Once this is complete, add_mrls_cb() is called (i.e. it's called exactly once for each entry in
		 * @mrls).
		 * TODO: Cancellation is currently not supoprted, since no consumers of this API make use of it, and it needs careful thought when
		 * being implemented, as a separate #GCancellable instance will have to be created for each parallel computation. */
		xplayer_pl_parser_parse_async (self->priv->parser, mrl_data->mrl, FALSE, NULL, (GAsyncReadyCallback) add_mrls_cb, mrl_data);
	}

	/* Deal with the case that all async operations completed before we got to this point (since we've held a reference to the operation data so
	 * that it doesn't get freed prematurely if all the scheduled async parse operations complete before we've finished scheduling the rest. */
	add_mrls_finish_operation (operation_data);
}

/**
 * xplayer_playlist_add_mrls_finish:
 * @self: a #XplayerPlaylist
 * @result: the #GAsyncResult that was provided to the callback
 * @error: (allow-none): a #GError for error reporting, or %NULL
 *
 * Finish an asynchronous batch MRL addition operation started by xplayer_playlist_add_mrls().
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 3.0
 */
gboolean
xplayer_playlist_add_mrls_finish (XplayerPlaylist *self,
                                GAsyncResult *result,
                                GError **error)
{
	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (self), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (self), xplayer_playlist_add_mrls), FALSE);

	/* We don't have anything to return at the moment. */
	return TRUE;
}

static gboolean
xplayer_playlist_clear_cb (GtkTreeModel *model,
			 GtkTreePath  *path,
			 GtkTreeIter  *iter,
			 gpointer      data)
{
	xplayer_playlist_emit_item_removed (data, iter);
	return FALSE;
}

gboolean
xplayer_playlist_clear (XplayerPlaylist *playlist)
{
	GtkListStore *store;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), FALSE);

	if (PL_LEN == 0)
		return FALSE;

	gtk_tree_model_foreach (playlist->priv->model,
				xplayer_playlist_clear_cb,
				playlist);

	store = GTK_LIST_STORE (playlist->priv->model);
	gtk_list_store_clear (store);

	if (playlist->priv->current != NULL)
		gtk_tree_path_free (playlist->priv->current);
	playlist->priv->current = NULL;

	xplayer_playlist_update_save_button (playlist);

	return TRUE;
}

static int
compare_removal (GtkTreeRowReference *ref, GtkTreePath *path)
{
	GtkTreePath *ref_path;
	int ret = -1;

	ref_path = gtk_tree_row_reference_get_path (ref);
	if (gtk_tree_path_compare (path, ref_path) == 0)
		ret = 0;
	gtk_tree_path_free (ref_path);
	return ret;
}

/* Whether the item in question will be removed */
static gboolean
xplayer_playlist_item_to_be_removed (XplayerPlaylist *playlist,
				   GtkTreePath *path,
				   ClearComparisonFunc func)
{
	GList *ret;

	if (func == NULL) {
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (playlist->priv->treeview));
		return gtk_tree_selection_path_is_selected (selection, path);
	}

	ret = g_list_find_custom (playlist->priv->list, path, (GCompareFunc) compare_removal);
	return (ret != NULL);
}

static void
xplayer_playlist_clear_with_compare (XplayerPlaylist *playlist,
				   ClearComparisonFunc func,
				   gconstpointer data)
{
	GtkTreeRowReference *ref;
	GtkTreeRowReference *next;

	ref = NULL;
	next = NULL;

	if (func == NULL) {
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection
			(GTK_TREE_VIEW (playlist->priv->treeview));
		if (selection == NULL)
			return;

		gtk_tree_selection_selected_foreach (selection,
						     xplayer_playlist_foreach_selected,
						     (gpointer) playlist);
	} else {
		guint num_items, i;

		num_items = PL_LEN;
		if (num_items == 0)
			return;

		for (i = 0; i < num_items; i++) {
			GtkTreeIter iter;
			char *playlist_index;

			playlist_index = g_strdup_printf ("%d", i);
			if (gtk_tree_model_get_iter_from_string (playlist->priv->model, &iter, playlist_index) == FALSE) {
				g_free (playlist_index);
				continue;
			}
			g_free (playlist_index);

			if ((* func) (playlist, &iter, data) != FALSE) {
				GtkTreePath *path;
				GtkTreeRowReference *r;

				path = gtk_tree_path_new_from_indices (i, -1);
				r = gtk_tree_row_reference_new (playlist->priv->model, path);
				if (playlist->priv->current_to_be_removed == FALSE && playlist->priv->current != NULL) {
					if (gtk_tree_path_compare (path, playlist->priv->current) == 0) {
						playlist->priv->current_to_be_removed = TRUE;
					}
				}
				playlist->priv->list = g_list_prepend (playlist->priv->list, r);
				gtk_tree_path_free (path);
			}
		}

		if (playlist->priv->list == NULL)
			return;
	}

	/* If the current item is to change, we need to keep an static
	 * reference to it, TreeIter and TreePath don't allow that */
	if (playlist->priv->current_to_be_removed == FALSE &&
	    playlist->priv->current != NULL) {
		ref = gtk_tree_row_reference_new (playlist->priv->model,
						  playlist->priv->current);
	} else if (playlist->priv->current != NULL) {
		GtkTreePath *item;

		item = gtk_tree_path_copy (playlist->priv->current);
		gtk_tree_path_next (item);
		next = gtk_tree_row_reference_new (playlist->priv->model, item);
		while (next != NULL) {
			if (xplayer_playlist_item_to_be_removed (playlist, item, func) == FALSE) {
				/* Found the item after the current one that
				 * won't be removed, thus the new current */
				break;
			}
			gtk_tree_row_reference_free (next);
			gtk_tree_path_next (item);
			next = gtk_tree_row_reference_new (playlist->priv->model, item);
		}
	}

	/* We destroy the items, one-by-one from the list built above */
	while (playlist->priv->list != NULL) {
		GtkTreePath *path;
		GtkTreeIter iter;

		path = gtk_tree_row_reference_get_path
			((GtkTreeRowReference *)(playlist->priv->list->data));
		gtk_tree_model_get_iter (playlist->priv->model, &iter, path);
		gtk_tree_path_free (path);

		xplayer_playlist_emit_item_removed (playlist, &iter);
		gtk_list_store_remove (GTK_LIST_STORE (playlist->priv->model), &iter);

		gtk_tree_row_reference_free
			((GtkTreeRowReference *)(playlist->priv->list->data));
		playlist->priv->list = g_list_remove (playlist->priv->list,
				playlist->priv->list->data);
	}
	g_list_free (playlist->priv->list);
	playlist->priv->list = NULL;

	if (playlist->priv->current_to_be_removed != FALSE) {
		/* The current item was removed from the playlist */
		if (next != NULL) {
			playlist->priv->current = gtk_tree_row_reference_get_path (next);
			gtk_tree_row_reference_free (next);
		} else {
			playlist->priv->current = NULL;
		}

		playlist->priv->current_shuffled = -1;
		if (playlist->priv->shuffle)
			ensure_shuffled (playlist);

		g_signal_emit (G_OBJECT (playlist),
				xplayer_playlist_table_signals[CURRENT_REMOVED],
				0, NULL);
	} else {
		if (ref != NULL) {
			/* The path to the current item changed */
			playlist->priv->current = gtk_tree_row_reference_get_path (ref);
		}

		if (playlist->priv->shuffle)
			ensure_shuffled (playlist);

		g_signal_emit (G_OBJECT (playlist),
				xplayer_playlist_table_signals[CHANGED], 0,
				NULL);
	}
	if (ref != NULL)
		gtk_tree_row_reference_free (ref);
	xplayer_playlist_update_save_button (playlist);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW (playlist->priv->treeview));

	playlist->priv->current_to_be_removed = FALSE;
}

static char *
get_mount_default_location (GMount *mount)
{
	GFile *file;
	char *path;

	file = g_mount_get_root (mount);
	if (file == NULL)
		return NULL;
	path = g_file_get_path (file);
	g_object_unref (file);
	return path;
}

static gboolean
xplayer_playlist_compare_with_mount (XplayerPlaylist *playlist, GtkTreeIter *iter, gconstpointer data)
{
	GMount *clear_mount = (GMount *) data;
	GMount *mount;
	char *mount_path, *clear_mount_path;
	gboolean retval = FALSE;

	gtk_tree_model_get (playlist->priv->model, iter,
			    MOUNT_COL, &mount, -1);

	if (mount == NULL)
		return FALSE;

	clear_mount_path = NULL;

	mount_path = get_mount_default_location (mount);
	if (mount_path == NULL)
		goto bail;

	clear_mount_path = get_mount_default_location (clear_mount);
	if (clear_mount_path == NULL)
		goto bail;

	if (g_str_equal (mount_path, clear_mount_path))
		retval = TRUE;

bail:
	g_free (mount_path);
	g_free (clear_mount_path);

	if (mount != NULL)
		g_object_unref (mount);

	return retval;
}

void
xplayer_playlist_clear_with_g_mount (XplayerPlaylist *playlist,
				   GMount *mount)
{
	g_return_if_fail (mount != NULL);

	xplayer_playlist_clear_with_compare (playlist,
					   (ClearComparisonFunc) xplayer_playlist_compare_with_mount,
					   mount);
}

char *
xplayer_playlist_get_current_mrl (XplayerPlaylist *playlist, char **subtitle)
{
	GtkTreeIter iter;
	char *path;

	if (subtitle != NULL)
		*subtitle = NULL;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), NULL);

	if (update_current_from_playlist (playlist) == FALSE)
		return NULL;

	if (gtk_tree_model_get_iter (playlist->priv->model, &iter,
				     playlist->priv->current) == FALSE)
		return NULL;

	if (subtitle != NULL) {
		gtk_tree_model_get (playlist->priv->model, &iter,
				    URI_COL, &path,
				    SUBTITLE_URI_COL, subtitle,
				    -1);
	} else {
		gtk_tree_model_get (playlist->priv->model, &iter,
				    URI_COL, &path,
				    -1);
	}

	return path;
}

char *
xplayer_playlist_get_current_title (XplayerPlaylist *playlist)
{
	GtkTreeIter iter;
	char *title;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), NULL);

	if (update_current_from_playlist (playlist) == FALSE)
		return NULL;

	gtk_tree_model_get_iter (playlist->priv->model,
				 &iter,
				 playlist->priv->current);

	gtk_tree_model_get (playlist->priv->model,
			    &iter,
			    FILENAME_COL, &title,
			    -1);
	return title;
}

char *
xplayer_playlist_get_current_content_type (XplayerPlaylist *playlist)
{
	GtkTreeIter iter;
	char *content_type;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), NULL);

	if (update_current_from_playlist (playlist) == FALSE)
		return NULL;

	gtk_tree_model_get_iter (playlist->priv->model,
				 &iter,
				 playlist->priv->current);

	gtk_tree_model_get (playlist->priv->model,
			    &iter,
			    MIME_TYPE_COL, &content_type,
			    -1);

	return content_type;
}

char *
xplayer_playlist_get_title (XplayerPlaylist *playlist, guint title_index)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	char *title;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), NULL);

	path = gtk_tree_path_new_from_indices (title_index, -1);

	gtk_tree_model_get_iter (playlist->priv->model,
				 &iter, path);

	gtk_tree_path_free (path);

	gtk_tree_model_get (playlist->priv->model,
			    &iter,
			    FILENAME_COL, &title,
			    -1);

	return title;
}

gboolean
xplayer_playlist_has_previous_mrl (XplayerPlaylist *playlist)
{
	GtkTreeIter iter;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), FALSE);

	if (update_current_from_playlist (playlist) == FALSE)
		return FALSE;

	if (playlist->priv->shuffle == FALSE) {
		gtk_tree_model_get_iter (playlist->priv->model,
					 &iter,
					 playlist->priv->current);

		return gtk_tree_model_iter_previous (playlist->priv->model, &iter);
	} else {
		if (playlist->priv->current_shuffled == 0)
			return FALSE;
	}

	return TRUE;
}

gboolean
xplayer_playlist_has_next_mrl (XplayerPlaylist *playlist)
{
	GtkTreeIter iter;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), FALSE);

	if (update_current_from_playlist (playlist) == FALSE)
		return FALSE;

	if (playlist->priv->shuffle == FALSE) {
		gtk_tree_model_get_iter (playlist->priv->model,
					 &iter,
					 playlist->priv->current);

		return gtk_tree_model_iter_next (playlist->priv->model, &iter);
	} else {
		if (playlist->priv->current_shuffled == PL_LEN - 1)
			return FALSE;
	}

	return TRUE;
}

gboolean
xplayer_playlist_set_title (XplayerPlaylist *playlist, const char *title)
{
	GtkListStore *store;
	GtkTreeIter iter;
	char *escaped_title;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), FALSE);

	if (update_current_from_playlist (playlist) == FALSE)
		return FALSE;

	store = GTK_LIST_STORE (playlist->priv->model);
	gtk_tree_model_get_iter (playlist->priv->model,
			&iter,
			playlist->priv->current);

	escaped_title = g_markup_escape_text (title, -1);
	gtk_list_store_set (store, &iter,
			FILENAME_COL, title,
			FILENAME_ESCAPED_COL, escaped_title,
			TITLE_CUSTOM_COL, TRUE,
			-1);
	g_free (escaped_title);

	g_signal_emit (playlist,
		       xplayer_playlist_table_signals[ACTIVE_NAME_CHANGED], 0);

	return TRUE;
}

gboolean
xplayer_playlist_set_playing (XplayerPlaylist *playlist, XplayerPlaylistStatus state)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreePath *path;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), FALSE);

	if (update_current_from_playlist (playlist) == FALSE)
		return FALSE;

	store = GTK_LIST_STORE (playlist->priv->model);
	gtk_tree_model_get_iter (playlist->priv->model,
			&iter,
			playlist->priv->current);

	gtk_list_store_set (store, &iter,
			PLAYING_COL, state,
			-1);

	if (state == FALSE)
		return TRUE;

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (playlist->priv->treeview),
				      path, NULL,
				      TRUE, 0.5, 0);
	gtk_tree_path_free (path);

	return TRUE;
}

XplayerPlaylistStatus
xplayer_playlist_get_playing (XplayerPlaylist *playlist)
{
	GtkTreeIter iter;
	XplayerPlaylistStatus status;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), XPLAYER_PLAYLIST_STATUS_NONE);

	if (gtk_tree_model_get_iter (playlist->priv->model, &iter, playlist->priv->current) == FALSE)
		return XPLAYER_PLAYLIST_STATUS_NONE;

	gtk_tree_model_get (playlist->priv->model,
			    &iter,
			    PLAYING_COL, &status,
			    -1);

	return status;
}

void
xplayer_playlist_set_previous (XplayerPlaylist *playlist)
{
	GtkTreeIter iter;

	g_return_if_fail (XPLAYER_IS_PLAYLIST (playlist));

	if (xplayer_playlist_has_previous_mrl (playlist) == FALSE)
		return;

	xplayer_playlist_unset_playing (playlist);

	if (playlist->priv->shuffle == FALSE) {
		char *path;

		path = gtk_tree_path_to_string (playlist->priv->current);
		if (g_str_equal (path, "0")) {
			xplayer_playlist_set_at_end (playlist);
			g_free (path);
			return;
		}
		g_free (path);

		gtk_tree_model_get_iter (playlist->priv->model,
				&iter,
				playlist->priv->current);

		if (!gtk_tree_model_iter_previous (playlist->priv->model, &iter))
			g_assert_not_reached ();
		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current = gtk_tree_model_get_path
			(playlist->priv->model, &iter);
	} else {
		int indice;

		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current_shuffled--;
		if (playlist->priv->current_shuffled < 0) {
			indice = playlist->priv->shuffled[PL_LEN -1];
			playlist->priv->current_shuffled = PL_LEN -1;
		} else {
			indice = playlist->priv->shuffled[playlist->priv->current_shuffled];
		}
		playlist->priv->current = gtk_tree_path_new_from_indices
			(indice, -1);
	}
}

void
xplayer_playlist_set_next (XplayerPlaylist *playlist)
{
	GtkTreeIter iter;

	g_return_if_fail (XPLAYER_IS_PLAYLIST (playlist));

	if (xplayer_playlist_has_next_mrl (playlist) == FALSE) {
		xplayer_playlist_set_at_start (playlist);
		return;
	}

	xplayer_playlist_unset_playing (playlist);

	if (playlist->priv->shuffle == FALSE) {
		gtk_tree_model_get_iter (playlist->priv->model,
					 &iter,
					 playlist->priv->current);

		if (!gtk_tree_model_iter_next (playlist->priv->model, &iter))
			g_assert_not_reached ();
		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current = gtk_tree_model_get_path (playlist->priv->model, &iter);
	} else {
		int indice;

		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current_shuffled++;
		if (playlist->priv->current_shuffled == PL_LEN)
			playlist->priv->current_shuffled = 0;
		indice = playlist->priv->shuffled[playlist->priv->current_shuffled];
		playlist->priv->current = gtk_tree_path_new_from_indices
			                        (indice, -1);
	}
}

gboolean
xplayer_playlist_get_repeat (XplayerPlaylist *playlist)
{
	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), FALSE);

	return playlist->priv->repeat;
}

void
xplayer_playlist_set_repeat (XplayerPlaylist *playlist, gboolean repeat)
{
	g_return_if_fail (XPLAYER_IS_PLAYLIST (playlist));

	g_settings_set_boolean (playlist->priv->settings, "repeat", repeat);
}

gboolean
xplayer_playlist_get_shuffle (XplayerPlaylist *playlist)
{
	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), FALSE);

	return playlist->priv->shuffle;
}

void
xplayer_playlist_set_shuffle (XplayerPlaylist *playlist, gboolean shuffle)
{
	g_return_if_fail (XPLAYER_IS_PLAYLIST (playlist));

	g_settings_set_boolean (playlist->priv->settings, "shuffle", shuffle);
}

void
xplayer_playlist_set_at_start (XplayerPlaylist *playlist)
{
	g_return_if_fail (XPLAYER_IS_PLAYLIST (playlist));

	xplayer_playlist_unset_playing (playlist);

	if (playlist->priv->current != NULL)
	{
		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current = NULL;
	}
	update_current_from_playlist (playlist);
}

void
xplayer_playlist_set_at_end (XplayerPlaylist *playlist)
{
	int indice;

	g_return_if_fail (XPLAYER_IS_PLAYLIST (playlist));

	xplayer_playlist_unset_playing (playlist);

	if (playlist->priv->current != NULL)
	{
		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current = NULL;
	}

	if (PL_LEN)
	{
		if (playlist->priv->shuffle == FALSE)
			indice = PL_LEN - 1;
		else
			indice = playlist->priv->shuffled[PL_LEN - 1];

		playlist->priv->current = gtk_tree_path_new_from_indices
			(indice, -1);
	}
}

int
xplayer_playlist_get_current (XplayerPlaylist *playlist)
{
	char *path;
	double current_index;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), -1);

	if (playlist->priv->current == NULL)
		return -1;
	path = gtk_tree_path_to_string (playlist->priv->current);
	if (path == NULL)
		return -1;

	current_index = g_ascii_strtod (path, NULL);
	g_free (path);

	return current_index;
}

int
xplayer_playlist_get_last (XplayerPlaylist *playlist)
{
	guint len = PL_LEN;

	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), -1);

	if (len == 0)
		return -1;

	return len - 1;
}

GtkWidget *
xplayer_playlist_get_toolbar (XplayerPlaylist *playlist)
{
	g_return_val_if_fail (XPLAYER_IS_PLAYLIST (playlist), NULL);

	return playlist->priv->toolbar;
}

void
xplayer_playlist_set_current (XplayerPlaylist *playlist, guint current_index)
{
	g_return_if_fail (XPLAYER_IS_PLAYLIST (playlist));

	if (current_index >= (guint) PL_LEN)
		return;

	xplayer_playlist_unset_playing (playlist);
	//FIXME problems when shuffled?
	gtk_tree_path_free (playlist->priv->current);
	playlist->priv->current = gtk_tree_path_new_from_indices (current_index, -1);
}

static void
xplayer_playlist_class_init (XplayerPlaylistClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (XplayerPlaylistPrivate));

	object_class->dispose = xplayer_playlist_dispose;
	object_class->finalize = xplayer_playlist_finalize;

	/* Signals */
	xplayer_playlist_table_signals[CHANGED] =
		g_signal_new ("changed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (XplayerPlaylistClass, changed),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
	xplayer_playlist_table_signals[ITEM_ACTIVATED] =
		g_signal_new ("item-activated",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (XplayerPlaylistClass, item_activated),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
	xplayer_playlist_table_signals[ACTIVE_NAME_CHANGED] =
		g_signal_new ("active-name-changed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (XplayerPlaylistClass, active_name_changed),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
	xplayer_playlist_table_signals[CURRENT_REMOVED] =
		g_signal_new ("current-removed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (XplayerPlaylistClass,
						 current_removed),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
	xplayer_playlist_table_signals[REPEAT_TOGGLED] =
		g_signal_new ("repeat-toggled",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (XplayerPlaylistClass,
						 repeat_toggled),
				NULL, NULL,
				g_cclosure_marshal_VOID__BOOLEAN,
				G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	xplayer_playlist_table_signals[SHUFFLE_TOGGLED] =
		g_signal_new ("shuffle-toggled",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (XplayerPlaylistClass,
						 shuffle_toggled),
				NULL, NULL,
				g_cclosure_marshal_VOID__BOOLEAN,
				G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	xplayer_playlist_table_signals[SUBTITLE_CHANGED] =
		g_signal_new ("subtitle-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XplayerPlaylistClass,
					       subtitle_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	xplayer_playlist_table_signals[ITEM_ADDED] =
		g_signal_new ("item-added",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (XplayerPlaylistClass,
					item_added),
				NULL, NULL,
				g_cclosure_marshal_generic,
				G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
	xplayer_playlist_table_signals[ITEM_REMOVED] =
		g_signal_new ("item-removed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (XplayerPlaylistClass,
					item_removed),
				NULL, NULL,
				g_cclosure_marshal_generic,
				G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
}
