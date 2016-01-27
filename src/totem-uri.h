/* xplayer-uri.h

   Copyright (C) 2004 Bastien Nocera <hadess@hadess.net>

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

#ifndef XPLAYER_URI_H
#define XPLAYER_URI_H

#include "xplayer.h"
#include <gtk/gtk.h>
#include <gio/gio.h>

G_BEGIN_DECLS

const char *	xplayer_dot_dir			(void);
const char *	xplayer_data_dot_dir		(void);
char *		xplayer_pictures_dir		(void);
char *		xplayer_create_full_path		(const char *path);
GMount *	xplayer_get_mount_for_media	(const char *uri);
gboolean	xplayer_playing_dvd		(const char *uri);
gboolean	xplayer_uri_is_subtitle		(const char *uri);
gboolean	xplayer_is_special_mrl		(const char *uri);
gboolean	xplayer_is_block_device		(const char *uri);
void		xplayer_setup_file_monitoring	(Xplayer *xplayer);
void		xplayer_setup_file_filters	(void);
void		xplayer_destroy_file_filters	(void);
char *		xplayer_uri_escape_for_display	(const char *uri);
GSList *	xplayer_add_files			(GtkWindow *parent,
						 const char *path);
char *		xplayer_add_subtitle		(GtkWindow *parent, 
						 const char *uri);

void xplayer_save_position (Xplayer *xplayer);
void xplayer_try_restore_position (Xplayer *xplayer, const char *mrl);

G_END_DECLS

#endif /* XPLAYER_URI_H */
