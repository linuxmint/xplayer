/* xplayer-menu.c

   Copyright (C) 2004-2005 Bastien Nocera

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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#define GST_USE_UNSTABLE_API 1
#include <gst/tag/tag.h>
#include <string.h>
#include <libpeas-gtk/peas-gtk-plugin-manager.h>

#include "xplayer-menu.h"
#include "xplayer.h"
#include "xplayer-interface.h"
#include "xplayer-private.h"
#include "xplayer-sidebar.h"
#include "xplayer-statusbar.h"
#include "bacon-video-widget.h"
#include "xplayer-uri.h"

#include "xplayer-profile.h"

#define XPLAYER_MAX_RECENT_ITEM_LEN 40

/* Callback functions for GtkBuilder */
G_MODULE_EXPORT void open_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void open_location_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void eject_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void properties_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void play_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void quit_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void preferences_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void fullscreen_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void zoom_1_2_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void zoom_1_1_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void zoom_2_1_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void zoom_toggle_action_callback (GtkToggleAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void next_angle_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void dvd_root_menu_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void dvd_title_menu_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void dvd_audio_menu_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void dvd_angle_menu_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void dvd_chapter_menu_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void next_chapter_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void previous_chapter_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void skip_forward_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void skip_backwards_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void volume_up_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void volume_down_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void contents_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void about_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void plugins_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void repeat_mode_action_callback (GtkToggleAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void shuffle_mode_action_callback (GtkToggleAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void show_controls_action_callback (GtkToggleAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void show_sidebar_action_callback (GtkToggleAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void aspect_ratio_changed_callback (GtkRadioAction *action, GtkRadioAction *current, Xplayer *xplayer);
G_MODULE_EXPORT void select_subtitle_action_callback (GtkAction *action, Xplayer *xplayer);
G_MODULE_EXPORT void clear_playlist_action_callback (GtkAction *action, Xplayer *xplayer);

/* Helper function to escape underscores in labels
 * before putting them in menu items */
static char *
escape_label_for_menu (const char *name)
{
	char *new, **a;

	a = g_strsplit (name, "_", -1);
	new = g_strjoinv ("__", a);
	g_strfreev (a);

	return new;
}

/* Subtitle and language menus */
static void
xplayer_g_list_deep_free (GList *list)
{
	GList *l;

	for (l = list; l != NULL; l = l->next)
		g_free (l->data);
	g_list_free (list);
}

static void
subtitles_changed_callback (GtkRadioAction *action, GtkRadioAction *current,
		Xplayer *xplayer)
{
	int rank;

	rank = gtk_radio_action_get_current_value (current);

	bacon_video_widget_set_subtitle (xplayer->bvw, rank);
}


static void
languages_changed_callback (GtkRadioAction *action, GtkRadioAction *current,
		Xplayer *xplayer)
{
	int rank;

	rank = gtk_radio_action_get_current_value (current);

	bacon_video_widget_set_language (xplayer->bvw, rank);
}

static GtkAction *
add_lang_action (Xplayer *xplayer, GtkActionGroup *action_group, guint ui_id,
		const char **paths, const char *prefix, const char *lang, 
		int lang_id, int lang_index, GSList **group)
{
	const char *full_lang;
	char *label;
	char *name;
	GtkAction *action;
	guint i;

	full_lang = gst_tag_get_language_name (lang);

	if (lang_index > 1) {
		char *num_lang;

		num_lang = g_strdup_printf ("%s #%u",
					    full_lang ? full_lang : lang,
					    lang_index);
		label = escape_label_for_menu (num_lang);
		g_free (num_lang);
	} else {
		label = escape_label_for_menu (full_lang ? full_lang : lang);
	}

	name = g_strdup_printf ("%s-%d", prefix, lang_id);

	action = g_object_new (GTK_TYPE_RADIO_ACTION,
			       "name", name,
			       "label", label,
			       "value", lang_id,
			       NULL);
	g_free (label);

	gtk_radio_action_set_group (GTK_RADIO_ACTION (action), *group);
	*group = gtk_radio_action_get_group (GTK_RADIO_ACTION (action));
	gtk_action_group_add_action (action_group, action);
	g_object_unref (action);
	for (i = 0; paths[i] != NULL; i++) {
		gtk_ui_manager_add_ui (xplayer->ui_manager, ui_id,
				       paths[i], name, name, GTK_UI_MANAGER_MENUITEM, FALSE);
	}
	g_free (name);

	return action;
}

static GtkAction *
create_lang_actions (Xplayer *xplayer, GtkActionGroup *action_group, guint ui_id,
		const char **paths, const char *prefix, GList *list,
		gboolean is_lang)
{
	GtkAction *action = NULL;
	unsigned int i, *hash_value;
	GList *l;
	GSList *group = NULL;
	GHashTable *lookup;
	char *action_data;

	if (is_lang == FALSE) {
		add_lang_action (xplayer, action_group, ui_id, paths, prefix,
		                /* Translators: an entry in the "Languages" menu, used to choose the audio language of a DVD */
				_("None"), -2, 0, &group);
	}

	action = add_lang_action (xplayer, action_group, ui_id, paths, prefix,
	                          /* Translators: an entry in the "Languages" menu, used to choose the audio language of a DVD */
	                          C_("Language", "Auto"), -1, 0, &group);

	i = 0;
	lookup = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);

	for (l = list; l != NULL; l = l->next)
	{
		guint num;

		hash_value = g_hash_table_lookup (lookup, l->data);
		if (hash_value == NULL) {
			num = 0;
			action_data = g_strdup (l->data);
			g_hash_table_insert (lookup, l->data, GINT_TO_POINTER (1));
		} else {
			num = GPOINTER_TO_INT (hash_value);
			action_data = g_strdup (l->data);
			g_hash_table_replace (lookup, l->data, GINT_TO_POINTER (num + 1));
		}

		add_lang_action (xplayer, action_group, ui_id, paths, prefix,
				 action_data, i, num + 1, &group);
		g_free (action_data);
		i++;
	}

	g_hash_table_destroy (lookup);

	return action;
}

static gboolean
xplayer_sublang_equal_lists (GList *orig, GList *new)
{
	GList *o, *n;
	gboolean retval;

	if ((orig == NULL && new != NULL) || (orig != NULL && new == NULL))
		return FALSE;
	if (orig == NULL && new == NULL)
		return TRUE;

	if (g_list_length (orig) != g_list_length (new))
		return FALSE;

	retval = TRUE;
	o = orig;
	n = new;
	while (o != NULL && n != NULL && retval != FALSE)
	{
		if (g_str_equal (o->data, n->data) == FALSE)
			retval = FALSE;
                o = g_list_next (o);
                n = g_list_next (n);
	}

	return retval;
}

static void
xplayer_languages_update (Xplayer *xplayer, GList *list)
{
	GtkAction *action;
	const char *paths[3] = { "/tmw-menubar/sound/languages/placeholder", "/xplayer-main-popup/popup-languages/placeholder", NULL };
	int current;

	/* Remove old UI */
	gtk_ui_manager_remove_ui (xplayer->ui_manager, xplayer->languages_ui_id);
	gtk_ui_manager_ensure_update (xplayer->ui_manager);

	/* Create new ActionGroup */
	if (xplayer->languages_action_group) {
		gtk_ui_manager_remove_action_group (xplayer->ui_manager,
				xplayer->languages_action_group);
		g_object_unref (xplayer->languages_action_group);
	}
	xplayer->languages_action_group = gtk_action_group_new ("languages-action-group");
	gtk_ui_manager_insert_action_group (xplayer->ui_manager,
			xplayer->languages_action_group, -1);

	if (list != NULL) {
		action = create_lang_actions (xplayer, xplayer->languages_action_group,
				xplayer->languages_ui_id,
				paths,
				"languages", list, TRUE);
		gtk_ui_manager_ensure_update (xplayer->ui_manager);

		current = bacon_video_widget_get_language (xplayer->bvw);
		gtk_radio_action_set_current_value (GTK_RADIO_ACTION (action),
				current);
		g_signal_connect (G_OBJECT (action), "changed",
				G_CALLBACK (languages_changed_callback), xplayer);
	}

	xplayer_g_list_deep_free (xplayer->language_list);
	xplayer->language_list = list;
}

static void
xplayer_subtitles_update (Xplayer *xplayer, GList *list)
{
	GtkAction *action;
	int current;
	const char *paths[3] = { "/tmw-menubar/view/subtitles/placeholder", "/xplayer-main-popup/popup-subtitles/placeholder", NULL };

	/* Remove old UI */
	gtk_ui_manager_remove_ui (xplayer->ui_manager, xplayer->subtitles_ui_id);
	gtk_ui_manager_ensure_update (xplayer->ui_manager);

	/* Create new ActionGroup */
	if (xplayer->subtitles_action_group) {
		gtk_ui_manager_remove_action_group (xplayer->ui_manager,
				xplayer->subtitles_action_group);
		g_object_unref (xplayer->subtitles_action_group);
	}
	xplayer->subtitles_action_group = gtk_action_group_new ("subtitles-action-group");
	gtk_ui_manager_insert_action_group (xplayer->ui_manager,
			xplayer->subtitles_action_group, -1);


	if (list != NULL) {
		action = create_lang_actions (xplayer, xplayer->subtitles_action_group,
				xplayer->subtitles_ui_id,
				paths,
				"subtitles", list, FALSE);
		gtk_ui_manager_ensure_update (xplayer->ui_manager);

		current = bacon_video_widget_get_subtitle (xplayer->bvw);
		gtk_radio_action_set_current_value (GTK_RADIO_ACTION (action),
				current);
		g_signal_connect (G_OBJECT (action), "changed",
				G_CALLBACK (subtitles_changed_callback), xplayer);
	}

	xplayer_g_list_deep_free (xplayer->subtitles_list);
	xplayer->subtitles_list = list;
}

void
xplayer_sublang_update (Xplayer *xplayer)
{
	GList *list;

	list = bacon_video_widget_get_languages (xplayer->bvw);
	if (xplayer_sublang_equal_lists (xplayer->language_list, list) == TRUE) {
		xplayer_g_list_deep_free (list);
	} else {
		xplayer_languages_update (xplayer, list);
	}

	list = bacon_video_widget_get_subtitles (xplayer->bvw);
	if (xplayer_sublang_equal_lists (xplayer->subtitles_list, list) == TRUE) {
		xplayer_g_list_deep_free (list);
	} else {
		xplayer_subtitles_update (xplayer, list);
	}
}

void
xplayer_sublang_exit (Xplayer *xplayer)
{
	xplayer_g_list_deep_free (xplayer->subtitles_list);
	xplayer_g_list_deep_free (xplayer->language_list);
}

/* Recent files */
static void
connect_proxy_cb (GtkActionGroup *action_group,
                  GtkAction *action,
                  GtkWidget *proxy,
                  gpointer data)
{
        GtkLabel *label;

        if (!GTK_IS_MENU_ITEM (proxy))
                return;

        label = GTK_LABEL (gtk_bin_get_child (GTK_BIN (proxy)));

        gtk_label_set_ellipsize (label, PANGO_ELLIPSIZE_MIDDLE);
        gtk_label_set_max_width_chars (label,XPLAYER_MAX_RECENT_ITEM_LEN);
}

static void
on_recent_file_item_activated (GtkAction *action,
                               Xplayer *xplayer)
{
	GtkRecentInfo *recent_info;
	const gchar *uri, *display_name;

	recent_info = g_object_get_data (G_OBJECT (action), "recent-info");
	uri = gtk_recent_info_get_uri (recent_info);
	display_name = gtk_recent_info_get_display_name (recent_info);

	xplayer_add_to_playlist_and_play (xplayer, uri, display_name);
}

static gint
xplayer_compare_recent_items (GtkRecentInfo *a, GtkRecentInfo *b)
{
	gboolean has_xplayer_a, has_xplayer_b;

	has_xplayer_a = gtk_recent_info_has_group (a, "Xplayer");
	has_xplayer_b = gtk_recent_info_has_group (b, "Xplayer");

	if (has_xplayer_a && has_xplayer_b) {
		time_t time_a, time_b;

		time_a = gtk_recent_info_get_modified (a);
		time_b = gtk_recent_info_get_modified (b);

		return (time_b - time_a);
	} else if (has_xplayer_a) {
		return -1;
	} else if (has_xplayer_b) {
		return 1;
	}

	return 0;
}

static void
xplayer_recent_manager_changed_callback (GtkRecentManager *recent_manager, Xplayer *xplayer)
{
        GList *items, *xplayer_items, *l;
        guint n_items = 0;

        if (xplayer->recent_ui_id != 0) {
                gtk_ui_manager_remove_ui (xplayer->ui_manager, xplayer->recent_ui_id);
                gtk_ui_manager_ensure_update (xplayer->ui_manager);
        }

        if (xplayer->recent_action_group) {
                gtk_ui_manager_remove_action_group (xplayer->ui_manager,
                                xplayer->recent_action_group);
        }

        xplayer->recent_action_group = gtk_action_group_new ("recent-action-group");
        g_signal_connect (xplayer->recent_action_group, "connect-proxy",
                          G_CALLBACK (connect_proxy_cb), NULL);
        gtk_ui_manager_insert_action_group (xplayer->ui_manager,
                        xplayer->recent_action_group, -1);
        g_object_unref (xplayer->recent_action_group);

        xplayer->recent_ui_id = gtk_ui_manager_new_merge_id (xplayer->ui_manager);
        items = gtk_recent_manager_get_items (recent_manager);

	/* Remove the non-Xplayer items */
	xplayer_items = NULL;
        for (l = items; l && l->data; l = l->next) {
                GtkRecentInfo *info;

                info = (GtkRecentInfo *) l->data;

                if (gtk_recent_info_has_group (info, "Xplayer")) {
			gtk_recent_info_ref (info);
			xplayer_items = g_list_prepend (xplayer_items, info);
		}
	}
	g_list_foreach (items, (GFunc) gtk_recent_info_unref, NULL);
        g_list_free (items);

        xplayer_items = g_list_sort (xplayer_items, (GCompareFunc) xplayer_compare_recent_items);

        for (l = xplayer_items; l && l->data; l = l->next) {
                GtkRecentInfo *info;
                GtkAction     *action;
                char           action_name[32];
                const char    *display_name;
                char          *label;
                char          *escaped_label;
                const gchar   *mime_type;
                gchar         *content_type;
                GIcon         *icon = NULL;

                info = (GtkRecentInfo *) l->data;

                if (!gtk_recent_info_has_group (info, "Xplayer"))
                        continue;

                g_snprintf (action_name, sizeof (action_name), "RecentFile%u", n_items);

                display_name = gtk_recent_info_get_display_name (info);
                escaped_label = escape_label_for_menu (display_name);

                label = g_strdup_printf ("_%d.  %s", n_items + 1, escaped_label);
                g_free (escaped_label);

                action = gtk_action_new (action_name, label, NULL, NULL);
                g_object_set_data_full (G_OBJECT (action), "recent-info",
                                        gtk_recent_info_ref (info),
                                        (GDestroyNotify) gtk_recent_info_unref);
                g_signal_connect (G_OBJECT (action), "activate",
                                  G_CALLBACK (on_recent_file_item_activated),
                                  xplayer);

                mime_type = gtk_recent_info_get_mime_type (info);
                content_type = g_content_type_from_mime_type (mime_type);
                if (content_type != NULL) {
                        icon = g_content_type_get_icon (content_type);
                        g_free (content_type);
                }
                if (icon != NULL) {
                        gtk_action_set_gicon (action, icon);
                        gtk_action_set_always_show_image (action, TRUE);
                        g_object_unref (icon);
                }

                gtk_action_group_add_action (xplayer->recent_action_group,
                                            action);
                g_object_unref (action);

                gtk_ui_manager_add_ui (xplayer->ui_manager, xplayer->recent_ui_id,
                                      "/tmw-menubar/movie/recent-placeholder",
                                      label, action_name, GTK_UI_MANAGER_MENUITEM,
                                      FALSE);
                g_free (label);

                if (++n_items == 5)
                        break;
        }

        g_list_foreach (xplayer_items, (GFunc) gtk_recent_info_unref, NULL);
        g_list_free (xplayer_items);
}

void
xplayer_setup_recent (Xplayer *xplayer)
{
	xplayer->recent_manager = gtk_recent_manager_get_default ();
	xplayer->recent_action_group = NULL;
	xplayer->recent_ui_id = 0;

	g_signal_connect (G_OBJECT (xplayer->recent_manager), "changed",
			G_CALLBACK (xplayer_recent_manager_changed_callback),
			xplayer);

	xplayer_recent_manager_changed_callback (xplayer->recent_manager, xplayer);
}

void
open_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_open (xplayer);
}

void
open_location_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_open_location (xplayer);
}

void
eject_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_eject (xplayer);
}

void
properties_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_show_properties (xplayer);
}

