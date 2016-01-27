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
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 */

/*
 * SECTION:totem-cmml-parser
 * @short_description: parser for CMML files
 * @stability: Unstable
 *
 * These functions are used to parse CMML files for chapters.
 */

#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <glib/gi18n-lib.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <string.h>
#include <math.h>
#include "totem-cmml-parser.h"

#define MSECS_IN_HOUR (1000 * 60 * 60)
#define MSECS_IN_MINUTE (1000 * 60)
#define MSECS_IN_SECOND (1000)

#define TOTEM_CMML_PREAMBLE 	"<!DOCTYPE cmml SYSTEM \"cmml.dtd\">\n"

typedef void (*TotemCmmlCallback) (TotemCmmlClip *, gpointer user_data);

typedef enum {
	TOTEM_CMML_NONE = 0,
	TOTEM_CMML_CMML,
	TOTEM_CMML_HEAD,
	TOTEM_CMML_CLIP
} TotemCmmlStatus;

typedef struct {
	xmlTextReaderPtr	reader;
	TotemCmmlStatus		status;
	TotemCmmlClip		*clip;
	TotemCmmlCallback	callback;
	gpointer		user_data;
} TotemCmmlContext;

static TotemCmmlContext * totem_cmml_context_new (void);
static void totem_cmml_context_free (TotemCmmlContext *context);
static void totem_cmml_context_set_callback (TotemCmmlContext *context, TotemCmmlCallback cb, gpointer user_data);
static int totem_cmml_compare_clips (gconstpointer pointer_a, gconstpointer pointer_b);
static TotemCmmlClip * totem_cmml_clip_new_from_attrs (const xmlChar **attrs);
static void totem_cmml_clip_insert_img_attr (TotemCmmlClip *clip, const xmlChar **attrs);
static gdouble totem_cmml_parse_smpte (const gchar *str, gdouble framerate);
static gdouble totem_cmml_parse_npt (const gchar *str);
static gdouble totem_cmml_parse_time_str (const gchar *str);
static void totem_cmml_parse_start (TotemCmmlContext *context, const xmlChar *tag, const xmlChar **attrs);
static void totem_cmml_parse_end (TotemCmmlContext *context, const xmlChar *tag);
static void totem_cmml_parse_xml_node (TotemCmmlContext	*context);
static void totem_cmml_read_clip_cb (TotemCmmlClip *clip, gpointer user_data);

static TotemCmmlContext *
totem_cmml_context_new (void)
{
	TotemCmmlContext *ctx = g_new0 (TotemCmmlContext, 1);

	ctx->status = TOTEM_CMML_NONE;
	return ctx;
}

static void
totem_cmml_context_free (TotemCmmlContext *context)
{
	g_free (context);
}

static void
totem_cmml_context_set_callback (TotemCmmlContext	*context,
				 TotemCmmlCallback	cb,
				 gpointer		user_data)
{
	g_return_if_fail (context != NULL);

	context->callback = cb;
	context->user_data = user_data;
}

static TotemCmmlClip *
totem_cmml_clip_new_from_attrs (const xmlChar **attrs)
{
	gint		i = 0;
	gint64		start = -1;
	gchar		*title = NULL;

	g_return_val_if_fail (attrs != NULL, NULL);

	while (attrs[i] != NULL) {
		if (g_strcmp0 ((const gchar *) attrs[i], "title") == 0)
			title = g_strdup ((const gchar *) attrs[i + 1]);
		else if (g_strcmp0 ((const gchar *) attrs[i], "start") == 0)
			start = (gint64) (totem_cmml_parse_time_str ((const gchar *) attrs[i + 1]) * 1000);
		i += 2;
	}

	return totem_cmml_clip_new (title, NULL, start, NULL);
}

