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
 * The Xplayer project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Xplayer. This
 * permission are above and beyond the permissions granted by the GPL license
 * Xplayer is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

#ifndef __XPLAYER_H__
#define __XPLAYER_H__

#include <glib-object.h>
#include <gtk/gtk.h>

/**
 * XPLAYER_GSETTINGS_SCHEMA:
 *
 * The GSettings schema under which all Xplayer settings are stored.
 **/
#define XPLAYER_GSETTINGS_SCHEMA "org.x.player"

G_BEGIN_DECLS

/**
 * XplayerRemoteCommand:
 * @XPLAYER_REMOTE_COMMAND_UNKNOWN: unknown command
 * @XPLAYER_REMOTE_COMMAND_PLAY: play the current stream
 * @XPLAYER_REMOTE_COMMAND_PAUSE: pause the current stream
 * @XPLAYER_REMOTE_COMMAND_STOP: stop playing the current stream
 * @XPLAYER_REMOTE_COMMAND_PLAYPAUSE: toggle play/pause on the current stream
 * @XPLAYER_REMOTE_COMMAND_NEXT: play the next playlist item
 * @XPLAYER_REMOTE_COMMAND_PREVIOUS: play the previous playlist item
 * @XPLAYER_REMOTE_COMMAND_SEEK_FORWARD: seek forwards in the current stream
 * @XPLAYER_REMOTE_COMMAND_SEEK_BACKWARD: seek backwards in the current stream
 * @XPLAYER_REMOTE_COMMAND_VOLUME_UP: increase the volume
 * @XPLAYER_REMOTE_COMMAND_VOLUME_DOWN: decrease the volume
 * @XPLAYER_REMOTE_COMMAND_FULLSCREEN: toggle fullscreen mode
 * @XPLAYER_REMOTE_COMMAND_QUIT: quit the instance of Xplayer
 * @XPLAYER_REMOTE_COMMAND_ENQUEUE: enqueue a new playlist item
 * @XPLAYER_REMOTE_COMMAND_REPLACE: replace an item in the playlist
 * @XPLAYER_REMOTE_COMMAND_SHOW: show the Xplayer instance
 * @XPLAYER_REMOTE_COMMAND_TOGGLE_CONTROLS: toggle the control visibility
 * @XPLAYER_REMOTE_COMMAND_UP: go up (DVD controls)
 * @XPLAYER_REMOTE_COMMAND_DOWN: go down (DVD controls)
 * @XPLAYER_REMOTE_COMMAND_LEFT: go left (DVD controls)
 * @XPLAYER_REMOTE_COMMAND_RIGHT: go right (DVD controls)
 * @XPLAYER_REMOTE_COMMAND_SELECT: select the current item (DVD controls)
 * @XPLAYER_REMOTE_COMMAND_DVD_MENU: go to the DVD menu
 * @XPLAYER_REMOTE_COMMAND_ZOOM_UP: increase the zoom level
 * @XPLAYER_REMOTE_COMMAND_ZOOM_DOWN: decrease the zoom level
 * @XPLAYER_REMOTE_COMMAND_EJECT: eject the current disc
 * @XPLAYER_REMOTE_COMMAND_PLAY_DVD: play a DVD in a drive
 * @XPLAYER_REMOTE_COMMAND_MUTE: toggle mute
 * @XPLAYER_REMOTE_COMMAND_TOGGLE_ASPECT: toggle the aspect ratio
 *
 * Represents a command which can be sent to a running Xplayer instance remotely.
 **/
