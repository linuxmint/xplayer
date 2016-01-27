/* 
 * Copyright (C) 2003,2004 Bastien Nocera <hadess@hadess.net>
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

#include "config.h"

#define GST_USE_UNSTABLE_API 1

#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <cairo.h>
#include <gst/gst.h>
#include <totem-pl-parser.h>

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gst/totem-gst-helpers.h"
#include "gst/totem-time-helpers.h"
#include "gst/totem-gst-pixbuf-helpers.h"
#include "video-utils.h"
#include "totem-resources.h"

#ifdef G_HAVE_ISO_VARARGS
#define PROGRESS_DEBUG(...) { if (verbose != FALSE) g_message (__VA_ARGS__); }
#elif defined(G_HAVE_GNUC_VARARGS)
#define PROGRESS_DEBUG(format...) { if (verbose != FALSE) g_message (format); }
#endif

/* The main() function controls progress in the first and last 10% */
#define PRINT_PROGRESS(p) { if (print_progress) g_printf ("%f%% complete\n", p); }
#define MIN_PROGRESS 10.0
#define MAX_PROGRESS 90.0

#define BORING_IMAGE_VARIANCE 256.0		/* Tweak this if necessary */
#define GALLERY_MIN 3				/* minimum number of screenshots in a gallery */
#define GALLERY_MAX 30				/* maximum number of screenshots in a gallery */
#define GALLERY_HEADER_HEIGHT 66		/* header height (in pixels) for the gallery */
#define DEFAULT_OUTPUT_SIZE 256

static gboolean jpeg_output = FALSE;
static gboolean raw_output = FALSE;
static int output_size = -1;
static gboolean time_limit = TRUE;
static gboolean verbose = FALSE;
static gboolean print_progress = FALSE;
static gboolean g_fatal_warnings = FALSE;
static gint gallery = -1;
static gint64 second_index = -1;
static char **filenames = NULL;

typedef struct {
	const char *output;
	const char *input;
	GstElement *play;
	gint64      duration;
} ThumbApp;

static void save_pixbuf (GdkPixbuf *pixbuf, const char *path,
			 const char *video_path, int size, gboolean is_still);

static void
entry_parsed_cb (TotemPlParser *parser,
		 const char    *uri,
		 GHashTable    *metadata,
		 char         **new_url)
{
	*new_url = g_strdup (uri);
}

static char *
get_special_url (GFile *file)
{
	char *path, *orig_uri, *uri, *mime_type;
	TotemPlParser *parser;
	TotemPlParserResult res;

	path = g_file_get_path (file);

	mime_type = g_content_type_guess (path, NULL, 0, NULL);
	g_free (path);
	if (g_strcmp0 (mime_type, "application/x-cd-image") != 0) {
		g_free (mime_type);
		return NULL;
	}
	g_free (mime_type);

	uri = NULL;
	orig_uri = g_file_get_uri (file);

	parser = totem_pl_parser_new ();
	g_signal_connect (parser, "entry-parsed",
			  G_CALLBACK (entry_parsed_cb), &uri);

	res = totem_pl_parser_parse (parser, orig_uri, FALSE);

	g_free (orig_uri);
	g_object_unref (parser);

	if (res == TOTEM_PL_PARSER_RESULT_SUCCESS)
		return uri;

	g_free (uri);

	return NULL;
}

static gboolean
is_special_uri (const char *uri)
{
	if (g_str_has_prefix (uri, "dvd://") ||
	    g_str_has_prefix (uri, "vcd://"))
		return TRUE;

	return FALSE;
}

static void
thumb_app_set_filename (ThumbApp *app)
{
	GFile *file;
	char *uri;

	if (is_special_uri (app->input)) {
		g_object_set (app->play, "uri", app->input, NULL);
		return;
	}

	file = g_file_new_for_commandline_arg (app->input);
	uri = get_special_url (file);
	if (uri == NULL)
		uri = g_file_get_uri (file);
	g_object_unref (file);

	PROGRESS_DEBUG("setting URI %s", uri);

	g_object_set (app->play, "uri", uri, NULL);
	g_free (uri);
}

static GstBusSyncReply
error_handler (GstBus *bus,
	       GstMessage *message,
	       GstElement *play)
{
	GstMessageType msg_type;

	msg_type = GST_MESSAGE_TYPE (message);
	switch (msg_type) {
	case GST_MESSAGE_ERROR:
		totem_gst_message_print (message, play, "totem-video-thumbnailer-error");
		exit (1);
	case GST_MESSAGE_EOS:
		exit (0);

	default:
		/* Ignored */
		;;
	}

	return GST_BUS_PASS;
}