static void
totem_cmml_clip_insert_img_attr (TotemCmmlClip	*clip,
				 const xmlChar	**attrs)
{
	gint		i = 0;
	GdkPixdata	*pixdata;
	GdkPixbuf	*pixbuf;
	guchar		*base64_dec;
	gsize		st_len;
	GError		*error = NULL;

	g_return_if_fail (clip != NULL);
	g_return_if_fail (attrs != NULL);

	while (attrs[i] != NULL) {
		if (G_LIKELY (g_strcmp0 ((const gchar *) attrs[i], "src") == 0 &&
		    xmlStrlen (attrs[i + 1]) > 0)) {
			pixdata = g_new0 (GdkPixdata, 1);
			/* decode pixbuf data */
			base64_dec = g_base64_decode ((const gchar *) attrs[i + 1], &st_len);
			/* deserialize pixbuf data */
			if (G_UNLIKELY (!gdk_pixdata_deserialize (pixdata, st_len, base64_dec, NULL))) {
				g_warning ("chapters: failed to deserialize clip's pixbuf data");
				pixbuf = NULL;
			} else {
				pixbuf = gdk_pixbuf_from_pixdata (pixdata, TRUE, &error);
				if (error != NULL) {
					g_warning ("chapters: failed to create pixbuf from pixdata: %s", error->message);
					pixbuf = NULL;
					g_free (error);
				}
			}
			g_free (pixdata);
			g_free (base64_dec);

			if (G_LIKELY (pixbuf != NULL)) {
				clip->pixbuf = g_object_ref (pixbuf);
				g_object_unref (pixbuf);
			} else
				clip->pixbuf = pixbuf;
			break;
		}
		i += 2;
	}
}

/* the idea of parsing time was taken from libcmml (and some of code, too) */
static gdouble
totem_cmml_parse_smpte (const gchar	*str,
			gdouble		framerate)
{
	gint		h = 0, m = 0, s = 0;
	gfloat		frames;
	const gchar	*endptr, *old_endptr;

	if (G_UNLIKELY (str == NULL))
		return -1.0;

	/* We expect something in the format %d:%d:%d:%f (hours:minutes:seconds:frames), but we can handle %d:%d:%f (minutes:seconds:frames) as well.
	 * We can't use sscanf() as it's locale-dependent.
	 *
	 * Test cases (with framerate = 24):
	 *   "1:2:3:12.5" -> 3723.520833333
	 *   "1:2:3:12" -> 3723.5
	 *   "1:1:3:12.5" -> 3663.520833333
	 *   "1:2:3:-12.5" -> -1.0
	 *   "1:2:3:12.0" -> 3723.5
	 *   "0:2:3:12.5" -> 123.520833333
	 *   "2:3:12.5" -> 123.520833333
	 *   "2:3.6:12.5" -> -1.0
	 *   "::" -> -1.0
	 *   ":::" -> -1.0
	 *   "asd" -> -1.0
	 *   "" -> -1.0
	 */
	old_endptr = str;
	h = g_ascii_strtoll (str, (gchar**) &endptr, 10);
	if (*endptr != ':' || endptr == old_endptr)
		return -1.0;
	else
		endptr++;

	old_endptr = endptr;
	m = g_ascii_strtoll (endptr, (gchar**) &endptr, 10);
	if (*endptr != ':' || endptr == old_endptr)
		return -1.0;
	else
		endptr++;

	old_endptr = endptr;
	s = g_ascii_strtoll (endptr, (gchar**) &endptr, 10);
	if (*endptr == '\0' || endptr == old_endptr) {
		return -1.0;
	} else if (*endptr != ':') {
		/* We've probably got the %d:%d:%f format; back up and try parsing a float */
		endptr = old_endptr;

		/* Shift all the previously-parsed parameters along */
		s = m;
		m = h;
		h = 0;
	} else {
		endptr++;
	}

	old_endptr = endptr;
	frames = g_ascii_strtod (endptr, (gchar**) &endptr);
	if (*endptr != '\0' || endptr == old_endptr)
		return -1.0;

	/* check time and framerate bounds */
	if (G_UNLIKELY (h < 0))
		return -1.0;
	if (G_UNLIKELY (m > 59 || m < 0))
		return -1.0;
	if (G_UNLIKELY (s > 59 || s < 0))
		return -1.0;

	if (G_UNLIKELY (frames > (gfloat) ceil (framerate) || frames < 0))
		return -1.0;

	return ((h * 3600.0) + (m * 60.0) + s) + (frames / framerate);
}