typedef enum {
	XPLAYER_REMOTE_COMMAND_UNKNOWN = 0,
	XPLAYER_REMOTE_COMMAND_PLAY,
	XPLAYER_REMOTE_COMMAND_PAUSE,
	XPLAYER_REMOTE_COMMAND_STOP,
	XPLAYER_REMOTE_COMMAND_PLAYPAUSE,
	XPLAYER_REMOTE_COMMAND_NEXT,
	XPLAYER_REMOTE_COMMAND_PREVIOUS,
	XPLAYER_REMOTE_COMMAND_SEEK_FORWARD,
	XPLAYER_REMOTE_COMMAND_SEEK_BACKWARD,
	XPLAYER_REMOTE_COMMAND_VOLUME_UP,
	XPLAYER_REMOTE_COMMAND_VOLUME_DOWN,
	XPLAYER_REMOTE_COMMAND_FULLSCREEN,
	XPLAYER_REMOTE_COMMAND_QUIT,
	XPLAYER_REMOTE_COMMAND_ENQUEUE,
	XPLAYER_REMOTE_COMMAND_REPLACE,
	XPLAYER_REMOTE_COMMAND_SHOW,
	XPLAYER_REMOTE_COMMAND_TOGGLE_CONTROLS,
	XPLAYER_REMOTE_COMMAND_UP,
	XPLAYER_REMOTE_COMMAND_DOWN,
	XPLAYER_REMOTE_COMMAND_LEFT,
	XPLAYER_REMOTE_COMMAND_RIGHT,
	XPLAYER_REMOTE_COMMAND_SELECT,
	XPLAYER_REMOTE_COMMAND_DVD_MENU,
	XPLAYER_REMOTE_COMMAND_ZOOM_UP,
	XPLAYER_REMOTE_COMMAND_ZOOM_DOWN,
	XPLAYER_REMOTE_COMMAND_EJECT,
	XPLAYER_REMOTE_COMMAND_PLAY_DVD,
	XPLAYER_REMOTE_COMMAND_MUTE,
	XPLAYER_REMOTE_COMMAND_TOGGLE_ASPECT
} XplayerRemoteCommand;

/**
 * XplayerRemoteSetting:
 * @XPLAYER_REMOTE_SETTING_SHUFFLE: whether shuffle is enabled
 * @XPLAYER_REMOTE_SETTING_REPEAT: whether repeat is enabled
 *
 * Represents a boolean setting or preference on a remote Xplayer instance.
 **/
typedef enum {
	XPLAYER_REMOTE_SETTING_SHUFFLE,
	XPLAYER_REMOTE_SETTING_REPEAT
} XplayerRemoteSetting;

GType xplayer_remote_command_get_type	(void);
GQuark xplayer_remote_command_quark	(void);
#define XPLAYER_TYPE_REMOTE_COMMAND	(xplayer_remote_command_get_type())
#define XPLAYER_REMOTE_COMMAND		xplayer_remote_command_quark ()

GType xplayer_remote_setting_get_type	(void);
GQuark xplayer_remote_setting_quark	(void);
#define XPLAYER_TYPE_REMOTE_SETTING	(xplayer_remote_setting_get_type())
#define XPLAYER_REMOTE_SETTING		xplayer_remote_setting_quark ()

#define XPLAYER_TYPE_OBJECT              (xplayer_object_get_type ())
#define XPLAYER_OBJECT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), xplayer_object_get_type (), XplayerObject))
#define XPLAYER_OBJECT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), xplayer_object_get_type (), XplayerObjectClass))
#define XPLAYER_IS_OBJECT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE (obj, xplayer_object_get_type ()))
#define XPLAYER_IS_OBJECT_CLASS(klass)   (G_CHECK_INSTANCE_GET_CLASS ((klass), xplayer_object_get_type ()))

/**
 * Xplayer:
 *
 * The #Xplayer object is a handy synonym for #XplayerObject, and the two can be used interchangably.
 **/

/**
 * XplayerObject:
 *
 * All the fields in the #XplayerObject structure are private and should never be accessed directly.
 **/
typedef struct _XplayerObject Xplayer;
typedef struct _XplayerObject XplayerObject;

typedef struct {
	GtkApplicationClass parent_class;

	void (*file_opened)			(XplayerObject *xplayer, const char *mrl);
	void (*file_closed)			(XplayerObject *xplayer);
	void (*file_has_played)			(XplayerObject *xplayer, const char *mrl);
	void (*metadata_updated)		(XplayerObject *xplayer,
						 const char *artist,
						 const char *title,
						 const char *album,
						 guint track_num);
	char * (*get_user_agent)		(XplayerObject *xplayer,
						 const char  *mrl);
	char * (*get_text_subtitle)		(XplayerObject *xplayer,
						 const char  *mrl);
} XplayerObjectClass;