static void
thumb_app_cleanup (ThumbApp *app)
{
	gst_element_set_state (app->play, GST_STATE_NULL);
	g_object_unref (app->play);
	app->play = NULL;
}

static void
thumb_app_set_error_handler (ThumbApp *app)
{
	GstBus *bus;

	bus = gst_element_get_bus (app->play);
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) error_handler, app->play, NULL);
}

static void
check_cover_for_stream (ThumbApp   *app,
			const char *signal_name)
{
	GdkPixbuf *pixbuf;
	GstTagList *tags = NULL;

	g_signal_emit_by_name (G_OBJECT (app->play), signal_name, 0, &tags);

	if (!tags)
		return;

	pixbuf = totem_gst_tag_list_get_cover (tags);
	if (!pixbuf) {
		gst_tag_list_unref (tags);
		return;
	}

	PROGRESS_DEBUG("Saving cover image");
	thumb_app_cleanup (app);
	save_pixbuf (pixbuf, app->output, app->input, output_size, TRUE);
	g_object_unref (pixbuf);

	exit (0);
}

static void
thumb_app_check_for_cover (ThumbApp *app)
{
	PROGRESS_DEBUG ("Checking whether file has cover");
	check_cover_for_stream (app, "get-audio-tags");
	check_cover_for_stream (app, "get-video-tags");
}

static gboolean
thumb_app_set_duration (ThumbApp *app)
{
	gint64 len = -1;

	if (gst_element_query_duration (app->play, GST_FORMAT_TIME, &len) && len != -1) {
		app->duration = len / GST_MSECOND;
		return TRUE;
	}
	app->duration = -1;
	return FALSE;
}

static void
assert_duration (ThumbApp *app)
{
	if (app->duration != -1)
		return;
	g_print ("totem-video-thumbnailer couldn't get the duration of file '%s'\n", app->input);
	exit (1);
}

static gboolean
thumb_app_get_has_video (ThumbApp *app)
{
	guint n_video;
	g_object_get (app->play, "n-video", &n_video, NULL);
	return n_video > 0;
}

static gboolean
thumb_app_start (ThumbApp *app)
{
	GstBus *bus;
	GstMessageType events;
	gboolean terminate = FALSE;
	gboolean async_received = FALSE;

	gst_element_set_state (app->play, GST_STATE_PAUSED);
	bus = gst_element_get_bus (app->play);
	events = GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_ERROR;

	while (terminate == FALSE) {
		GstMessage *message;
		GstElement *src;

		message = gst_bus_timed_pop_filtered (bus,
		                                      GST_CLOCK_TIME_NONE,
		                                      events);

		src = (GstElement*)GST_MESSAGE_SRC (message);

		switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_ASYNC_DONE:
			if (src == app->play) {
				async_received = TRUE;
				terminate = TRUE;
			}
			break;
		case GST_MESSAGE_ERROR:
			totem_gst_message_print (message, app->play, "totem-video-thumbnailer-error");
			terminate = TRUE;
			break;

		default:
			/* Ignore */
			;;
		}

		gst_message_unref (message);
	}

	gst_object_unref (bus);

	if (async_received) {
		/* state change succeeded */
		GST_DEBUG ("state change to %s succeeded", gst_element_state_get_name (GST_STATE_PAUSED));
	}

	return async_received;
}

static void
thumb_app_setup_play (ThumbApp *app)
{
	GstElement *play;
	GstElement *audio_sink, *video_sink;
	GstRegistry *registry;
	GstPluginFeature *feature;

	play = gst_element_factory_make ("playbin", "play");
	audio_sink = gst_element_factory_make ("fakesink", "audio-fake-sink");
	video_sink = gst_element_factory_make ("fakesink", "video-fake-sink");
	g_object_set (video_sink, "sync", TRUE, NULL);

	g_object_set (play,
		      "audio-sink", audio_sink,
		      "video-sink", video_sink,
		      "flags", GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO,
		      NULL);

	app->play = play;

	/* Disable the vaapi plugin as it will not work with the
	 * fakesink we use:
	 * See: https://bugzilla.gnome.org/show_bug.cgi?id=700186 */
	registry = gst_registry_get ();
	feature = gst_registry_find_feature (registry,
					     "vaapidecode",
					     GST_TYPE_ELEMENT_FACTORY);
	if (!feature)
		return;
	gst_registry_remove_feature (registry, feature);
}