static gdouble
totem_cmml_parse_npt (const gchar *str)
{
	gint		h, m;
	gfloat		s;
	const gchar	*endptr, *old_endptr;

	if (G_UNLIKELY (str == NULL))
		return -1.0;

	/* We expect something in the format %d:%d:%f (hours:minutes:seconds), but we can handle %d:%f (minutes:seconds) or %f (seconds) as well.
	 * We can't use sscanf() as it's locale-dependent.
	 *
	 * Test cases:
	 *   "1:2:0.5" -> 3720.5
	 *   "1:2" -> 62.0
	 *   "0:2:0.5" -> 120.5
	 *   "2:0.5" -> 120.5
	 *   "0:0:0.5" -> 0.5
	 *   "0.5" -> 0.5
	 *   "6" -> 6.0
	 *   "::" -> -1.0
	 *   ":" -> -1.0
	 *   "asd" -> -1.0
	 *   "" -> -1.0
	 */
	old_endptr = str;
	h = g_ascii_strtoll (str, (gchar**) &endptr, 10);
	if (endptr == old_endptr) {
		return -1.0;
	} else if (*endptr != ':') {
		/* We've probably got the %f format */
		endptr = old_endptr;

		/* Zero the hours */
		h = 0;
	} else {
		endptr++;
	}

	old_endptr = endptr;
	m = g_ascii_strtoll (endptr, (gchar**) &endptr, 10);
	if (endptr == old_endptr) {
		return -1.0;
	} else if (*endptr != ':') {
		/* We've probably got the %d:%f format */
		endptr = old_endptr;

		/* Shift all the previously-parsed parameters along */
		m = h;
		h = 0;
	} else {
		endptr++;
	}

	old_endptr = endptr;
	s = g_ascii_strtod (endptr, (gchar**) &endptr);
	if (*endptr != '\0' || endptr == old_endptr)
		return -1.0;

	if (G_UNLIKELY (h < 0))
		return -1;
	if (G_UNLIKELY (m > 59 || m < 0))
		return -1;

	/* We break slightly with the specifications here and allow seconds-only values greater than 60 seconds.
	 * (i.e. we allow "90" to be successfully parsed as 1.5 minutes and returned as 90 seconds, rather than
	 * returning an error because it's > 60). This is because Totem previously (incorrectly) wrote out timestamps
	 * in this (seconds only, potentially > 60) format; so for compatibility with CMML files written by older
	 * versions of Totem, we have to allow such formats.
	 * However, if either h or m is non-zero, we error out as before. */
	if (G_UNLIKELY ((h != 0 || m != 0) && (s >= 60.0 || s < 0.0)))
		return -1;

	return (h * 3600.0) + (m * 60.0) + s;
}


static gdouble
totem_cmml_parse_time_str (const gchar *str)
{
	if (G_UNLIKELY (str == NULL))
		return -1.0;

	/* we need to choose parsing function to use */
	if (g_str_has_prefix (str, "npt:"))
		return totem_cmml_parse_npt (str + 4);

	if (g_str_has_prefix (str, "smpte-24:"))
		return totem_cmml_parse_smpte (str + 9, 24.0);

	if (g_str_has_prefix (str, "smpte-24-drop:"))
		return totem_cmml_parse_smpte (str + 14, 23.976);

	if (g_str_has_prefix (str, "smpte-25:"))
		return totem_cmml_parse_smpte (str + 9, 25.0);

	if (g_str_has_prefix (str, "smpte-30:"))
		return totem_cmml_parse_smpte (str + 9, 30.0);

	if (g_str_has_prefix (str, "smpte-30-drop:"))
		return totem_cmml_parse_smpte (str + 14, 29.97);

	if (g_str_has_prefix (str, "smpte-50:"))
		return totem_cmml_parse_smpte (str + 9, 50.0);

	if (g_str_has_prefix (str, "smpte-60:"))
		return totem_cmml_parse_smpte (str + 9, 60);

	if (g_str_has_prefix (str, "smpte-60-drop:"))
		return totem_cmml_parse_smpte (str + 14, 59.94);

	/* default is npt */
	return totem_cmml_parse_npt (str);
}

