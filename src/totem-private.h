/* 
 * Copyright (C) 2001-2002 Bastien Nocera <hadess@hadess.net>
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
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

#ifndef __TOTEM_PRIVATE_H__
#define __TOTEM_PRIVATE_H__

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "totem-playlist.h"
#include "backend/bacon-video-widget.h"
#include "totem-open-location.h"
#include "totem-fullscreen.h"
#include "totem-plugins-engine.h"

#define totem_signal_block_by_data(obj, data) (g_signal_handlers_block_matched (obj, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, data))
#define totem_signal_unblock_by_data(obj, data) (g_signal_handlers_unblock_matched (obj, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, data))

#define totem_set_sensitivity(xml, name, state)					\
	{									\
		GtkWidget *widget;						\
		widget = GTK_WIDGET (gtk_builder_get_object (xml, name));	\
		gtk_widget_set_sensitive (widget, state);			\
	}
#define totem_main_set_sensitivity(name, state) totem_set_sensitivity (totem->xml, name, state)

#define totem_action_set_sensitivity(name, state)					\
	{										\
		GtkAction *__action;							\
		__action = gtk_action_group_get_action (totem->main_action_group, name);\
		gtk_action_set_sensitive (__action, state);				\
	}

typedef enum {
	TOTEM_CONTROLS_UNDEFINED,
	TOTEM_CONTROLS_VISIBLE,
	TOTEM_CONTROLS_HIDDEN,
	TOTEM_CONTROLS_FULLSCREEN
} ControlsVisibility;

typedef enum {
	STATE_PLAYING,
	STATE_PAUSED,
	STATE_STOPPED
} TotemStates;

struct _TotemObject {
	GtkApplication parent;

	/* Control window */
	GtkBuilder *xml;
	GtkWidget *win;
	BaconVideoWidget *bvw;
	GtkWidget *prefs;
	GtkBuilder *prefs_xml;
	GtkWidget *statusbar;

	/* UI manager */
	GtkActionGroup *main_action_group;
	GtkUIManager *ui_manager;

	GtkActionGroup *devices_action_group;
	guint devices_ui_id;

	GtkActionGroup *languages_action_group;
	guint languages_ui_id;

	GtkActionGroup *subtitles_action_group;
	guint subtitles_ui_id;

	/* Plugins */
	GtkWidget *plugins;
	TotemPluginsEngine *engine;

	/* Sidebar */
	GtkWidget *sidebar;
	gboolean sidebar_shown;
	int sidebar_w;

	/* Seek */
	GtkWidget *seek;
	GtkAdjustment *seekadj;
	gboolean seek_lock;
	gboolean seekable;

	/* Volume */
	GtkWidget *volume;
	gboolean volume_sensitive;
	gboolean muted;
	double prev_volume;

	/* Subtitles/Languages menus */
	GtkWidget *subtitles;
	GtkWidget *languages;
	GList *subtitles_list;
	GList *language_list;

	/* Fullscreen */
	TotemFullscreen *fs;

	/* controls management */
	ControlsVisibility controls_visibility;

	/* Stream info */
	gint64 stream_length;

	/* recent file stuff */
	GtkRecentManager *recent_manager;
	GtkActionGroup *recent_action_group;
	guint recent_ui_id;

	/* Monitor for playlist unmounts and drives/volumes monitoring */
	GVolumeMonitor *monitor;
	gboolean drives_changed;

	/* session */
	const char *argv0;
	gint64 seek_to_start;
	guint index;
	gboolean session_restored;

	/* Window State */
	int window_w, window_h;
	gboolean maximised;

	/* other */
	char *mrl;
	gint64 seek_to;
	TotemPlaylist *playlist;
	GSettings *settings;
	TotemStates state;
	TotemOpenLocation *open_location;
	gboolean remember_position;
	gboolean disable_kbd_shortcuts;
	gboolean has_played_emitted;
};

GtkWidget *totem_volume_create (void);

#define SEEK_FORWARD_OFFSET 60
#define SEEK_BACKWARD_OFFSET -15

#define VOLUME_DOWN_OFFSET (-0.08)
#define VOLUME_UP_OFFSET (0.08)

#define VOLUME_DOWN_SHORT_OFFSET (-0.02)
#define VOLUME_UP_SHORT_OFFSET (0.02)

#define ZOOM_IN_OFFSET 0.01
#define ZOOM_OUT_OFFSET -0.01

void	totem_action_open			(Totem *totem);
void	totem_action_open_location		(Totem *totem);
void	totem_action_eject			(Totem *totem);
void	totem_action_set_zoom			(Totem *totem, gboolean zoom);
void	totem_action_show_help			(Totem *totem);
void	totem_action_show_properties		(Totem *totem);
gboolean totem_action_open_files		(Totem *totem, char **list);
G_GNUC_NORETURN void totem_action_error_and_exit (const char *title, const char *reason, Totem *totem);

void	show_controls				(Totem *totem, gboolean was_fullscreen);

char	*totem_setup_window			(Totem *totem);
void	totem_callback_connect			(Totem *totem);
void	playlist_widget_setup			(Totem *totem);
void	video_widget_create			(Totem *totem);

#endif /* __TOTEM_PRIVATE_H__ */