void
play_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_play_pause (xplayer);
}

G_GNUC_NORETURN void
quit_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_exit (xplayer);
}

void
preferences_action_callback (GtkAction *action, Xplayer *xplayer)
{
	gtk_widget_show (xplayer->prefs);
}

void
fullscreen_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_fullscreen_toggle (xplayer);
}

void
zoom_1_2_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_set_scale_ratio (xplayer, 0.5);
}

void
zoom_1_1_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_set_scale_ratio (xplayer, 1);
}

void
zoom_2_1_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_set_scale_ratio (xplayer, 2);
}

void
zoom_toggle_action_callback (GtkToggleAction *action,
			     Xplayer           *xplayer)
{
	bacon_video_widget_set_zoom (xplayer->bvw,
				     gtk_toggle_action_get_active (action) ? BVW_ZOOM_EXPAND : BVW_ZOOM_NONE);
}

void
select_subtitle_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_playlist_select_subtitle_dialog (xplayer->playlist,
					       XPLAYER_PLAYLIST_DIALOG_PLAYING);
}

void
next_angle_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_next_angle (xplayer);
}

void
dvd_root_menu_action_callback (GtkAction *action, Xplayer *xplayer)
{
        bacon_video_widget_dvd_event (xplayer->bvw, BVW_DVD_ROOT_MENU);
}