static void
thumb_app_seek (ThumbApp *app,
		gint64    _time)
{
	gst_element_seek (app->play, 1.0,
			  GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
			  GST_SEEK_TYPE_SET, _time * GST_MSECOND,
			  GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
	/* And wait for this seek to complete */
	gst_element_get_state (app->play, NULL, NULL, GST_CLOCK_TIME_NONE);
}

static GdkPixbuf *
add_holes_to_pixbuf_small (GdkPixbuf *pixbuf, int width, int height)
{
	GdkPixbuf *holes, *tmp, *target;
	char *filename;
	int i;

	filename = g_build_filename (DATADIR, "totem", "filmholes.png", NULL);
	holes = gdk_pixbuf_new_from_file (filename, NULL);
	g_free (filename);

	if (holes == NULL) {
		g_object_ref (pixbuf);
		return pixbuf;
	}

	g_assert (gdk_pixbuf_get_has_alpha (pixbuf) == FALSE);
	g_assert (gdk_pixbuf_get_has_alpha (holes) != FALSE);
	target = g_object_ref (pixbuf);

	for (i = 0; i < height; i += gdk_pixbuf_get_height (holes))
	{
		gdk_pixbuf_composite (holes, target, 0, i,
				      MIN (width, gdk_pixbuf_get_width (holes)),
				      MIN (height - i, gdk_pixbuf_get_height (holes)),
				      0, i, 1, 1, GDK_INTERP_NEAREST, 255);
	}

	tmp = gdk_pixbuf_flip (holes, FALSE);
	g_object_unref (holes);
	holes = tmp;

	for (i = 0; i < height; i += gdk_pixbuf_get_height (holes))
	{
		gdk_pixbuf_composite (holes, target,
				      width - gdk_pixbuf_get_width (holes), i,
				      MIN (width, gdk_pixbuf_get_width (holes)),
				      MIN (height - i, gdk_pixbuf_get_height (holes)),
				      width - gdk_pixbuf_get_width (holes), i,
				      1, 1, GDK_INTERP_NEAREST, 255);
	}

	g_object_unref (holes);

	return target;
}

static GdkPixbuf *
add_holes_to_pixbuf_large (GdkPixbuf *pixbuf, int size)
{
	char *filename;
	int lh, lw, rh, rw, i;
	GdkPixbuf *left, *right, *small;
	int canvas_w, canvas_h;
	int d_height, d_width;
	double ratio;

	filename = g_build_filename (DATADIR, "totem",
			"filmholes-big-left.png", NULL);
	left = gdk_pixbuf_new_from_file (filename, NULL);
	g_free (filename);

	if (left == NULL) {
		g_object_ref (pixbuf);
		return pixbuf;
	}

	filename = g_build_filename (DATADIR, "totem",
			"filmholes-big-right.png", NULL);
	right = gdk_pixbuf_new_from_file (filename, NULL);
	g_free (filename);

	if (right == NULL) {
		g_object_unref (left);
		g_object_ref (pixbuf);
		return pixbuf;
	}

	lh = gdk_pixbuf_get_height (left);
	lw = gdk_pixbuf_get_width (left);
	rh = gdk_pixbuf_get_height (right);
	rw = gdk_pixbuf_get_width (right);
	g_assert (lh == rh);
	g_assert (lw == rw);

	{
		int height, width;

		height = gdk_pixbuf_get_height (pixbuf);
		width = gdk_pixbuf_get_width (pixbuf);

		if (width > height) {
			d_width = size - lw - lw;
			d_height = d_width * height / width;
		} else {
			d_height = size - lw -lw;
			d_width = d_height * width / height;
		}

		canvas_h = d_height;
		canvas_w = d_width + 2 * lw;
	}

	small = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
			canvas_w, canvas_h);
	gdk_pixbuf_fill (small, 0x000000ff);
	ratio = ((double)d_width / (double) gdk_pixbuf_get_width (pixbuf));

	gdk_pixbuf_scale (pixbuf, small, lw, 0,
			d_width, d_height,
			lw, 0, ratio, ratio, GDK_INTERP_BILINEAR);

	/* Left side holes */
	for (i = 0; i < canvas_h; i += lh) {
		gdk_pixbuf_composite (left, small, 0, i,
				MIN (canvas_w, lw),
				MIN (canvas_h - i, lh),
				0, i, 1, 1, GDK_INTERP_NEAREST, 255);
	}

	/* Right side holes */
	for (i = 0; i < canvas_h; i += rh) {
		gdk_pixbuf_composite (right, small,
				canvas_w - rw, i,
				MIN (canvas_w, rw),
				MIN (canvas_h - i, rh),
				canvas_w - rw, i,
				1, 1, GDK_INTERP_NEAREST, 255);
	}

	/* TODO Add a one pixel border of 0x33333300 all around */

	return small;
}