static void
totem_cmml_parse_start (TotemCmmlContext	*context,
			const xmlChar		*tag,
			const xmlChar		**attrs)
{
	g_return_if_fail (context != NULL);
	g_return_if_fail (tag != NULL);

	if (g_strcmp0 ((const gchar *) tag, "cmml") == 0) {
		if (G_UNLIKELY (context->status != TOTEM_CMML_NONE))
			return;

		/* empty document */
		if (G_UNLIKELY (xmlTextReaderIsEmptyElement (context->reader)))
			return;

		context->status = TOTEM_CMML_CMML;
	} else if (g_strcmp0 ((const gchar *) tag, "head") == 0) {
		if (G_UNLIKELY (context->status != TOTEM_CMML_CMML))
			return;

		/* empty head, let it ok */
		if (G_UNLIKELY (xmlTextReaderIsEmptyElement (context->reader)))
			context->status = TOTEM_CMML_CMML;

		context->status = TOTEM_CMML_HEAD;
	} else if (g_strcmp0 ((const gchar *) tag, "clip") == 0) {
		if (G_UNLIKELY (context->status != TOTEM_CMML_CMML))
			return;

		context->clip = totem_cmml_clip_new_from_attrs (attrs);

		/* empty clip element, we need to set status to CMML */
		if (G_UNLIKELY (xmlTextReaderIsEmptyElement (context->reader))) {
			if (G_LIKELY(context->callback != NULL))
				(context->callback) (context->clip, context->user_data);

			context->status = TOTEM_CMML_CMML;
			totem_cmml_clip_free (context->clip);
			context->clip = NULL;
		} else
			context->status = TOTEM_CMML_CLIP;
	} else if (g_strcmp0 ((const gchar *) tag, "img") == 0) {
		if (G_UNLIKELY (context->status != TOTEM_CMML_CLIP))
			return;

		totem_cmml_clip_insert_img_attr (context->clip, attrs);
	}
}

static void
totem_cmml_parse_end (TotemCmmlContext	*context,
		      const xmlChar	*tag)
{
	g_return_if_fail (context != NULL);
	g_return_if_fail (tag != NULL);

	if (g_strcmp0 ((const gchar *) tag, "cmml") == 0) {
		if (G_UNLIKELY (context->status != TOTEM_CMML_CMML))
			return;

		context->status = TOTEM_CMML_NONE;
	} else if (g_strcmp0 ((const gchar *) tag, "head") == 0) {
		if (G_UNLIKELY (context->status != TOTEM_CMML_HEAD))
			return;

		context->status = TOTEM_CMML_CMML;
	} else if (g_strcmp0 ((const gchar *) tag, "clip") == 0) {
		if (G_UNLIKELY (context->status != TOTEM_CMML_CLIP))
			return;

		context->status = TOTEM_CMML_CMML;
		if (G_LIKELY (context->callback != NULL))
			(context->callback) (context->clip, context->user_data);

		totem_cmml_clip_free (context->clip);
		context->clip = NULL;
		return;
	}
}

