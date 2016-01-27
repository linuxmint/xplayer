/* 
 * Copyright (C) 2001,2002,2003,2004,2005 Bastien Nocera <hadess@hadess.net>
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

#ifndef __TOTEM_H__
#define __TOTEM_H__

#include <glib-object.h>
#include <gtk/gtk.h>

/**
 * TOTEM_GSETTINGS_SCHEMA:
 *
 * The GSettings schema under which all Totem settings are stored.
 **/
#define TOTEM_GSETTINGS_SCHEMA "org.gnome.totem"

G_BEGIN_DECLS

/**
 * TotemRemoteCommand:
 * @TOTEM_REMOTE_COMMAND_UNKNOWN: unknown command
 * @TOTEM_REMOTE_COMMAND_PLAY: play the current stream
 * @TOTEM_REMOTE_COMMAND_PAUSE: pause the current stream
 * @TOTEM_REMOTE_COMMAND_STOP: stop playing the current stream
 * @TOTEM_REMOTE_COMMAND_PLAYPAUSE: toggle play/pause on the current stream
 * @TOTEM_REMOTE_COMMAND_NEXT: play the next playlist item
 * @TOTEM_REMOTE_COMMAND_PREVIOUS: play the previous playlist item
 * @TOTEM_REMOTE_COMMAND_SEEK_FORWARD: seek forwards in the current stream
 * @TOTEM_REMOTE_COMMAND_SEEK_BACKWARD: seek backwards in the current stream
 * @TOTEM_REMOTE_COMMAND_VOLUME_UP: increase the volume
 * @TOTEM_REMOTE_COMMAND_VOLUME_DOWN: decrease the volume
 * @TOTEM_REMOTE_COMMAND_FULLSCREEN: toggle fullscreen mode
 * @TOTEM_REMOTE_COMMAND_QUIT: quit the instance of Totem
 * @TOTEM_REMOTE_COMMAND_ENQUEUE: enqueue a new playlist item
 * @TOTEM_REMOTE_COMMAND_REPLACE: replace an item in the playlist
 * @TOTEM_REMOTE_COMMAND_SHOW: show the Totem instance
 * @TOTEM_REMOTE_COMMAND_TOGGLE_CONTROLS: toggle the control visibility
 * @TOTEM_REMOTE_COMMAND_UP: go up (DVD controls)
 * @TOTEM_REMOTE_COMMAND_DOWN: go down (DVD controls)
 * @TOTEM_REMOTE_COMMAND_LEFT: go left (DVD controls)
 * @TOTEM_REMOTE_COMMAND_RIGHT: go right (DVD controls)
 * @TOTEM_REMOTE_COMMAND_SELECT: select the current item (DVD controls)
 * @TOTEM_REMOTE_COMMAND_DVD_MENU: go to the DVD menu
 * @TOTEM_REMOTE_COMMAND_ZOOM_UP: increase the zoom level
 * @TOTEM_REMOTE_COMMAND_ZOOM_DOWN: decrease the zoom level
 * @TOTEM_REMOTE_COMMAND_EJECT: eject the current disc
 * @TOTEM_REMOTE_COMMAND_PLAY_DVD: play a DVD in a drive
 * @TOTEM_REMOTE_COMMAND_MUTE: toggle mute
 * @TOTEM_REMOTE_COMMAND_TOGGLE_ASPECT: toggle the aspect ratio
 *
 * Represents a command which can be sent to a running Totem instance remotely.
 **/
typedef enum {
	TOTEM_REMOTE_COMMAND_UNKNOWN = 0,
	TOTEM_REMOTE_COMMAND_PLAY,
	TOTEM_REMOTE_COMMAND_PAUSE,
	TOTEM_REMOTE_COMMAND_STOP,
	TOTEM_REMOTE_COMMAND_PLAYPAUSE,
	TOTEM_REMOTE_COMMAND_NEXT,
	TOTEM_REMOTE_COMMAND_PREVIOUS,
	TOTEM_REMOTE_COMMAND_SEEK_FORWARD,
	TOTEM_REMOTE_COMMAND_SEEK_BACKWARD,
	TOTEM_REMOTE_COMMAND_VOLUME_UP,
	TOTEM_REMOTE_COMMAND_VOLUME_DOWN,
	TOTEM_REMOTE_COMMAND_FULLSCREEN,
	TOTEM_REMOTE_COMMAND_QUIT,
	TOTEM_REMOTE_COMMAND_ENQUEUE,
	TOTEM_REMOTE_COMMAND_REPLACE,
	TOTEM_REMOTE_COMMAND_SHOW,
	TOTEM_REMOTE_COMMAND_TOGGLE_CONTROLS,
	TOTEM_REMOTE_COMMAND_UP,
	TOTEM_REMOTE_COMMAND_DOWN,
	TOTEM_REMOTE_COMMAND_LEFT,
	TOTEM_REMOTE_COMMAND_RIGHT,
	TOTEM_REMOTE_COMMAND_SELECT,
	TOTEM_REMOTE_COMMAND_DVD_MENU,
	TOTEM_REMOTE_COMMAND_ZOOM_UP,
	TOTEM_REMOTE_COMMAND_ZOOM_DOWN,
	TOTEM_REMOTE_COMMAND_EJECT,
	TOTEM_REMOTE_COMMAND_PLAY_DVD,
	TOTEM_REMOTE_COMMAND_MUTE,
	TOTEM_REMOTE_COMMAND_TOGGLE_ASPECT
} TotemRemoteCommand;

