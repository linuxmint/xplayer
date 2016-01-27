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

#ifndef XPLAYER_CMML_PARSER_H_
#define XPLAYER_CMML_PARSER_H_

#include <glib.h>
#include <libxml/xmlreader.h>

/**
 * XplayerCmmlClip:
 * @title: clip title
 * @desc: clip description
 * @time_start: start time of clip in msecs
 * @pixbuf: clip thumbnail
 *
 *  Structure to handle clip data.
 **/
typedef struct {
	gchar		*title;
	gchar		*desc;
	gint64		time_start;
	GdkPixbuf	*pixbuf;
} XplayerCmmlClip;

/**
 * XplayerCmmlAsyncData:
 * @file: file to read
 * @list: list to store chapters to read in/write
 * @final: function to call at final, %NULL is allowed
 * @user_data: user data passed to @final callback
 * @buf: buffer for writing
 * @error: last error message string, or %NULL if not any
 * @successful: whether operation was successful or not
 * @is_exists: whether @file exists or not
 * @from_dialog: whether read operation was started from open dialog
 * @cancellable: object to cancel operation, %NULL is allowed
 *
 * Structure to handle data for async reading/writing clip data.
 **/
typedef struct {
	gchar		*file;
	GList		*list;
	GFunc		final;
	gpointer	user_data;
	gchar		*buf;
	gchar		*error;
	gboolean	successful;
	gboolean	is_exists;
	gboolean	from_dialog;
	GCancellable	*cancellable;
} XplayerCmmlAsyncData;

gchar *	xplayer_cmml_convert_msecs_to_str (gint64 time_msecs);
XplayerCmmlClip * xplayer_cmml_clip_new (const gchar *title, const gchar *desc, gint64 start, GdkPixbuf *pixbuf);
void xplayer_cmml_clip_free (XplayerCmmlClip *clip);
XplayerCmmlClip * xplayer_cmml_clip_copy (XplayerCmmlClip *clip);

void xplayer_cmml_read_file (GFile               *file,
			   GCancellable        *cancellable,
			   GAsyncReadyCallback  callback,
			   gpointer             user_data);
GList *xplayer_cmml_read_file_finish (GFile        *file,
				    GAsyncResult *res,
				    GError      **error);
gint xplayer_cmml_write_file_async (XplayerCmmlAsyncData *data);

#endif /* XPLAYER_CMML_PARSER_H_ */