static void
totem_cmml_parse_xml_node (TotemCmmlContext *context)
{
	xmlChar		*tag;
	xmlChar		**attrs = NULL;
	gint		j, i = 0;

	g_return_if_fail (context != NULL);
	g_return_if_fail (context->reader != NULL);

	tag = xmlStrdup (xmlTextReaderName (context->reader));

	if (xmlTextReaderNodeType (context->reader) == XML_READER_TYPE_ELEMENT) {
		/* read all attributes into the array [name, value, name...] */
		if (xmlTextReaderHasAttributes (context->reader)) {
			attrs = g_new0 (xmlChar *, xmlTextReaderAttributeCount (context->reader) * 2 + 1);

			while (xmlTextReaderMoveToNextAttribute (context->reader)) {
				attrs[i] = xmlStrdup (xmlTextReaderName (context->reader));
				attrs[i + 1] = xmlStrdup (xmlTextReaderValue (context->reader));
				i += 2;
			}

			attrs[i] = NULL;
			xmlTextReaderMoveToElement (context->reader);
		}

		totem_cmml_parse_start (context, tag, (const xmlChar **) attrs);
		/* free resources */
		for (j = i - 1; j >= 0; j -= 1)
			xmlFree (attrs[j]);
		g_free (attrs);
	} else if (xmlTextReaderNodeType (context->reader) == XML_READER_TYPE_END_ELEMENT)
		totem_cmml_parse_end (context, tag);
	xmlFree (tag);
}

static void
totem_cmml_read_clip_cb (TotemCmmlClip	*clip,
			 gpointer	user_data)
{
	TotemCmmlClip	*new_clip;

	g_return_if_fail (clip != NULL);
	g_return_if_fail (user_data != NULL);

	new_clip = totem_cmml_clip_copy (clip);

	if (G_LIKELY (new_clip != NULL && new_clip->time_start >= 0)) {
		* ( (GList **) user_data) = g_list_append ( * ( (GList **) user_data), new_clip);
	/* clip with -1 start time is bad one, remove it */
	} else if (new_clip != NULL) {
		g_warning ("Ignoring clip '%s' due to having an invalid start time: %" G_GINT64_FORMAT, new_clip->title, new_clip->time_start);
		totem_cmml_clip_free (new_clip);
	}
}

/**
 * totem_cmml_convert_msecs_to_str:
 * @time_msecs: time to convert in msecs
 *
 * Converts %time_msecs to string "hh:mm:ss".
 *
 * Returns: string in "hh:mm:ss" format.
 **/
gchar *
totem_cmml_convert_msecs_to_str (gint64 time_msecs)
{
	gint32		hours, minutes, seconds;

	if (G_UNLIKELY (time_msecs < 0))
		hours = minutes = seconds = 0;
	else {
		hours = time_msecs / MSECS_IN_HOUR;
		minutes = (time_msecs % MSECS_IN_HOUR) / MSECS_IN_MINUTE;
		seconds = (time_msecs % MSECS_IN_MINUTE) / MSECS_IN_SECOND;
	}
	return g_strdup_printf ("%.2d:%.2d:%.2d", hours, minutes, seconds);
}

static int
totem_cmml_compare_clips (gconstpointer pointer_a,
			  gconstpointer pointer_b)
{
	TotemCmmlClip	*clip_a, *clip_b;

	g_return_val_if_fail (pointer_a != NULL && pointer_b != NULL, -1);

	clip_a = (TotemCmmlClip *) pointer_a;
	clip_b = (TotemCmmlClip *) pointer_b;

	return clip_a->time_start - clip_b->time_start;
}

/**
 * totem_cmml_clip_new:
 * @title: clip title, %NULL allowed
 * @desc: clip description, %NULL allowed
 * @start: clip start time in msecs
 * @pixbuf: clip thumbnail
 *
 * Creates new clip structure with appropriate parameters.
 *
 * Returns: newly allocated #TotemCmmlClip structure.
 **/
TotemCmmlClip *
totem_cmml_clip_new (const gchar	*title,
		     const gchar	*desc,
		     gint64		start,
		     GdkPixbuf		*pixbuf)
{
	TotemCmmlClip		*clip;

	clip = g_new0 (TotemCmmlClip, 1);

	clip->title = g_strdup (title);
	clip->desc = g_strdup (desc);
	clip->time_start = start;
	if (G_LIKELY (pixbuf != NULL))
		clip->pixbuf = g_object_ref (pixbuf);

	return clip;
}

/**
 * totem_cmml_clip_free:
 * @clip: #TotemCmmlClip to free
 *
 * Frees unused clip structure.
 **/
