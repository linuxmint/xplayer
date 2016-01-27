/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Sunil Mohan Adapa <sunilmohan@gnu.org.in>
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
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "backend/bacon-video-widget.h"

#define XPLAYER_TYPE_FULLSCREEN            (xplayer_fullscreen_get_type ())
#define XPLAYER_FULLSCREEN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                          XPLAYER_TYPE_FULLSCREEN, \
                                          XplayerFullscreen))
#define XPLAYER_FULLSCREEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                          XPLAYER_TYPE_FULLSCREEN, \
                                          XplayerFullscreenClass))
#define XPLAYER_IS_FULLSCREEN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                          XPLAYER_TYPE_FULLSCREEN))
#define XPLAYER_IS_FULLSCREEN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                          XPLAYER_TYPE_FULLSCREEN))

typedef struct XplayerFullscreen XplayerFullscreen;
typedef struct XplayerFullscreenClass XplayerFullscreenClass;
typedef struct _XplayerFullscreenPrivate XplayerFullscreenPrivate;

struct XplayerFullscreen {
	GObject                parent;

	/* Public Widgets from popups */
	GtkWidget              *time_label;
	GtkWidget              *seek;
	GtkWidget              *volume;
	GtkWidget              *buttons_box;
	GtkWidget              *exit_button;

	/* Private */
	XplayerFullscreenPrivate *priv;
};

struct XplayerFullscreenClass {
	GObjectClass parent_class;
};

GType    xplayer_fullscreen_get_type           (void);
XplayerFullscreen * xplayer_fullscreen_new       (GtkWindow *toplevel_window);
void     xplayer_fullscreen_set_video_widget   (XplayerFullscreen *fs,
					      BaconVideoWidget *bvw);
void     xplayer_fullscreen_set_parent_window  (XplayerFullscreen *fs,
					      GtkWindow *parent_window);
void     xplayer_fullscreen_show_popups        (XplayerFullscreen *fs,
					      gboolean show_cursor);
void xplayer_fullscreen_show_popups_or_osd (XplayerFullscreen *fs,
					  const char *icon_name,
					  gboolean show_cursor);
gboolean xplayer_fullscreen_is_fullscreen      (XplayerFullscreen *fs);
void     xplayer_fullscreen_set_fullscreen     (XplayerFullscreen *fs,
					      gboolean fullscreen);
void     xplayer_fullscreen_set_title          (XplayerFullscreen *fs,
					      const char *title);
void     xplayer_fullscreen_set_seekable       (XplayerFullscreen *fs,
					      gboolean seekable);
void     xplayer_fullscreen_set_can_set_volume (XplayerFullscreen *fs,
					      gboolean can_set_volume);
