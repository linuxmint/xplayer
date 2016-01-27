/* xplayer-menu.h

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

#ifndef XPLAYER_MENU_H
#define XPLAYER_MENU_H

#include "xplayer.h"

G_BEGIN_DECLS

void xplayer_ui_manager_setup (Xplayer *xplayer);

void xplayer_sublang_update (Xplayer *xplayer);
void xplayer_sublang_exit (Xplayer *xplayer);

void xplayer_setup_recent (Xplayer *xplayer);

G_END_DECLS

#endif /* XPLAYER_MENU_H */
