/*
 * Copyright © 2013 Bastien Nocera <hadess@hadess.net>
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
 * permission is above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

#include <gtk/gtk.h>

#include "totem-rtl-helpers.h"

struct {
	const char *orig;
	const char *ltr;
	const char *rtl;
} icons[] = {
	{ "go-previous", "go-previous-symbolic", "go-previous-rtl-symbolic" },
	{ "media-playback-start", "media-playback-start-symbolic", "media-playback-start-rtl-symbolic" },
	{ "media-seek-forward", "media-seek-forward-symbolic", "media-seek-forward-rtl-symbolic" },
	{ "media-seek-backward", "media-seek-backward-symbolic", "media-seek-backward-rtl-symbolic" },
	{ "media-skip-forward", "media-skip-forward-symbolic", "media-skip-forward-rtl-symbolic" },
	{ "media-skip-backward", "media-skip-backward-symbolic", "media-skip-backward-rtl-symbolic" },
};

const char *
totem_get_rtl_icon_name (const char *name)
{
	guint i;
	gboolean rtl;

	g_return_val_if_fail (name != NULL, NULL);

	for (i = 0; i < G_N_ELEMENTS(icons); i++) {
		if (g_str_equal (name, icons[i].orig)) {
			rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);
			return rtl ? icons[i].rtl : icons[i].ltr;
		}
	}

	return NULL;
}