void
dvd_title_menu_action_callback (GtkAction *action, Xplayer *xplayer)
{
        bacon_video_widget_dvd_event (xplayer->bvw, BVW_DVD_TITLE_MENU);
}

void
dvd_audio_menu_action_callback (GtkAction *action, Xplayer *xplayer)
{
        bacon_video_widget_dvd_event (xplayer->bvw, BVW_DVD_AUDIO_MENU);
}

void
dvd_angle_menu_action_callback (GtkAction *action, Xplayer *xplayer)
{
        bacon_video_widget_dvd_event (xplayer->bvw, BVW_DVD_ANGLE_MENU);
}

void
dvd_chapter_menu_action_callback (GtkAction *action, Xplayer *xplayer)
{
        bacon_video_widget_dvd_event (xplayer->bvw, BVW_DVD_CHAPTER_MENU);
}

void
next_chapter_action_callback (GtkAction *action, Xplayer *xplayer)
{
	XPLAYER_PROFILE (xplayer_action_next (xplayer));
}

void
previous_chapter_action_callback (GtkAction *action, Xplayer *xplayer)
{
	XPLAYER_PROFILE (xplayer_action_previous (xplayer));
}

void
skip_forward_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_seek_relative (xplayer, SEEK_FORWARD_OFFSET * 1000, FALSE);
}