/* This function attempts to detect images that are mostly solid images
 * It does this by calculating the statistical variance of the
 * black-and-white image */
static gboolean
is_image_interesting (GdkPixbuf *pixbuf)
{
	/* We're gonna assume 8-bit samples. If anyone uses anything different,
	 * it doesn't really matter cause it's gonna be ugly anyways */
	int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
	int height = gdk_pixbuf_get_height(pixbuf);
	guchar* buffer = gdk_pixbuf_get_pixels(pixbuf);
	int num_samples = (rowstride * height);
	int i;
	float x_bar = 0.0f;
	float variance = 0.0f;

	/* FIXME: If this proves to be a performance issue, this function
	 * can be modified to perhaps only check 3 rows. I doubt this'll
	 * be a problem though. */

	/* Iterate through the image to calculate x-bar */
	for (i = 0; i < num_samples; i++) {
		x_bar += (float) buffer[i];
	}
	x_bar /= ((float) num_samples);

	/* Calculate the variance */
	for (i = 0; i < num_samples; i++) {
		float tmp = ((float) buffer[i] - x_bar);
		variance += tmp * tmp;
	}
	variance /= ((float) (num_samples - 1));

	return (variance > BORING_IMAGE_VARIANCE);
}

static GdkPixbuf *
scale_pixbuf (GdkPixbuf *pixbuf, int size, gboolean is_still)
{
	GdkPixbuf *result;
	int width, height;
	int d_width, d_height;

	if (size != -1) {
		height = gdk_pixbuf_get_height (pixbuf);
		width = gdk_pixbuf_get_width (pixbuf);

		if (width > height) {
			d_width = size;
			d_height = size * height / width;
		} else {
			d_height = size;
			d_width = size * width / height;
		}
	} else {
		d_width = d_height = -1;
	}

	if (size <= 256) {
		GdkPixbuf *small;

		small = gdk_pixbuf_scale_simple (pixbuf, d_width, d_height, GDK_INTERP_BILINEAR);

		if (is_still == FALSE) {
			result = add_holes_to_pixbuf_small (small, d_width, d_height);
			g_return_val_if_fail (result != NULL, NULL);
			g_object_unref (small);
		} else {
			result = small;
		}
	} else {
		if (is_still == FALSE) {
			result = add_holes_to_pixbuf_large (pixbuf, size);
			g_return_val_if_fail (result != NULL, NULL);
		} else {
			if (size > 0)
				result = gdk_pixbuf_scale_simple (pixbuf, d_width, d_height, GDK_INTERP_BILINEAR);
			else
				result = g_object_ref (pixbuf);
		}
	}

	return result;
}

static void
save_pixbuf (GdkPixbuf *pixbuf, const char *path,
	     const char *video_path, int size, gboolean is_still)
{
	int width, height;
	GdkPixbuf *with_holes;
	GError *err = NULL;
	gboolean ret;

	height = gdk_pixbuf_get_height (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);

	/* If we're outputting a gallery or a raw image without a size,
	 * don't scale the pixbuf or add borders */
	if (gallery != -1 || (raw_output != FALSE && size == -1))
		with_holes = g_object_ref (pixbuf);
	else if (raw_output != FALSE)
		with_holes = scale_pixbuf (pixbuf, size, TRUE);
	else
		with_holes = scale_pixbuf (pixbuf, size, is_still);


	if (jpeg_output == FALSE) {
		char *a_width, *a_height;

		a_width = g_strdup_printf ("%d", width);
		a_height = g_strdup_printf ("%d", height);

		ret = gdk_pixbuf_save (with_holes, path, "png", &err,
				       "tEXt::Thumb::Image::Width", a_width,
				       "tEXt::Thumb::Image::Height", a_height,
				       NULL);
	} else {
		ret = gdk_pixbuf_save (with_holes, path, "jpeg", &err, NULL);
	}

	if (ret == FALSE) {
		if (err != NULL) {
			g_print ("totem-video-thumbnailer couldn't write the thumbnail '%s' for video '%s': %s\n", path, video_path, err->message);
			g_error_free (err);
		} else {
			g_print ("totem-video-thumbnailer couldn't write the thumbnail '%s' for video '%s'\n", path, video_path);
		}

		g_object_unref (with_holes);
		return;
	}

	g_object_unref (with_holes);
}

