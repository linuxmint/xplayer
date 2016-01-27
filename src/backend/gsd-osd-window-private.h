/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 8; tab-width: 8 -*-
 *
 * On-screen-display (OSD) window for gnome-settings-daemon's plugins
 *
 * Copyright (C) 2006 William Jon McCann <mccann@jhu.edu> 
 * Copyright (C) 2009 Novell, Inc
 *
 * Authors:
 *   William Jon McCann <mccann@jhu.edu>
 *   Federico Mena-Quintero <federico@novell.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

/* GsdOsdWindow is an "on-screen-display" window (OSD).  It is the cute,
 * semi-transparent, curved popup that appears when you press a hotkey global to
 * the desktop, such as to change the volume, switch your monitor's parameters,
 * etc.
 *
 * You can create a GsdOsdWindow and use it as a normal GtkWindow.  It will
 * automatically center itself, figure out if it needs to be composited, etc.
 * Just pack your widgets in it, sit back, and enjoy the ride.
 */

#ifndef GSD_OSD_WINDOW_PRIVATE_H
#define GSD_OSD_WINDOW_PRIVATE_H

#include <glib-object.h>
#include <cairo.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DIALOG_FADE_TIMEOUT 1500  /* timeout before fade starts */
#define FADE_FRAME_TIMEOUT 10     /* timeout in ms between each frame of the fade */

typedef struct {
        int                 size;
        GtkStyleContext    *style;
        GtkTextDirection    direction;

        GsdOsdWindowAction  action;
        GtkIconTheme       *theme;
        const char         *icon_name;

        gboolean            show_level;
        int                 volume_level;
        guint               volume_muted : 1;
} GsdOsdDrawContext;

void gsd_osd_window_draw (GsdOsdDrawContext *ctx, cairo_t *cr);

G_END_DECLS

#endif