void
skip_backwards_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_seek_relative (xplayer, SEEK_BACKWARD_OFFSET * 1000, FALSE);
}

void
volume_up_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_volume_relative (xplayer, VOLUME_UP_OFFSET);
}

void
volume_down_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_volume_relative (xplayer, VOLUME_DOWN_OFFSET);
}

void
contents_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_action_show_help (xplayer);
}

void
about_action_callback (GtkAction *action, Xplayer *xplayer)
{
	char *license = xplayer_interface_get_license ();

	gtk_show_about_dialog (GTK_WINDOW (xplayer->win),
				     "program-name", "Xplayer",
				     "version", VERSION,
				     "logo-icon-name", "xplayer",
				     "license", license,
				     "wrap-license", TRUE,
				     "website", PACKAGE_URL,
				     NULL);
	g_free (license);
}

static gboolean
xplayer_plugins_window_delete_cb (GtkWidget *window,
				   GdkEventAny *event,
				   gpointer data)
{
	gtk_widget_hide (window);

	return TRUE;
}

static void
xplayer_plugins_response_cb (GtkDialog *dialog,
			      int response_id,
			      gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (dialog));
}


void
plugins_action_callback (GtkAction *action, Xplayer *xplayer)
{
	if (xplayer->plugins == NULL) {
		GtkWidget *manager;

		xplayer->plugins = gtk_dialog_new_with_buttons (_("Configure Plugins"),
							      GTK_WINDOW (xplayer->win),
							      GTK_DIALOG_DESTROY_WITH_PARENT,
							      GTK_STOCK_CLOSE,
							      GTK_RESPONSE_CLOSE,
							      NULL);
		gtk_container_set_border_width (GTK_CONTAINER (xplayer->plugins), 5);
		gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (xplayer->plugins))), 2);

		g_signal_connect_object (G_OBJECT (xplayer->plugins),
					 "delete_event",
					 G_CALLBACK (xplayer_plugins_window_delete_cb),
					 NULL, 0);
		g_signal_connect_object (G_OBJECT (xplayer->plugins),
					 "response",
					 G_CALLBACK (xplayer_plugins_response_cb),
					 NULL, 0);

		manager = peas_gtk_plugin_manager_new (NULL);
		gtk_widget_show_all (GTK_WIDGET (manager));
		gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (xplayer->plugins))),
				    manager, TRUE, TRUE, 0);
		gtk_window_set_default_size (GTK_WINDOW (xplayer->plugins), 600, 400);
	}

	gtk_window_present (GTK_WINDOW (xplayer->plugins));
}

