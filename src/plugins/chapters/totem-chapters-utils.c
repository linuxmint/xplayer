/*
 * Copyright (C) 2010 Alexander Saprykin <xelfium@gmail.com>
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
 *
 * The Xplayer project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Xplayer. This
 * permission are above and beyond the permissions granted by the GPL license
 * Xplayer is covered by.
 */

/*
 * SECTION:xplayer-chapters-utils
 * @short_description: misc helper functions
 * @stability: Unstable
 * @include: xplayer-chapters-utils.h
 *
 * These functions are used for misc operations within chapters plugin.
 */

#include <glib.h>

#include "xplayer-chapters-utils.h"
#include <string.h>

/**
 * xplayer_remove_file_extension:
 * @filename: filename to remove extension from
 *
 * Removes extension from the @filename.
 *
 * Returns: filename without extension on success, %NULL otherwise; free with g_free ().
 **/
gchar *
xplayer_remove_file_extension (const gchar *filename)
{
	gchar	*p, *s;

	g_return_val_if_fail (filename != NULL, NULL);
	g_return_val_if_fail (strlen (filename) > 0, NULL);

	p = g_strrstr (filename, ".");
	if (G_UNLIKELY (p == NULL))
		return NULL;

	s = g_strrstr (p, G_DIR_SEPARATOR_S);
	if (G_UNLIKELY (s != NULL))
		return NULL;

	return g_strndup (filename, ABS(p - filename));
}

/**
 * xplayer_change_file_extension:
 * @filename: filename to change extension in
 * @ext: new extension for @filename
 *
 * Changes extension in @filename to @ext.
 *
 * Returns: filename with new extension on success, %NULL otherwise; free with g_free ().
 **/
gchar *
xplayer_change_file_extension (const gchar	*filename,
			     const gchar	*ext)
{
	gchar	*no_ext, *new_file;

	g_return_val_if_fail (filename != NULL, NULL);
	g_return_val_if_fail (strlen (filename) > 0, NULL);
	g_return_val_if_fail (ext != NULL, NULL);
	g_return_val_if_fail (strlen (ext) > 0, NULL);

	no_ext = xplayer_remove_file_extension (filename);

	if (no_ext == NULL)
		return NULL;

	new_file = g_strconcat (no_ext, ".", ext, NULL);
	g_free (no_ext);

	return new_file;
}