GType	xplayer_object_get_type			(void);
void    xplayer_object_plugins_init		(XplayerObject *xplayer);
void    xplayer_object_plugins_shutdown		(XplayerObject *xplayer);
void	xplayer_file_opened			(XplayerObject *xplayer,
						 const char *mrl);
void	xplayer_file_has_played			(XplayerObject *xplayer,
						 const char *mrl);
void	xplayer_file_closed			(XplayerObject *xplayer);
void	xplayer_metadata_updated			(XplayerObject *xplayer,
						 const char *artist,
						 const char *title,
						 const char *album,
						 guint track_num);

#define xplayer_action_exit xplayer_object_action_exit
void	xplayer_object_action_exit		(XplayerObject *xplayer) G_GNUC_NORETURN;
#define xplayer_action_play xplayer_object_action_play
void	xplayer_object_action_play		(XplayerObject *xplayer);
#define xplayer_action_stop xplayer_object_action_stop
void	xplayer_object_action_stop		(XplayerObject *xplayer);
#define xplayer_action_play_pause xplayer_object_action_play_pause
void	xplayer_object_action_play_pause		(XplayerObject *xplayer);
#define xplayer_action_pause xplayer_object_action_pause
void	xplayer_object_action_pause			(XplayerObject *xplayer);
#define xplayer_action_fullscreen_toggle xplayer_object_action_fullscreen_toggle
void	xplayer_object_action_fullscreen_toggle	(XplayerObject *xplayer);
void	xplayer_action_fullscreen			(XplayerObject *xplayer, gboolean state);
void	xplayer_action_blank			    (XplayerObject *xplayer);
#define xplayer_action_next xplayer_object_action_next
void	xplayer_object_action_next		(XplayerObject *xplayer);
#define xplayer_action_previous xplayer_object_action_previous
void	xplayer_object_action_previous		(XplayerObject *xplayer);
#define xplayer_action_seek_time xplayer_object_action_seek_time
void	xplayer_object_action_seek_time		(XplayerObject *xplayer, gint64 msec, gboolean accurate);
void	xplayer_action_seek_relative		(XplayerObject *xplayer, gint64 offset, gboolean accurate);
#define xplayer_get_volume xplayer_object_get_volume
double	xplayer_object_get_volume			(XplayerObject *xplayer);
#define xplayer_action_volume xplayer_object_action_volume
void	xplayer_object_action_volume		(XplayerObject *xplayer, double volume);
void	xplayer_action_volume_relative		(XplayerObject *xplayer, double off_pct);
void	xplayer_action_volume_toggle_mute		(XplayerObject *xplayer);
gboolean xplayer_action_set_mrl			(XplayerObject *xplayer,
						 const char *mrl,
						 const char *subtitle);
void	xplayer_action_set_mrl_and_play		(XplayerObject *xplayer,
						 const char *mrl, 
						 const char *subtitle);

gboolean xplayer_action_set_mrl_with_warning	(XplayerObject *xplayer,
						 const char *mrl,
						 const char *subtitle,
						 gboolean warn);

void	xplayer_action_toggle_aspect_ratio	(XplayerObject *xplayer);
void	xplayer_action_set_aspect_ratio		(XplayerObject *xplayer, int ratio);
int	xplayer_action_get_aspect_ratio		(XplayerObject *xplayer);
void	xplayer_action_toggle_controls		(XplayerObject *xplayer);
void	xplayer_action_next_angle			(XplayerObject *xplayer);

void	xplayer_action_set_scale_ratio		(XplayerObject *xplayer, gfloat ratio);
#define xplayer_action_error xplayer_object_action_error
void    xplayer_object_action_error               (XplayerObject *xplayer,
						 const char *title,
						 const char *reason);

