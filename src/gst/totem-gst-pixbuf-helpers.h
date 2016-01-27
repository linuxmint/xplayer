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

#ifndef HAVE_TOTEM_GST_PIXBUF_HELPERS_H
#define HAVE_TOTEM_GST_PIXBUF_HELPERS_H

#define GST_USE_UNSTABLE_API 1

#include <gst/gst.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

GdkPixbuf * totem_gst_playbin_get_frame (GstElement *play);

GdkPixbuf * totem_gst_tag_list_get_cover (GstTagList *tag_list);

G_END_DECLS

#endif				/* HAVE_TOTEM_GST_PIXBUF_HELPERS_H */
