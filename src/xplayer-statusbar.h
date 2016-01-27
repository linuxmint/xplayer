/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * XplayerStatusbar Copyright (C) 1998 Shawn T. Amundson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __XPLAYER_STATUSBAR_H__
#define __XPLAYER_STATUSBAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XPLAYER_TYPE_STATUSBAR            (xplayer_statusbar_get_type ())
#define XPLAYER_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XPLAYER_TYPE_STATUSBAR, XplayerStatusbar))
#define XPLAYER_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XPLAYER_TYPE_STATUSBAR, XplayerStatusbarClass))
#define XPLAYER_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XPLAYER_TYPE_STATUSBAR))
#define XPLAYER_IS_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XPLAYER_TYPE_STATUSBAR))
#define XPLAYER_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XPLAYER_TYPE_STATUSBAR, XplayerStatusbarClass))

typedef struct _XplayerStatusbarPrivate XplayerStatusbarPrivate;

typedef struct
{
  GtkStatusbar parent_instance;
  XplayerStatusbarPrivate *priv;
} XplayerStatusbar;

typedef GtkStatusbarClass XplayerStatusbarClass;

G_MODULE_EXPORT GType xplayer_statusbar_get_type  (void) G_GNUC_CONST;
GtkWidget* xplayer_statusbar_new          	(void);

void       xplayer_statusbar_set_time		(XplayerStatusbar *statusbar,
						 gint time);
void       xplayer_statusbar_set_time_and_length	(XplayerStatusbar *statusbar,
						 gint time, gint length);
void       xplayer_statusbar_set_seeking          (XplayerStatusbar *statusbar,
						 gboolean seeking);

void       xplayer_statusbar_set_text             (XplayerStatusbar *statusbar,
						 const char *label);
void       xplayer_statusbar_push_help            (XplayerStatusbar *statusbar,
						 const char *message);
void       xplayer_statusbar_pop_help             (XplayerStatusbar *statusbar);
void	   xplayer_statusbar_push			(XplayerStatusbar *statusbar,
						 gdouble percentage);
void       xplayer_statusbar_pop			(XplayerStatusbar *statusbar);

G_END_DECLS

#endif /* __XPLAYER_STATUSBAR_H__ */
