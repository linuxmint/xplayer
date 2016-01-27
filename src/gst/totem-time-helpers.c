/*
 * Copyright © 2002-2012 Bastien Nocera <hadess@hadess.net>
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

#include <glib/gi18n.h>
#include <libintl.h>

#include "totem-time-helpers.h"


/* FIXME: Remove
 * See https://bugzilla.gnome.org/show_bug.cgi?id=679850 */
char *
totem_time_to_string (gint64 msecs)
{
	int sec, min, hour, _time;

	_time = (int) (msecs / 1000);
	sec = _time % 60;
	_time = _time - sec;
	min = (_time % (60*60)) / 60;
	_time = _time - (min * 60);
	hour = _time / (60*60);

	if (hour > 0)
	{
		/* hour:minutes:seconds */
		/* Translators: This is a time format, like "9:05:02" for 9
		 * hours, 5 minutes, and 2 seconds. You may change ":" to
		 * the separator that your locale uses or use "%Id" instead
		 * of "%d" if your locale uses localized digits.
		 */
		return g_strdup_printf (C_("long time format", "%d:%02d:%02d"), hour, min, sec);
	}

	/* minutes:seconds */
	/* Translators: This is a time format, like "5:02" for 5
	 * minutes and 2 seconds. You may change ":" to the
	 * separator that your locale uses or use "%Id" instead of
	 * "%d" if your locale uses localized digits.
	 */
	return g_strdup_printf (C_("short time format", "%d:%02d"), min, sec);
}