static GdkPixbuf *
capture_frame_at_time (ThumbApp   *app,
		       gint64 milliseconds)
{
	if (milliseconds != 0)
		thumb_app_seek (app, milliseconds);

	return totem_gst_playbin_get_frame (app->play);
}

static GdkPixbuf *
capture_interesting_frame (ThumbApp *app)
{
	GdkPixbuf* pixbuf;
	guint current;
	const double frame_locations[] = {
		1.0 / 3.0,
		2.0 / 3.0,
		0.1,
		0.9,
		0.5
	};

	if (app->duration == -1) {
		PROGRESS_DEBUG("Video has no duration, so capture 1st frame");
		return capture_frame_at_time (app, 0);
	}

	/* Test at multiple points in the file to see if we can get an
	 * interesting frame */
	for (current = 0; current < G_N_ELEMENTS(frame_locations); current++)
	{
		PROGRESS_DEBUG("About to seek to %f", frame_locations[current]);
		thumb_app_seek (app, frame_locations[current] * app->duration);

		/* Pull the frame, if it's interesting we bail early */
		PROGRESS_DEBUG("About to get frame for iter %d", current);
		pixbuf = totem_gst_playbin_get_frame (app->play);
		if (pixbuf != NULL && is_image_interesting (pixbuf) != FALSE) {
			PROGRESS_DEBUG("Frame for iter %d is interesting", current);
			break;
		}

		/* If we get to the end of this loop, we'll end up using
		 * the last image we pulled */
		if (current + 1 < G_N_ELEMENTS(frame_locations)) {
			if (pixbuf != NULL) {
				g_object_unref (pixbuf);
				pixbuf = NULL;
			}
		}
		PROGRESS_DEBUG("Frame for iter %d was not interesting", current);
	}
	return pixbuf;
}

static GdkPixbuf *
cairo_surface_to_pixbuf (cairo_surface_t *surface)
{
	gint stride, width, height, x, y;
	guchar *data, *output, *output_pixel;

	/* This doesn't deal with alpha --- it simply converts the 4-byte Cairo ARGB
	 * format to the 3-byte GdkPixbuf packed RGB format. */
	g_assert (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_RGB24);

	stride = cairo_image_surface_get_stride (surface);
	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);
	data = cairo_image_surface_get_data (surface);

	output = g_malloc (stride * height);
	output_pixel = output;

	for (y = 0; y < height; y++) {
		guint32 *row = (guint32*) (data + y * stride);

		for (x = 0; x < width; x++) {
			output_pixel[0] = (row[x] & 0x00ff0000) >> 16;
			output_pixel[1] = (row[x] & 0x0000ff00) >> 8;
			output_pixel[2] = (row[x] & 0x000000ff);

			output_pixel += 3;
		}
	}

	return gdk_pixbuf_new_from_data (output, GDK_COLORSPACE_RGB, FALSE, 8,
					 width, height, width * 3,
					 (GdkPixbufDestroyNotify) g_free, NULL);
}