/**
 * TotemRemoteSetting:
 * @TOTEM_REMOTE_SETTING_SHUFFLE: whether shuffle is enabled
 * @TOTEM_REMOTE_SETTING_REPEAT: whether repeat is enabled
 *
 * Represents a boolean setting or preference on a remote Totem instance.
 **/
typedef enum {
	TOTEM_REMOTE_SETTING_SHUFFLE,
	TOTEM_REMOTE_SETTING_REPEAT
} TotemRemoteSetting;

GType totem_remote_command_get_type	(void);
GQuark totem_remote_command_quark	(void);
#define TOTEM_TYPE_REMOTE_COMMAND	(totem_remote_command_get_type())
#define TOTEM_REMOTE_COMMAND		totem_remote_command_quark ()

GType totem_remote_setting_get_type	(void);
GQuark totem_remote_setting_quark	(void);
#define TOTEM_TYPE_REMOTE_SETTING	(totem_remote_setting_get_type())
#define TOTEM_REMOTE_SETTING		totem_remote_setting_quark ()

#define TOTEM_TYPE_OBJECT              (totem_object_get_type ())
#define TOTEM_OBJECT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), totem_object_get_type (), TotemObject))
#define TOTEM_OBJECT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), totem_object_get_type (), TotemObjectClass))
#define TOTEM_IS_OBJECT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE (obj, totem_object_get_type ()))
#define TOTEM_IS_OBJECT_CLASS(klass)   (G_CHECK_INSTANCE_GET_CLASS ((klass), totem_object_get_type ()))

/**
 * Totem:
 *
 * The #Totem object is a handy synonym for #TotemObject, and the two can be used interchangably.
 **/

/**
 * TotemObject:
 *
 * All the fields in the #TotemObject structure are private and should never be accessed directly.
 **/
typedef struct _TotemObject Totem;
typedef struct _TotemObject TotemObject;

typedef struct {
	GtkApplicationClass parent_class;

	void (*file_opened)			(TotemObject *totem, const char *mrl);
	void (*file_closed)			(TotemObject *totem);
	void (*file_has_played)			(TotemObject *totem, const char *mrl);
	void (*metadata_updated)		(TotemObject *totem,
						 const char *artist,
						 const char *title,
						 const char *album,
						 guint track_num);
	char * (*get_user_agent)		(TotemObject *totem,
						 const char  *mrl);
	char * (*get_text_subtitle)		(TotemObject *totem,
						 const char  *mrl);
} TotemObjectClass;

GType	totem_object_get_type			(void);
void    totem_object_plugins_init		(TotemObject *totem);
void    totem_object_plugins_shutdown		(TotemObject *totem);
void	totem_file_opened			(TotemObject *totem,
						 const char *mrl);
void	totem_file_has_played			(TotemObject *totem,
						 const char *mrl);
void	totem_file_closed			(TotemObject *totem);
void	totem_metadata_updated			(TotemObject *totem,
						 const char *artist,
						 const char *title,
						 const char *album,
						 guint track_num);

#define totem_action_exit totem_object_action_exit
void	totem_object_action_exit		(TotemObject *totem) G_GNUC_NORETURN;
#define totem_action_play totem_object_action_play
void	totem_object_action_play		(TotemObject *totem);
#define totem_action_stop totem_object_action_stop
void	totem_object_action_stop		(TotemObject *totem);
#define totem_action_play_pause totem_object_action_play_pause
void	totem_object_action_play_pause		(TotemObject *totem);
void	totem_action_pause			(TotemObject *totem);
#define totem_action_fullscreen_toggle totem_object_action_fullscreen_toggle
void	totem_object_action_fullscreen_toggle	(TotemObject *totem);
void	totem_action_fullscreen			(TotemObject *totem, gboolean state);
#define totem_action_next totem_object_action_next
void	totem_object_action_next		(TotemObject *totem);
#define totem_action_previous totem_object_action_previous
void	totem_object_action_previous		(TotemObject *totem);
#define totem_action_seek_time totem_object_action_seek_time
void	totem_object_action_seek_time		(TotemObject *totem, gint64 msec, gboolean accurate);
void	totem_action_seek_relative		(TotemObject *totem, gint64 offset, gboolean accurate);
#define totem_get_volume totem_object_get_volume
double	totem_object_get_volume			(TotemObject *totem);
#define totem_action_volume totem_object_action_volume
void	totem_object_action_volume		(TotemObject *totem, double volume);
void	totem_action_volume_relative		(TotemObject *totem, double off_pct);
void	totem_action_volume_toggle_mute		(TotemObject *totem);
gboolean totem_action_set_mrl			(TotemObject *totem,
						 const char *mrl,
						 const char *subtitle);