void
repeat_mode_action_callback (GtkToggleAction *action, Xplayer *xplayer)
{
	xplayer_playlist_set_repeat (xplayer->playlist,
			gtk_toggle_action_get_active (action));
}

void
shuffle_mode_action_callback (GtkToggleAction *action, Xplayer *xplayer)
{
	xplayer_playlist_set_shuffle (xplayer->playlist,
			gtk_toggle_action_get_active (action));
}

void
show_controls_action_callback (GtkToggleAction *action, Xplayer *xplayer)
{
	gboolean show;

	show = gtk_toggle_action_get_active (action);

	/* Let's update our controls visibility */
	if (show)
		xplayer->controls_visibility = XPLAYER_CONTROLS_VISIBLE;
	else
		xplayer->controls_visibility = XPLAYER_CONTROLS_HIDDEN;

	show_controls (xplayer, FALSE);
}

void
show_sidebar_action_callback (GtkToggleAction *action, Xplayer *xplayer)
{
	if (xplayer_is_fullscreen (xplayer))
		return;

	xplayer_sidebar_toggle (xplayer, gtk_toggle_action_get_active (action));
}

void
aspect_ratio_changed_callback (GtkRadioAction *action, GtkRadioAction *current, Xplayer *xplayer)
{
	xplayer_action_set_aspect_ratio (xplayer, gtk_radio_action_get_current_value (current));
}