static GdkPixbuf *
create_gallery (ThumbApp *app)
{
	GdkPixbuf *screenshot, *pixbuf = NULL;
	cairo_t *cr;
	cairo_surface_t *surface;
	PangoLayout *layout;
	PangoFontDescription *font_desc;
	gint64 stream_length, screenshot_interval, pos;
	guint columns = 3, rows, current_column, current_row, x, y;
	gint screenshot_width = 0, screenshot_height = 0, x_padding = 0, y_padding = 0;
	gfloat scale = 1.0;
	gchar *header_text, *duration_text, *filename;

	/* Calculate how many screenshots we're going to take */
	stream_length = app->duration;

	/* As a default, we have one screenshot per minute of stream,
	 * but adjusted so we don't have any gaps in the resulting gallery. */
	if (gallery == 0) {
		gallery = stream_length / 60000;

		while (gallery % 3 != 0 &&
		       gallery % 4 != 0 &&
		       gallery % 5 != 0) {
			gallery++;
		}
	}

	if (gallery < GALLERY_MIN)
		gallery = GALLERY_MIN;
	if (gallery > GALLERY_MAX)
		gallery = GALLERY_MAX;
	screenshot_interval = stream_length / gallery;

	/* Put a lower bound on the screenshot interval so we can't enter an infinite loop below */
	if (screenshot_interval == 0)
		screenshot_interval = 1;

	PROGRESS_DEBUG ("Producing gallery of %u screenshots, taken at %" G_GINT64_FORMAT " millisecond intervals throughout a %" G_GINT64_FORMAT " millisecond-long stream.",
			gallery, screenshot_interval, stream_length);

	/* Calculate how to arrange the screenshots so we don't get ones orphaned on the last row.
	 * At this point, only deal with arrangements of 3, 4 or 5 columns. */
	y = G_MAXUINT;
	for (x = 3; x <= 5; x++) {
		if (gallery % x == 0 || x - gallery % x < y) {
			y = x - gallery % x;
			columns = x;

			/* Have we found an optimal solution already? */
			if (y == x)
				break;
		}
	}

	rows = ceil ((gfloat) gallery / (gfloat) columns);

	PROGRESS_DEBUG ("Outputting as %u rows and %u columns.", rows, columns);

	/* Take the screenshots and composite them into a pixbuf */
	current_column = current_row = x = y = 0;
	for (pos = screenshot_interval; pos <= stream_length; pos += screenshot_interval) {
		if (pos == stream_length)
			screenshot = capture_frame_at_time (app, pos - 1);
		else
			screenshot = capture_frame_at_time (app, pos);

		if (pixbuf == NULL) {
			screenshot_width = gdk_pixbuf_get_width (screenshot);
			screenshot_height = gdk_pixbuf_get_height (screenshot);

			/* Calculate a scaling factor so that screenshot_width -> output_size */
			scale = (float) output_size / (float) screenshot_width;

			x_padding = x = MAX (output_size * 0.05, 1);
			y_padding = y = MAX (scale * screenshot_height * 0.05, 1);

			PROGRESS_DEBUG ("Scaling each screenshot by %f.", scale);

			/* Create our massive pixbuf */
			pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
						 columns * output_size + (columns + 1) * x_padding,
						 (guint) (rows * scale * screenshot_height + (rows + 1) * y_padding));
			gdk_pixbuf_fill (pixbuf, 0x000000ff);

			PROGRESS_DEBUG ("Created output pixbuf (%ux%u).", gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf));
		}

		/* Composite the screenshot into our gallery */
		gdk_pixbuf_composite (screenshot, pixbuf,
				      x, y, output_size, scale * screenshot_height,
				      (gdouble) x, (gdouble) y, scale, scale,
				      GDK_INTERP_BILINEAR, 255);
		g_object_unref (screenshot);

		PROGRESS_DEBUG ("Composited screenshot from %" G_GINT64_FORMAT " milliseconds (address %u) at (%u,%u).",
				pos, GPOINTER_TO_UINT (screenshot), x, y);

		/* We print progress in the range 10% (MIN_PROGRESS) to 50% (MAX_PROGRESS - MIN_PROGRESS) / 2.0 */
		PRINT_PROGRESS (MIN_PROGRESS + (current_row * columns + current_column) * (((MAX_PROGRESS - MIN_PROGRESS) / gallery) / 2.0));

		current_column = (current_column + 1) % columns;
		x += output_size + x_padding;
		if (current_column == 0) {
			x = x_padding;
			y += scale * screenshot_height + y_padding;
			current_row++;
		}
	}

	PROGRESS_DEBUG ("Converting pixbuf to a Cairo surface.");

	/* Load the pixbuf into a Cairo surface and overlay the text. The height is the height of
	 * the gallery plus the necessary height for 3 lines of header (at ~18px each), plus some
	 * extra padding. */
	surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, gdk_pixbuf_get_width (pixbuf),
					      gdk_pixbuf_get_height (pixbuf) + GALLERY_HEADER_HEIGHT + y_padding);
	cr = cairo_create (surface);
	cairo_surface_destroy (surface);

	/* First, copy across the gallery pixbuf */
	gdk_cairo_set_source_pixbuf (cr, pixbuf, 0.0, GALLERY_HEADER_HEIGHT + y_padding);
	cairo_rectangle (cr, 0.0, GALLERY_HEADER_HEIGHT + y_padding, gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf));
	cairo_fill (cr);
	g_object_unref (pixbuf);

	/* Build the header information */
	duration_text = totem_time_to_string (stream_length);
	filename = NULL;
	if (strstr (app->input, "://")) {
		char *local;
		local = g_filename_from_uri (app->input, NULL, NULL);
		filename = g_path_get_basename (local);
		g_free (local);
	}
	if (filename == NULL)
		filename = g_path_get_basename (app->input);

	/* Translators: The first string is "Filename" (as translated); the second is an actual filename.
			The third string is "Resolution" (as translated); the fourth and fifth are screenshot height and width, respectively.
			The sixth string is "Duration" (as translated); the seventh is the movie duration in words. */
	header_text = g_markup_printf_escaped (_("<b>%s</b>: %s\n<b>%s</b>: %d\303\227%d\n<b>%s</b>: %s"),
					       _("Filename"),
					       filename,
					       _("Resolution"),
					       screenshot_width,
					       screenshot_height,
					       _("Duration"),
					       duration_text);
	g_free (duration_text);
	g_free (filename);

	PROGRESS_DEBUG ("Writing header text with Pango.");

	/* Write out some header information */
	layout = pango_cairo_create_layout (cr);
	font_desc = pango_font_description_from_string ("Sans 18px");
	pango_layout_set_font_description (layout, font_desc);
	pango_font_description_free (font_desc);

	pango_layout_set_markup (layout, header_text, -1);
	g_free (header_text);

	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
	cairo_move_to (cr, (gdouble) x_padding, (gdouble) y_padding);
	pango_cairo_show_layout (cr, layout);

	/* Go through each screenshot and write its timestamp */
	current_column = current_row = 0;
	x = x_padding + output_size;
	y = y_padding * 2 + GALLERY_HEADER_HEIGHT + scale * screenshot_height;

	font_desc = pango_font_description_from_string ("Sans 10px");
	pango_layout_set_font_description (layout, font_desc);
	pango_font_description_free (font_desc);

	PROGRESS_DEBUG ("Writing screenshot timestamps with Pango.");

	for (pos = screenshot_interval; pos <= stream_length; pos += screenshot_interval) {
		gchar *timestamp_text;
		gint layout_width, layout_height;

		timestamp_text = totem_time_to_string (pos);

		pango_layout_set_text (layout, timestamp_text, -1);
		pango_layout_get_pixel_size (layout, &layout_width, &layout_height);

		/* Display the timestamp in the bottom-right corner of the current screenshot */
		cairo_move_to (cr, x - layout_width - 0.02 * output_size, y - layout_height - 0.02 * scale * screenshot_height);

		/* We have to stroke the text so it's visible against screenshots of the same
		 * foreground color. */
		pango_cairo_layout_path (cr, layout);
		cairo_set_source_rgb (cr, 0.0, 0.0, 0.0); /* black */
		cairo_stroke_preserve (cr);
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
		cairo_fill (cr);

		PROGRESS_DEBUG ("Writing timestamp \"%s\" at (%f,%f).", timestamp_text,
				x - layout_width - 0.02 * output_size,
				y - layout_height - 0.02 * scale * screenshot_height);

		/* We print progress in the range 50% (MAX_PROGRESS - MIN_PROGRESS) / 2.0) to 90% (MAX_PROGRESS) */
		PRINT_PROGRESS (MIN_PROGRESS + (MAX_PROGRESS - MIN_PROGRESS) / 2.0 + (current_row * columns + current_column) * (((MAX_PROGRESS - MIN_PROGRESS) / gallery) / 2.0));

		g_free (timestamp_text);

		current_column = (current_column + 1) % columns;
		x += output_size + x_padding;
		if (current_column == 0) {
			x = x_padding + output_size;
			y += scale * screenshot_height + y_padding;
			current_row++;
		}
	}

	g_object_unref (layout);

	PROGRESS_DEBUG ("Converting Cairo surface back to pixbuf.");

	/* Create a new pixbuf from the Cairo context */
	pixbuf = cairo_surface_to_pixbuf (cairo_get_target (cr));
	cairo_destroy (cr);

	return pixbuf;
}

