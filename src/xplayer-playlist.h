/* xplayer-playlist.h: Simple playlist dialog

   Copyright (C) 2002, 2003, 2004, 2005 Bastien Nocera <hadess@hadess.net>

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

#ifndef XPLAYER_PLAYLIST_H
#define XPLAYER_PLAYLIST_H

#include <gtk/gtk.h>
#include <xplayer-pl-parser.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define XPLAYER_TYPE_PLAYLIST            (xplayer_playlist_get_type ())
#define XPLAYER_PLAYLIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XPLAYER_TYPE_PLAYLIST, XplayerPlaylist))
#define XPLAYER_PLAYLIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XPLAYER_TYPE_PLAYLIST, XplayerPlaylistClass))
#define XPLAYER_IS_PLAYLIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XPLAYER_TYPE_PLAYLIST))
#define XPLAYER_IS_PLAYLIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XPLAYER_TYPE_PLAYLIST))

typedef enum {
	XPLAYER_PLAYLIST_STATUS_NONE,
	XPLAYER_PLAYLIST_STATUS_PLAYING,
	XPLAYER_PLAYLIST_STATUS_PAUSED
} XplayerPlaylistStatus;

typedef enum {
	XPLAYER_PLAYLIST_DIRECTION_NEXT,
	XPLAYER_PLAYLIST_DIRECTION_PREVIOUS
} XplayerPlaylistDirection;

typedef enum {
	XPLAYER_PLAYLIST_DIALOG_SELECTED,
	XPLAYER_PLAYLIST_DIALOG_PLAYING
} XplayerPlaylistSelectDialog;


typedef struct XplayerPlaylist	       XplayerPlaylist;
typedef struct XplayerPlaylistClass      XplayerPlaylistClass;
typedef struct XplayerPlaylistPrivate    XplayerPlaylistPrivate;

typedef void (*XplayerPlaylistForeachFunc) (XplayerPlaylist *playlist,
					  const gchar   *filename,
					  const gchar   *uri,
					  gpointer       user_data);

struct XplayerPlaylist {
	GtkBox parent;
	XplayerPlaylistPrivate *priv;
};

struct XplayerPlaylistClass {
	GtkBoxClass parent_class;

	void (*changed) (XplayerPlaylist *playlist);
	void (*item_activated) (XplayerPlaylist *playlist);
	void (*active_name_changed) (XplayerPlaylist *playlist);
	void (*current_removed) (XplayerPlaylist *playlist);
	void (*repeat_toggled) (XplayerPlaylist *playlist, gboolean repeat);
	void (*shuffle_toggled) (XplayerPlaylist *playlist, gboolean toggled);
	void (*subtitle_changed) (XplayerPlaylist *playlist);
	void (*item_added) (XplayerPlaylist *playlist, const gchar *filename, const gchar *uri);
	void (*item_removed) (XplayerPlaylist *playlist, const gchar *filename, const gchar *uri);
};

GType    xplayer_playlist_get_type (void);
GtkWidget *xplayer_playlist_new      (void);

/* The application is responsible for checking that the mrl is correct
 * @display_name is if you have a preferred display string for the mrl,
 * NULL otherwise
 */
void xplayer_playlist_add_mrl (XplayerPlaylist *playlist,
                             const char *mrl,
                             const char *display_name,
                             gboolean cursor,
                             GCancellable *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer user_data);
gboolean xplayer_playlist_add_mrl_finish (XplayerPlaylist *playlist,
                                        GAsyncResult *result);
gboolean xplayer_playlist_add_mrl_sync (XplayerPlaylist *playlist,
                                      const char *mrl,
                                      const char *display_name);

typedef struct XplayerPlaylistMrlData XplayerPlaylistMrlData;

XplayerPlaylistMrlData *xplayer_playlist_mrl_data_new (const gchar *mrl,
                                                   const gchar *display_name);
void xplayer_playlist_mrl_data_free (XplayerPlaylistMrlData *data);

void xplayer_playlist_add_mrls (XplayerPlaylist *self,
                              GList *mrls,
                              gboolean cursor,
                              GCancellable *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer user_data);
gboolean xplayer_playlist_add_mrls_finish (XplayerPlaylist *self,
                                         GAsyncResult *result,
                                         GError **error);

void xplayer_playlist_save_current_playlist (XplayerPlaylist *playlist,
					   const char *output);
void xplayer_playlist_save_current_playlist_ext (XplayerPlaylist *playlist,
					   const char *output, XplayerPlParserType type);
void xplayer_playlist_select_subtitle_dialog (XplayerPlaylist *playlist,
					    XplayerPlaylistSelectDialog mode);

/* xplayer_playlist_clear doesn't emit the current_removed signal, even if it does
 * because the caller should know what to do after it's done with clearing */
gboolean   xplayer_playlist_clear (XplayerPlaylist *playlist);
void       xplayer_playlist_clear_with_g_mount (XplayerPlaylist *playlist,
					      GMount *mount);
char      *xplayer_playlist_get_current_mrl (XplayerPlaylist *playlist,
					   char **subtitle);
char      *xplayer_playlist_get_current_title (XplayerPlaylist *playlist);
char      *xplayer_playlist_get_current_content_type (XplayerPlaylist *playlist);
char      *xplayer_playlist_get_title (XplayerPlaylist *playlist,
				     guint title_index);

gboolean   xplayer_playlist_set_title (XplayerPlaylist *playlist,
				     const char *title);
void       xplayer_playlist_set_current_subtitle (XplayerPlaylist *playlist,
						const char *subtitle_uri);

#define    xplayer_playlist_has_direction(playlist, direction) (direction == XPLAYER_PLAYLIST_DIRECTION_NEXT ? xplayer_playlist_has_next_mrl (playlist) : xplayer_playlist_has_previous_mrl (playlist))
gboolean   xplayer_playlist_has_previous_mrl (XplayerPlaylist *playlist);
gboolean   xplayer_playlist_has_next_mrl (XplayerPlaylist *playlist);

#define    xplayer_playlist_set_direction(playlist, direction) (direction == XPLAYER_PLAYLIST_DIRECTION_NEXT ? xplayer_playlist_set_next (playlist) : xplayer_playlist_set_previous (playlist))
void       xplayer_playlist_set_previous (XplayerPlaylist *playlist);
void       xplayer_playlist_set_next (XplayerPlaylist *playlist);

gboolean   xplayer_playlist_get_repeat (XplayerPlaylist *playlist);
void       xplayer_playlist_set_repeat (XplayerPlaylist *playlist, gboolean repeat);

gboolean   xplayer_playlist_get_shuffle (XplayerPlaylist *playlist);
void       xplayer_playlist_set_shuffle (XplayerPlaylist *playlist,
				       gboolean shuffle);

gboolean   xplayer_playlist_set_playing (XplayerPlaylist *playlist, XplayerPlaylistStatus state);
XplayerPlaylistStatus xplayer_playlist_get_playing (XplayerPlaylist *playlist);

void       xplayer_playlist_set_at_start (XplayerPlaylist *playlist);
void       xplayer_playlist_set_at_end (XplayerPlaylist *playlist);

int        xplayer_playlist_get_current (XplayerPlaylist *playlist);
int        xplayer_playlist_get_last (XplayerPlaylist *playlist);
void       xplayer_playlist_set_current (XplayerPlaylist *playlist, guint current_index);
GtkWidget *xplayer_playlist_get_toolbar (XplayerPlaylist *playlist);

G_END_DECLS

#endif /* XPLAYER_PLAYLIST_H */