void
totem_cmml_clip_free (TotemCmmlClip *clip)
{
	if (clip == NULL)
		return;

	if (G_LIKELY (clip->pixbuf != NULL))
		g_object_unref (clip->pixbuf);
	g_free (clip->title);
	g_free (clip->desc);
	g_free (clip);
}

/**
 * totem_cmml_clip_copy:
 * @clip: #TotemCmmlClip structure to copy
 *
 * Copies #TotemCmmlClip structure.
 *
 * Returns: newly allocated #TotemCmmlClip if @clip != %NULL, %NULL otherwise.
 **/
TotemCmmlClip *
totem_cmml_clip_copy (TotemCmmlClip *clip)
{
	g_return_val_if_fail (clip != NULL, NULL);

	return totem_cmml_clip_new (clip->title, clip->desc, clip->time_start, clip->pixbuf);
}

static void
totem_cmml_read_file_cb (GObject	*source_object,
			 GAsyncResult	*result,
			 gpointer	 user_data)
{
	GError			*error = NULL;
	xmlTextReaderPtr	reader;
	TotemCmmlContext	*context;
	gint			ret;
	gchar			*contents;
	gsize			length;
	gboolean		load_ret;

	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
	GList *list = NULL;

	load_ret = g_file_load_contents_finish (G_FILE (source_object), result, &contents, &length, NULL, &error);
	g_object_unref (source_object);

	if (!load_ret) {
		g_simple_async_result_set_from_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);
		g_object_unref (simple);
		g_error_free (error);
		return;
	}

	/* parse in-memory xml data */
	reader = xmlReaderForMemory (contents, length, "", NULL, 0);
	if (G_UNLIKELY (reader == NULL)) {
		g_simple_async_result_set_error (simple, G_IO_ERROR, G_IO_ERROR_FAILED,
						 "Failed to parse CMML file");
		g_simple_async_result_complete_in_idle (simple);
		g_object_unref (simple);
		return;
	}

	context = totem_cmml_context_new ();
	context->reader = reader;
	totem_cmml_context_set_callback (context, totem_cmml_read_clip_cb, &list);

	ret = xmlTextReaderRead (reader);
	while (ret == 1) {
		totem_cmml_parse_xml_node (context);
		ret = xmlTextReaderRead (reader);
	}

	g_free (contents);
	xmlFreeTextReader (reader);
	totem_cmml_clip_free (context->clip);
	totem_cmml_context_free (context);

	/* sort clips by time growth */
	list = g_list_sort (list, (GCompareFunc) totem_cmml_compare_clips);

	g_simple_async_result_set_op_res_gpointer (simple, list, NULL);
	g_simple_async_result_complete_in_idle (simple);
	g_object_unref (simple);
}

/**
 * totem_cmml_read_file:
 * @file: a #GFile representing the file to read
 * @cancellable: optional #GCancellable object, %NULL to ignore
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: the data to pass to callback function.
 *
 * Reads and parses a CMML file asynchronously.
 **/
void
totem_cmml_read_file (GFile               *file,
		      GCancellable        *cancellable,
		      GAsyncReadyCallback  callback,
		      gpointer             user_data)
{
	GSimpleAsyncResult *simple;

	simple = g_simple_async_result_new (G_OBJECT (file),
					    callback,
					    user_data,
					    totem_cmml_read_file);

	g_file_load_contents_async (file, cancellable, totem_cmml_read_file_cb, simple);
}

/**
 * totem_ccml_read_file_finish:
 * @file: a #GFile representing the file to read
 * @res: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Returns a list of parsed chapters or %NULL on error
 **/
GList *
totem_cmml_read_file_finish (GFile        *file,
			     GAsyncResult *res,
			     GError      **error)
{
	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);

	g_return_val_if_fail (G_IS_FILE (file), NULL);

	g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == totem_cmml_read_file);

	if (g_simple_async_result_propagate_error (simple, error))
		return NULL;

	return g_simple_async_result_get_op_res_gpointer (simple);
}

