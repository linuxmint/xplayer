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
 * The Xplayer project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Xplayer. This
 * permission are above and beyond the permissions granted by the GPL license
 * Xplayer is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

#ifndef __XPLAYER_PRIVATE_H__
#define __XPLAYER_PRIVATE_H__

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "xplayer-playlist.h"
#include "backend/bacon-video-widget.h"
#include "xplayer-open-location.h"
#include "xplayer-fullscreen.h"
#include "xplayer-plugins-engine.h"
#include "xplayer-time-label.h"

#define xplayer_signal_block_by_data(obj, data) (g_signal_handlers_block_matched (obj, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, data))
#define xplayer_signal_unblock_by_data(obj, data) (g_signal_handlers_unblock_matched (obj, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, data))

#define xplayer_set_sensitivity(xml, name, state)					\
	{									\
		GtkWidget *widget;						\
		widget = GTK_WIDGET (gtk_builder_get_object (xml, name));	\
		gtk_widget_set_sensitive (widget, state);			\
	}
#define xplayer_main_set_sensitivity(name, state) xplayer_set_sensitivity (xplayer->xml, name, state)

#define xplayer_action_set_sensitivity(name, state)					\
	{										\
		GtkAction *__action;							\
		__action = gtk_action_group_get_action (xplayer->main_action_group, name);\
		gtk_action_set_sensitive (__action, state);				\
	}

typedef enum {
	XPLAYER_CONTROLS_UNDEFINED,
	XPLAYER_CONTROLS_VISIBLE,
	XPLAYER_CONTROLS_HIDDEN,
	XPLAYER_CONTROLS_FULLSCREEN
} ControlsVisibility;

typedef enum {
	STATE_PLAYING,
	STATE_PAUSED,
	STATE_STOPPED
} XplayerStates;

struct _XplayerObject {
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
	XplayerPluginsEngine *engine;

	/* Sidebar */
	GtkWidget *sidebar;
	gboolean sidebar_shown;
	int sidebar_w;

	/* Seek */
	GtkWidget *seek;
	GtkAdjustment *seekadj;
	gboolean seek_lock;
	gboolean seekable;
	XplayerTimeLabel *time_label;

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
	XplayerFullscreen *fs;

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
	XplayerPlaylist *playlist;
	GSettings *settings;
	XplayerStates state;
	XplayerOpenLocation *open_location;
	gboolean remember_position;
	gboolean disable_kbd_shortcuts;
	gboolean has_played_emitted;
};

GtkWidget *xplayer_volume_create (void);

#define SEEK_FORWARD_OFFSET 60
#define SEEK_BACKWARD_OFFSET -15

#define VOLUME_DOWN_OFFSET (-0.08)
#define VOLUME_UP_OFFSET (0.08)

#define VOLUME_DOWN_SHORT_OFFSET (-0.02)
#define VOLUME_UP_SHORT_OFFSET (0.02)

#define ZOOM_IN_OFFSET 0.01
#define ZOOM_OUT_OFFSET -0.01

void	xplayer_action_open			(Xplayer *xplayer);
void	xplayer_action_open_location		(Xplayer *xplayer);
void	xplayer_action_eject			(Xplayer *xplayer);
void	xplayer_action_set_zoom			(Xplayer *xplayer, gboolean zoom);
void	xplayer_action_show_help			(Xplayer *xplayer);
void	xplayer_action_show_properties		(Xplayer *xplayer);
gboolean xplayer_action_open_files		(Xplayer *xplayer, char **list);
G_GNUC_NORETURN void xplayer_action_error_and_exit (const char *title, const char *reason, Xplayer *xplayer);

void	show_controls				(Xplayer *xplayer, gboolean was_fullscreen);

char	*xplayer_setup_window			(Xplayer *xplayer);
void	xplayer_callback_connect			(Xplayer *xplayer);
void	playlist_widget_setup			(Xplayer *xplayer);
void	video_widget_create			(Xplayer *xplayer);

#endif /* __XPLAYER_PRIVATE_H__ */