static const GOptionEntry entries[] = {
	{ "jpeg", 'j',  0, G_OPTION_ARG_NONE, &jpeg_output, "Output the thumbnail as a JPEG instead of PNG", NULL },
	{ "size", 's', 0, G_OPTION_ARG_INT, &output_size, "Size of the thumbnail in pixels (with --gallery sets the size of individual screenshots)", NULL },
	{ "raw", 'r', 0, G_OPTION_ARG_NONE, &raw_output, "Output the raw picture of the video without scaling or adding borders", NULL },
	{ "no-limit", 'l', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &time_limit, "Don't limit the thumbnailing time to 30 seconds", NULL },
	{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Output debug information", NULL },
	{ "time", 't', 0, G_OPTION_ARG_INT64, &second_index, "Choose this time (in seconds) as the thumbnail (can't be used with --gallery)", NULL },
	{ "g-fatal-warnings", 0, 0, G_OPTION_ARG_NONE, &g_fatal_warnings, "Make all warnings fatal", NULL },
	{ "gallery", 'g', 0, G_OPTION_ARG_INT, &gallery, "Output a gallery of the given number (0 is default) of screenshots (can't be used with --time)", NULL },
	{ "print-progress", 'p', 0, G_OPTION_ARG_NONE, &print_progress, "Only print progress updates (can't be used with --verbose)", NULL },
	{ G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL, "[INPUT FILE] [OUTPUT FILE]" },
	{ NULL }
};

int main (int argc, char *argv[])
{
	GOptionGroup *options;
	GOptionContext *context;
	GError *err = NULL;
	GdkPixbuf *pixbuf;
	const char *input, *output;
	ThumbApp app;

	context = g_option_context_new ("Thumbnail movies");
	options = gst_init_get_option_group ();
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, options);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));

	if (g_option_context_parse (context, &argc, &argv, &err) == FALSE) {
		g_print ("couldn't parse command-line options: %s\n", err->message);
		g_error_free (err);
		return 1;
	}