void	totem_action_set_mrl_and_play		(TotemObject *totem,
						 const char *mrl, 
						 const char *subtitle);

gboolean totem_action_set_mrl_with_warning	(TotemObject *totem,
						 const char *mrl,
						 const char *subtitle,
						 gboolean warn);

void	totem_action_toggle_aspect_ratio	(TotemObject *totem);
void	totem_action_set_aspect_ratio		(TotemObject *totem, int ratio);
int	totem_action_get_aspect_ratio		(TotemObject *totem);
void	totem_action_toggle_controls		(TotemObject *totem);
void	totem_action_next_angle			(TotemObject *totem);

void	totem_action_set_scale_ratio		(TotemObject *totem, gfloat ratio);
#define totem_action_error totem_object_action_error
void    totem_object_action_error               (TotemObject *totem,
						 const char *title,
						 const char *reason);

gboolean totem_is_fullscreen			(TotemObject *totem);
#define totem_is_playing totem_object_is_playing
gboolean totem_object_is_playing		(TotemObject *totem);
#define totem_is_paused totem_object_is_paused
gboolean totem_object_is_paused			(TotemObject *totem);
#define totem_is_seekable totem_object_is_seekable
gboolean totem_object_is_seekable		(TotemObject *totem);
#define totem_get_main_window totem_object_get_main_window
GtkWindow *totem_object_get_main_window		(TotemObject *totem);
#define totem_get_ui_manager totem_object_get_ui_manager
GtkUIManager *totem_object_get_ui_manager	(TotemObject *totem);
 #define totem_get_video_widget totem_object_get_video_widget
GtkWidget *totem_object_get_video_widget	(TotemObject *totem);
#define totem_get_version totem_object_get_version
char *totem_object_get_version			(void);

/* Current media information */
char *	totem_get_short_title			(TotemObject *totem);
gint64	totem_get_current_time			(TotemObject *totem);

/* Playlist handling */
#define totem_get_playlist_length totem_object_get_playlist_length
guint	totem_object_get_playlist_length	(TotemObject *totem);
void	totem_action_set_playlist_index		(TotemObject *totem,
						 guint index);
#define totem_get_playlist_pos totem_object_get_playlist_pos
int	totem_object_get_playlist_pos		(TotemObject *totem);
#define totem_get_title_at_playlist_pos totem_object_get_title_at_playlist_pos
char *	totem_object_get_title_at_playlist_pos	(TotemObject *totem,
						 guint playlist_index);
#define totem_add_to_playlist_and_play totem_object_add_to_playlist_and_play
void totem_object_add_to_playlist_and_play	(TotemObject *totem,
						 const char *uri,
						 const char *display_name);
#define totem_get_current_mrl totem_object_get_current_mrl
char *  totem_object_get_current_mrl		(TotemObject *totem);
#define totem_set_current_subtitle totem_object_set_current_subtitle
void	totem_object_set_current_subtitle	(TotemObject *totem,
						 const char *subtitle_uri);
/* Sidebar handling */
#define totem_add_sidebar_page totem_object_add_sidebar_page
void    totem_object_add_sidebar_page		(TotemObject *totem,
						 const char *page_id,
						 const char *title,
						 GtkWidget *main_widget);
#define totem_remove_sidebar_page totem_object_remove_sidebar_page
void    totem_object_remove_sidebar_page	(TotemObject *totem,
						 const char *page_id);

/* Remote actions */
#define totem_action_remote totem_object_action_remote
void    totem_object_action_remote		(TotemObject *totem,
						 TotemRemoteCommand cmd,
						 const char *url);
#define totem_action_remote_set_setting totem_object_action_remote_set_setting
void	totem_object_action_remote_set_setting	(TotemObject *totem,
						 TotemRemoteSetting setting,
						 gboolean value);
#define totem_action_remote_get_setting totem_object_action_remote_get_setting
gboolean totem_object_action_remote_get_setting	(TotemObject *totem,
						 TotemRemoteSetting setting);

const gchar * const *totem_object_get_supported_content_types (void);
const gchar * const *totem_object_get_supported_uri_schemes (void);

#endif /* __TOTEM_H__ */