#define xplayer_action_cycle_language xplayer_object_action_cycle_language
void	xplayer_object_action_cycle_language		(XplayerObject *xplayer);

#define xplayer_action_cycle_subtitle xplayer_object_action_cycle_subtitle
void	xplayer_object_action_cycle_subtitle		(XplayerObject *xplayer);

gboolean xplayer_is_fullscreen			(XplayerObject *xplayer);
#define xplayer_is_playing xplayer_object_is_playing
gboolean xplayer_object_is_playing		(XplayerObject *xplayer);
#define xplayer_is_paused xplayer_object_is_paused
gboolean xplayer_object_is_paused			(XplayerObject *xplayer);
#define xplayer_is_seekable xplayer_object_is_seekable
gboolean xplayer_object_is_seekable		(XplayerObject *xplayer);
#define xplayer_get_main_window xplayer_object_get_main_window
GtkWindow *xplayer_object_get_main_window		(XplayerObject *xplayer);
#define xplayer_get_ui_manager xplayer_object_get_ui_manager
GtkUIManager *xplayer_object_get_ui_manager	(XplayerObject *xplayer);
 #define xplayer_get_video_widget xplayer_object_get_video_widget
GtkWidget *xplayer_object_get_video_widget	(XplayerObject *xplayer);
#define xplayer_get_version xplayer_object_get_version
char *xplayer_object_get_version			(void);

/* Current media information */
char *	xplayer_get_short_title			(XplayerObject *xplayer);
gint64	xplayer_get_current_time			(XplayerObject *xplayer);

/* Playlist handling */
#define xplayer_get_playlist_length xplayer_object_get_playlist_length
guint	xplayer_object_get_playlist_length	(XplayerObject *xplayer);
void	xplayer_action_set_playlist_index		(XplayerObject *xplayer,
						 guint index);
#define xplayer_get_playlist_pos xplayer_object_get_playlist_pos
int	xplayer_object_get_playlist_pos		(XplayerObject *xplayer);
#define xplayer_get_title_at_playlist_pos xplayer_object_get_title_at_playlist_pos
char *	xplayer_object_get_title_at_playlist_pos	(XplayerObject *xplayer,
						 guint playlist_index);
#define xplayer_add_to_playlist_and_play xplayer_object_add_to_playlist_and_play
void xplayer_object_add_to_playlist_and_play	(XplayerObject *xplayer,
						 const char *uri,
						 const char *display_name);
#define xplayer_get_current_mrl xplayer_object_get_current_mrl
char *  xplayer_object_get_current_mrl		(XplayerObject *xplayer);
#define xplayer_set_current_subtitle xplayer_object_set_current_subtitle
void	xplayer_object_set_current_subtitle	(XplayerObject *xplayer,
						 const char *subtitle_uri);
/* Sidebar handling */
#define xplayer_add_sidebar_page xplayer_object_add_sidebar_page
void    xplayer_object_add_sidebar_page		(XplayerObject *xplayer,
						 const char *page_id,
						 const char *title,
						 GtkWidget *main_widget);
#define xplayer_remove_sidebar_page xplayer_object_remove_sidebar_page
void    xplayer_object_remove_sidebar_page	(XplayerObject *xplayer,
						 const char *page_id);

/* Remote actions */
#define xplayer_action_remote xplayer_object_action_remote
void    xplayer_object_action_remote		(XplayerObject *xplayer,
						 XplayerRemoteCommand cmd,
						 const char *url);
#define xplayer_action_remote_set_setting xplayer_object_action_remote_set_setting
void	xplayer_object_action_remote_set_setting	(XplayerObject *xplayer,
						 XplayerRemoteSetting setting,
						 gboolean value);
#define xplayer_action_remote_get_setting xplayer_object_action_remote_get_setting
gboolean xplayer_object_action_remote_get_setting	(XplayerObject *xplayer,
						 XplayerRemoteSetting setting);

const gchar * const *xplayer_object_get_supported_content_types (void);
const gchar * const *xplayer_object_get_supported_uri_schemes (void);

#endif /* __XPLAYER_H__ */