#ifdef G_OS_UNIX
	if (time_limit != FALSE) {
		errno = 0;
		if (nice (20) != 20 && errno != 0)
			g_warning ("Couldn't change nice value of process.");
	}
#endif

	if (print_progress) {
		fcntl (fileno (stdout), F_SETFL, O_NONBLOCK);
		setbuf (stdout, NULL);
	}

	if (g_fatal_warnings) {
		GLogLevelFlags fatal_mask;

		fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
		fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
		g_log_set_always_fatal (fatal_mask);
	}

	if (raw_output == FALSE && output_size == -1)
		output_size = DEFAULT_OUTPUT_SIZE;

	if (filenames == NULL || g_strv_length (filenames) != 2 ||
	    (second_index != -1 && gallery != -1) ||
	    (print_progress == TRUE && verbose == TRUE)) {
		char *help;
		help = g_option_context_get_help (context, FALSE, NULL);
		g_print ("%s", help);
		g_free (help);
		return 1;
	}
	input = filenames[0];
	output = filenames[1];

	PROGRESS_DEBUG("Initialised libraries, about to create video widget");
	PRINT_PROGRESS (2.0);

	app.input = input;
	app.output = output;

	thumb_app_setup_play (&app);
	thumb_app_set_filename (&app);

	PROGRESS_DEBUG("Video widget created");
	PRINT_PROGRESS (6.0);

	if (time_limit != FALSE)
		totem_resources_monitor_start (input, 0);

	PROGRESS_DEBUG("About to open video file");

	if (thumb_app_start (&app) == FALSE) {
		g_print ("totem-video-thumbnailer couldn't open file '%s'\n", input);
		exit (1);
	}
	thumb_app_set_error_handler (&app);

	/* We don't need covers when we're in gallery mode */
	if (gallery == -1)
		thumb_app_check_for_cover (&app);
	if (thumb_app_get_has_video (&app) == FALSE) {
		PROGRESS_DEBUG ("totem-video-thumbnailer couldn't find a video track in '%s'\n", input);
		exit (1);
	}
	thumb_app_set_duration (&app);

	PROGRESS_DEBUG("Opened video file: '%s'", input);
	PRINT_PROGRESS (10.0);

	if (gallery == -1) {
		/* If the user has told us to use a frame at a specific second
		 * into the video, just use that frame no matter how boring it
		 * is */
		if (second_index != -1) {
			assert_duration (&app);
			pixbuf = capture_frame_at_time (&app, second_index * 1000);
		} else {
			pixbuf = capture_interesting_frame (&app);
		}
		PRINT_PROGRESS (90.0);
	} else {
		assert_duration (&app);
		/* We're producing a gallery of screenshots from throughout the file */
		pixbuf = create_gallery (&app);
	}

	/* Cleanup */
	totem_resources_monitor_stop ();
	thumb_app_cleanup (&app);
	PRINT_PROGRESS (92.0);

	if (pixbuf == NULL) {
		g_print ("totem-video-thumbnailer couldn't get a picture from '%s'\n", input);
		exit (1);
	}

	PROGRESS_DEBUG("Saving captured screenshot");
	save_pixbuf (pixbuf, output, input, output_size, FALSE);
	g_object_unref (pixbuf);
	PRINT_PROGRESS (100.0);

	return 0;
}

