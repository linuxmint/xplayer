/* xplayer-sidebar.h

   Copyright (C) 2004-2005 Bastien Nocera <hadess@hadess.net>

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

#ifndef XPLAYER_SIDEBAR_H
#define XPLAYER_SIDEBAR_H

G_BEGIN_DECLS

void xplayer_sidebar_setup (Xplayer *xplayer,
			  gboolean visible);

void xplayer_sidebar_toggle (Xplayer *xplayer, gboolean state);
void xplayer_sidebar_set_visibility (Xplayer *xplayer, gboolean visible);
gboolean xplayer_sidebar_is_visible (Xplayer *xplayer);

gboolean xplayer_sidebar_is_focused (Xplayer *xplayer, gboolean *handles_kbd);

char *xplayer_sidebar_get_current_page (Xplayer *xplayer);
void xplayer_sidebar_set_current_page (Xplayer *xplayer,
				     const char *page_id,
				     gboolean force_visible);

void xplayer_sidebar_add_page (Xplayer *xplayer,
			     const char *page_id,
			     const char *label,
			     const char *accelerator,
			     GtkWidget *main_widget);
void xplayer_sidebar_remove_page (Xplayer *xplayer,
				const char *page_id);

G_END_DECLS

#endif /* XPLAYER_SIDEBAR_H */
