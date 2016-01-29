/*
 * Copyright (C) 2003  Andrew Sobala <aes@gnome.org>
 * Copyright (C) 2005  Bastien Nocera <hadess@hadess.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#ifndef XPLAYER_PROPERTIES_VIEW_H
#define XPLAYER_PROPERTIES_VIEW_H

#include <gtk/gtk.h>

#define XPLAYER_TYPE_PROPERTIES_VIEW	    (xplayer_properties_view_get_type ())
#define XPLAYER_PROPERTIES_VIEW(obj)	    (G_TYPE_CHECK_INSTANCE_CAST ((obj), XPLAYER_TYPE_PROPERTIES_VIEW, XplayerPropertiesView))
#define XPLAYER_PROPERTIES_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), XPLAYER_TYPE_PROPERTIES_VIEW, XplayerPropertiesViewClass))
#define XPLAYER_IS_PROPERTIES_VIEW(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XPLAYER_TYPE_PROPERTIES_VIEW))
#define XPLAYER_IS_PROPERTIES_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XPLAYER_TYPE_PROPERTIES_VIEW))

typedef struct XplayerPropertiesViewPriv XplayerPropertiesViewPriv;

typedef struct {
	GtkGrid parent;
	XplayerPropertiesViewPriv *priv;
} XplayerPropertiesView;

typedef struct {
	GtkGridClass parent;
} XplayerPropertiesViewClass;

GType      xplayer_properties_view_get_type      (void);
void       xplayer_properties_view_register_type (GTypeModule *module);

GtkWidget *xplayer_properties_view_new           (const char *location,
						GtkWidget  *label);
void       xplayer_properties_view_set_location  (XplayerPropertiesView *view,
						 const char         *location);

#endif /* XPLAYER_PROPERTIES_VIEW_H */