static void
totem_cmml_write_file_result (GObject		*source_object,
			      GAsyncResult	*result,
			      gpointer		user_data)
{
	GError			*error = NULL;
	TotemCmmlAsyncData	*data;

	data = (TotemCmmlAsyncData *) user_data;

	g_file_replace_contents_finish (G_FILE (source_object), result, NULL, &error);
	g_object_unref (source_object);

	if (G_UNLIKELY (error != NULL)) {
		g_warning ("chapters: failed to write CMML file %s: %s", data->file, error->message);
		data->error = g_strdup (error->message);
		data->successful = FALSE;
		g_error_free (error);
		(data->final) (data, NULL);
		return;
	}

	g_free (data->buf);
	data->successful = TRUE;
	(data->final) (data, NULL);
}

/**
 * totem_cmml_write_file_async:
 * @data: #TotemCmmlAsyncData structure with info needed
 *
 * Writes CMML file with clips given.
 *
 * Returns: 0 if no errors occurred while starting async writing, -1 otherwise.
 **/
gint
totem_cmml_write_file_async (TotemCmmlAsyncData *data)
{
	GFile			*gio_file;
	gint			res, len;
	GList			*cur_clip;
	xmlTextWriterPtr	writer;
	xmlBufferPtr		buf;

	g_return_val_if_fail (data != NULL, -1);
	g_return_val_if_fail (data->file != NULL, -1);
	g_return_val_if_fail (data->final != NULL, -1);

	buf = xmlBufferCreate ();
	if (G_UNLIKELY (buf == NULL)) {
		g_warning ("chapters: failed to create xml buffer");
		return -1;
	}

	writer = xmlNewTextWriterMemory (buf, 0);
	if (G_UNLIKELY (writer == NULL)) {
		g_warning ("chapters: failed to create xml buffer");
		xmlBufferFree (buf);
		return -1;
	}

	res = xmlTextWriterStartDocument (writer, "1.0", "UTF-8", "yes");
	if (G_UNLIKELY (res < 0)) {
		xmlBufferFree (buf);
		xmlFreeTextWriter (writer);
		return -1;
	}

	/* CMML preamble */
	res = xmlTextWriterWriteRaw (writer, (const xmlChar *) TOTEM_CMML_PREAMBLE);
	if (G_UNLIKELY (res < 0)) {
		xmlBufferFree (buf);
		xmlFreeTextWriter (writer);
		return -1;
	}

	/* start <cmml> tag */
	res = xmlTextWriterStartElement (writer, (const xmlChar *) "cmml");
	if (G_UNLIKELY (res < 0)) {
		xmlBufferFree (buf);
		xmlFreeTextWriter (writer);
		return -1;
	}

	res = xmlTextWriterWriteRaw (writer, (const xmlChar *) "\n");
	if (G_UNLIKELY (res < 0)) {
		xmlBufferFree (buf);
		xmlFreeTextWriter (writer);
		return -1;
	}

	/* write <head> tag */
	res = xmlTextWriterWriteElement (writer, (const xmlChar *) "head", (const xmlChar *) "");
	if (G_UNLIKELY (res < 0)) {
		xmlBufferFree (buf);
		xmlFreeTextWriter (writer);
		return -1;
	}

	res = xmlTextWriterWriteRaw (writer, (const xmlChar *) "\n");
	if (G_UNLIKELY (res < 0)) {
		xmlBufferFree (buf);
		xmlFreeTextWriter (writer);
		return -1;
	}

	/* iterate through clip list */
	cur_clip = data->list;
	while (cur_clip != NULL) {

		gdouble		time_start;
		gchar 		*base64_enc;
		GdkPixdata 	*pixdata;
		guint		st_len;
		guint8		*stream;
		TotemCmmlClip	*clip;
		gchar		start_buf[G_ASCII_DTOSTR_BUF_SIZE];
		gchar		*start_string;
		gint		hours, minutes;
		gdouble		seconds;

		clip = (TotemCmmlClip *) cur_clip->data;
		time_start = ((gdouble) clip->time_start) / 1000;

		/* start <clip> tag */
		res = xmlTextWriterStartElement (writer, (const xmlChar *) "clip");
		if (G_UNLIKELY (res < 0))
			break;

		res = xmlTextWriterWriteAttribute (writer, (const xmlChar *) "title", (const xmlChar *) clip->title);
		if (G_UNLIKELY (res < 0))
			break;

		/* Format the time in NPT format (npt:%d:%d:%f) */
		hours = ((glong) time_start) / 3600;
		minutes = ((glong) time_start % 3600) / 60;
		seconds = time_start - ((glong) hours * 3600) - ((glong) minutes * 60);
		start_string = g_strdup_printf ("npt:%d:%d:%s", hours, minutes, g_ascii_dtostr (start_buf, sizeof (buf), seconds));
		res = xmlTextWriterWriteAttribute (writer, (const xmlChar *) "start", (const xmlChar *) start_string);
		g_free (start_string);
		if (G_UNLIKELY (res < 0))
			break;

		res = xmlTextWriterWriteRaw (writer, (const xmlChar *) "\n");
		if (G_UNLIKELY (res < 0))
			break;

		/* start <img> tag */
		res = xmlTextWriterStartElement (writer, (const xmlChar *) "img");
		if (G_UNLIKELY (res < 0))
			break;

		if (G_LIKELY (((TotemCmmlClip *) cur_clip->data)->pixbuf != NULL)) {
			pixdata = g_new0 (GdkPixdata, 1);

			/* encode and serialize pixbuf data */
			gdk_pixdata_from_pixbuf (pixdata, ((TotemCmmlClip *) cur_clip->data)->pixbuf, TRUE);
			stream = gdk_pixdata_serialize (pixdata, &st_len);
			base64_enc = g_base64_encode (stream, st_len);

			g_free (pixdata->pixel_data);
			g_free (pixdata);
			g_free (stream);
		}
		else
			base64_enc = g_strdup ("");

		if (g_strcmp0 (base64_enc, "") != 0) {
			res = xmlTextWriterWriteAttribute (writer, (const xmlChar *) "src", (const xmlChar *) base64_enc);
			if (G_UNLIKELY (res < 0)) {
				g_free (base64_enc);
				break;
			}
		}
		g_free (base64_enc);

		/* end <img> tag */
		res = xmlTextWriterEndElement (writer);
		if (G_UNLIKELY (res < 0))
			break;

		res = xmlTextWriterWriteRaw (writer, (const xmlChar *) "\n");
		if (G_UNLIKELY (res < 0))
			break;

		/* end <clip> tag */
		res = xmlTextWriterEndElement (writer);
		if (G_UNLIKELY (res < 0))
			break;

		res = xmlTextWriterWriteRaw (writer, (const xmlChar *) "\n");
		if (G_UNLIKELY (res < 0))
			break;

		cur_clip = cur_clip->next;
	}

	if (G_UNLIKELY (res < 0)) {
		xmlBufferFree (buf);
		xmlFreeTextWriter (writer);
		return -1;
	}

	/* end <cmml> tag*/
	res = xmlTextWriterEndElement (writer);
	if (G_UNLIKELY (res < 0)) {
		xmlBufferFree (buf);
		xmlFreeTextWriter (writer);
		return -1;
	}

	res = xmlTextWriterEndDocument (writer);
	if (G_UNLIKELY (res < 0)) {
		xmlBufferFree (buf);
		xmlFreeTextWriter (writer);
		return -1;
	}

	data->buf = g_strdup ((const char *) xmlBufferContent (buf));
	len = xmlBufferLength (buf);
	xmlBufferFree (buf);
	xmlFreeTextWriter (writer);

	gio_file = g_file_new_for_uri (data->file);
	g_file_replace_contents_async (gio_file, data->buf, len, NULL, FALSE,
				       G_FILE_CREATE_NONE, data->cancellable,
				       (GAsyncReadyCallback) totem_cmml_write_file_result, data);

	return 0;
}
