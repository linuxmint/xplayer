/*
 * Copyright (C) 2003-2007 the GStreamer project
 *      Julien Moutte <julien@moutte.net>
 *      Ronald Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2005-2008 Tim-Philipp Müller <tim centricular net>
 * Copyright (C) 2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 * Copyright © 2009 Christian Persch
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

#include "totem-gst-pixbuf-helpers.h"

#include <gst/tag/tag.h>
#include <gst/video/video-format.h>

static void
destroy_pixbuf (guchar *pix, gpointer data)
{
  gst_sample_unref (GST_SAMPLE (data));
}

GdkPixbuf *
totem_gst_playbin_get_frame (GstElement *play)
{
  GstStructure *s;
  GstSample *sample = NULL;
  GdkPixbuf *pixbuf = NULL;
  GstCaps *to_caps, *sample_caps;
  gint outwidth = 0;
  gint outheight = 0;
  GstMemory *memory;
  GstMapInfo info;

  g_return_val_if_fail (play != NULL, NULL);
  g_return_val_if_fail (GST_IS_ELEMENT (play), NULL);

  /* our desired output format (RGB24) */
  to_caps = gst_caps_new_simple ("video/x-raw",
      "format", G_TYPE_STRING, "RGB",
      /* Note: we don't ask for a specific width/height here, so that
       * videoscale can adjust dimensions from a non-1/1 pixel aspect
       * ratio to a 1/1 pixel-aspect-ratio. We also don't ask for a
       * specific framerate, because the input framerate won't
       * necessarily match the output framerate if there's a deinterlacer
       * in the pipeline. */
      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
      NULL);

  /* get frame */
  g_signal_emit_by_name (play, "convert-sample", to_caps, &sample);
  gst_caps_unref (to_caps);

  if (!sample) {
    GST_DEBUG ("Could not take screenshot: %s",
        "failed to retrieve or convert video frame");
    g_warning ("Could not take screenshot: %s",
        "failed to retrieve or convert video frame");
    return NULL;
  }

  sample_caps = gst_sample_get_caps (sample);
  if (!sample_caps) {
    GST_DEBUG ("Could not take screenshot: %s", "no caps on output buffer");
    g_warning ("Could not take screenshot: %s", "no caps on output buffer");
    return NULL;
  }

  GST_DEBUG ("frame caps: %" GST_PTR_FORMAT, sample_caps);

  s = gst_caps_get_structure (sample_caps, 0);
  gst_structure_get_int (s, "width", &outwidth);
  gst_structure_get_int (s, "height", &outheight);
  if (outwidth <= 0 || outheight <= 0)
    goto done;

  memory = gst_buffer_get_memory (gst_sample_get_buffer (sample), 0);
  gst_memory_map (memory, &info, GST_MAP_READ);

  /* create pixbuf from that - use our own destroy function */
  pixbuf = gdk_pixbuf_new_from_data (info.data,
      GDK_COLORSPACE_RGB, FALSE, 8, outwidth, outheight,
      GST_ROUND_UP_4 (outwidth * 3), destroy_pixbuf, sample);

  gst_memory_unmap (memory, &info);

done:
  if (!pixbuf) {
    GST_DEBUG ("Could not take screenshot: %s", "could not create pixbuf");
    g_warning ("Could not take screenshot: %s", "could not create pixbuf");
    gst_sample_unref (sample);
  }

  return pixbuf;
}

static GdkPixbuf *
totem_gst_buffer_to_pixbuf (GstBuffer *buffer)
{
  GdkPixbufLoader *loader;
  GdkPixbuf *pixbuf = NULL;
  GError *err = NULL;
  GstMapInfo info;

  if (!gst_buffer_map (buffer, &info, GST_MAP_READ)) {
    GST_WARNING("could not map memory buffer");
    return NULL;
  }

  loader = gdk_pixbuf_loader_new ();

  if (gdk_pixbuf_loader_write (loader, info.data, info.size, &err) &&
      gdk_pixbuf_loader_close (loader, &err)) {
    pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    if (pixbuf)
      g_object_ref (pixbuf);
  } else {
    GST_WARNING("could not convert tag image to pixbuf: %s", err->message);
    g_error_free (err);
  }

  g_object_unref (loader);

  gst_buffer_unmap (buffer, &info);

  return pixbuf;
}

static GstSample *
totem_gst_tag_list_get_cover_real (GstTagList *tag_list)
{
  GstSample *cover_sample = NULL;
  guint i;

  for (i = 0; ; i++) {
    GstSample *sample;
    GstCaps *caps;
    const GstStructure *caps_struct;
    int type = GST_TAG_IMAGE_TYPE_UNDEFINED;

    if (!gst_tag_list_get_sample_index (tag_list, GST_TAG_IMAGE, i, &sample))
      break;

    caps = gst_sample_get_caps (sample);
    caps_struct = gst_caps_get_structure (caps, 0);
    gst_structure_get_enum (caps_struct,
			    "image-type",
			    GST_TYPE_TAG_IMAGE_TYPE,
			    &type);
    if (type == GST_TAG_IMAGE_TYPE_UNDEFINED) {
      if (cover_sample == NULL) {
	/* take a ref here since we will continue and unref below */
	cover_sample = gst_sample_ref (sample);
      }
    } else if (type == GST_TAG_IMAGE_TYPE_FRONT_COVER) {
      cover_sample = sample;
      break;
    }
    gst_sample_unref (sample);
  }

  return cover_sample;
}

GdkPixbuf *
totem_gst_tag_list_get_cover (GstTagList *tag_list)
{
  GstSample *cover_sample;

  g_return_val_if_fail (tag_list != NULL, FALSE);

  cover_sample = totem_gst_tag_list_get_cover_real (tag_list);
  /* Fallback to preview */
    if (!cover_sample) {
      gst_tag_list_get_sample_index (tag_list, GST_TAG_PREVIEW_IMAGE, 0,
				     &cover_sample);
    }

  if (cover_sample) {
    GstBuffer *buffer;
    GdkPixbuf *pixbuf;

    buffer = gst_sample_get_buffer (cover_sample);
    pixbuf = totem_gst_buffer_to_pixbuf (buffer);
    gst_sample_unref (cover_sample);
    return pixbuf;
  }

  return NULL;
}

/*
 * vim: sw=2 ts=8 cindent noai bs=2
 */