void
clear_playlist_action_callback (GtkAction *action, Xplayer *xplayer)
{
	xplayer_playlist_clear (xplayer->playlist);
	xplayer_action_set_mrl (xplayer, NULL, NULL);
}

/* Show help in status bar when selecting (hovering over) a menu item. */
static void
menu_item_select_cb (GtkMenuItem *proxy, Xplayer *xplayer)
{
	GtkAction *action;
	const gchar *message;

	action = gtk_activatable_get_related_action (GTK_ACTIVATABLE (proxy));
	g_return_if_fail (action != NULL);

	message = gtk_action_get_tooltip (action);
	if (message)
		xplayer_statusbar_push_help (XPLAYER_STATUSBAR (xplayer->statusbar), message);
}

static void
menu_item_deselect_cb (GtkMenuItem *proxy, Xplayer *xplayer)
{
	xplayer_statusbar_pop_help (XPLAYER_STATUSBAR (xplayer->statusbar));
}

static void
setup_action (Xplayer *xplayer, GtkAction *action)
{
	GSList *proxies;
	for (proxies = gtk_action_get_proxies (action); proxies != NULL; proxies = proxies->next) {
		if (GTK_IS_MENU_ITEM (proxies->data)) {
			g_signal_connect (proxies->data, "select", G_CALLBACK (menu_item_select_cb), xplayer);
			g_signal_connect (proxies->data, "deselect", G_CALLBACK (menu_item_deselect_cb), xplayer);
		}

	}
}

static void
setup_menu_items (Xplayer *xplayer)
{
	GList *action_groups;

	/* FIXME: We can remove this once GTK+ bug #574001 is fixed */
	for (action_groups = gtk_ui_manager_get_action_groups (xplayer->ui_manager);
	     action_groups != NULL; action_groups = action_groups->next) {
		GtkActionGroup *action_group = GTK_ACTION_GROUP (action_groups->data);
		GList *actions;
		for (actions = gtk_action_group_list_actions (action_group); actions != NULL; actions = actions->next) {
			setup_action (xplayer, GTK_ACTION (actions->data));
		}
	}
}

void
xplayer_ui_manager_setup (Xplayer *xplayer)
{
	xplayer->main_action_group = GTK_ACTION_GROUP (gtk_builder_get_object (xplayer->xml, "main-action-group"));

	/* FIXME: Moving these to GtkBuilder depends on bug #457631 */
	if (gtk_widget_get_direction (xplayer->win) == GTK_TEXT_DIR_RTL) {
		GtkActionGroup *action_group = GTK_ACTION_GROUP (gtk_builder_get_object (xplayer->xml, "skip-action-group"));
		GtkAction *action;

		action = gtk_action_group_get_action (action_group, "skip-forward");
		gtk_action_set_accel_path (action, "Left");

		action = gtk_action_group_get_action (action_group, "skip-backwards");
		gtk_action_set_accel_path (action, "Right");
	}

	xplayer->ui_manager = GTK_UI_MANAGER (gtk_builder_get_object (xplayer->xml, "xplayer-ui-manager"));

	setup_menu_items (xplayer);

	xplayer->devices_action_group = NULL;
	xplayer->devices_ui_id = gtk_ui_manager_new_merge_id (xplayer->ui_manager);
	xplayer->languages_action_group = NULL;
	xplayer->languages_ui_id = gtk_ui_manager_new_merge_id (xplayer->ui_manager);
	xplayer->subtitles_action_group = NULL;
	xplayer->subtitles_ui_id = gtk_ui_manager_new_merge_id (xplayer->ui_manager);
}

