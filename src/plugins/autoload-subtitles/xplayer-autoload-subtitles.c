/*
 *  Copyright (C) 2012 Bastien Nocera <hadess@hadess.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *
 * The Xplayer project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Xplayer. This
 * permission are above and beyond the permissions granted by the GPL license
 * Xplayer is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */


#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "xplayer-plugin.h"
#include "xplayer.h"

#define XPLAYER_TYPE_AUTOLOAD_SUBTITLES_PLUGIN	(xplayer_autoload_subtitles_plugin_get_type ())
#define XPLAYER_AUTOLOAD_SUBTITLES_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), XPLAYER_TYPE_AUTOLOAD_SUBTITLES_PLUGIN, XplayerAutoloadSubtitlesPlugin))

typedef struct {
	guint signal_id;
	XplayerObject *xplayer;
	GSettings *settings;
	gboolean autoload_subs;
} XplayerAutoloadSubtitlesPluginPrivate;

XPLAYER_PLUGIN_REGISTER(XPLAYER_TYPE_AUTOLOAD_SUBTITLES_PLUGIN, XplayerAutoloadSubtitlesPlugin, xplayer_autoload_subtitles_plugin)

/* List from xine-lib's demux_sputext.c.
 * Keep in sync with the list in xplayer_setup_file_filters() in this file.
 * Don't add .txt extensions, as there are too many false positives. */
static const char subtitle_ext[][4] = {
	"sub",
	"srt",
	"smi",
	"ssa",
	"ass",
	"asc"
};

static gboolean
xplayer_uri_exists (const char *uri)
{
	GFile *file = g_file_new_for_uri (uri);
	if (file != NULL) {
		if (g_file_query_exists (file, NULL)) {
			g_object_unref (file);
			return TRUE;
		}
		g_object_unref (file);
	}
	return FALSE;
}

static char *
xplayer_uri_get_subtitle_for_uri (const char *uri)
{
	char *subtitle;
	guint len, i;
	gint suffix;

	g_return_val_if_fail (uri != NULL, NULL);

	/* Find the filename suffix delimiter */
	len = strlen (uri);
	for (suffix = len - 1; suffix > 0; suffix--) {
		if (uri[suffix] == G_DIR_SEPARATOR ||
		    (uri[suffix] == '/')) {
			/* This filename has no extension; we'll need to add one */
			suffix = len;
			break;
		}
		if (uri[suffix] == '.') {
			/* Found our extension marker */
			break;
		}
	}
	if (suffix < 0)
		return NULL;

	/* Generate a subtitle string with room at the end to store the
	 * 3 character extensions for which we want to search */
	subtitle = g_malloc0 (suffix + 4 + 1);
	g_return_val_if_fail (subtitle != NULL, NULL);
	g_strlcpy (subtitle, uri, suffix + 4 + 1);
	g_strlcpy (subtitle + suffix, ".???", 5);

	/* Search for any files with one of our known subtitle extensions */
	for (i = 0; i < G_N_ELEMENTS (subtitle_ext) ; i++) {
		char *subtitle_ext_upper;
		memcpy (subtitle + suffix + 1, subtitle_ext[i], 3);

		if (xplayer_uri_exists (subtitle))
			return subtitle;

		/* Check with upper-cased extension */
		subtitle_ext_upper = g_ascii_strup (subtitle_ext[i], -1);
		memcpy (subtitle + suffix + 1, subtitle_ext_upper, 3);
		g_free (subtitle_ext_upper);

		if (xplayer_uri_exists (subtitle))
			return subtitle;
	}
	g_free (subtitle);
	return NULL;
}

static char *
xplayer_uri_get_subtitle_in_subdir (GFile *file, const char *subdir)
{
	char *filename, *subtitle, *full_path_str;
	GFile *parent, *full_path, *directory;

	/* Get the sibling directory @subdir of the file @file */
	parent = g_file_get_parent (file);
	directory = g_file_get_child (parent, subdir);
	g_object_unref (parent);

	/* Get the file of the same name as @file in the @subdir directory */
	filename = g_file_get_basename (file);
	full_path = g_file_get_child (directory, filename);
	g_object_unref (directory);
	g_free (filename);

	/* Get the subtitles from that URI */
	full_path_str = g_file_get_uri (full_path);
	g_object_unref (full_path);
	subtitle = xplayer_uri_get_subtitle_for_uri (full_path_str);
	g_free (full_path_str);

	return subtitle;
}

static char *
xplayer_uri_get_cached_subtitle_for_uri (const char *uri)
{
	char *filename, *basename, *fake_filename, *fake_uri, *ret;

	filename = g_filename_from_uri (uri, NULL, NULL);
	if (filename == NULL)
		return NULL;

	basename = g_path_get_basename (filename);
	g_free (filename);
	if (basename == NULL || strcmp (basename, ".") == 0) {
		g_free (basename);
		return NULL;
	}

	fake_filename = g_build_filename (g_get_user_cache_dir (),
				"xplayer",
				"subtitles",
				basename,
				NULL);
	g_free (basename);
	fake_uri = g_filename_to_uri (fake_filename, NULL, NULL);
	g_free (fake_filename);

	ret = xplayer_uri_get_subtitle_for_uri (fake_uri);
	g_free (fake_uri);

	return ret;
}

static char *
xplayer_uri_get_subtitle_uri (const char *uri)
{
	GFile *file;
	char *subtitle;

	if (g_str_has_prefix (uri, "http") != FALSE ||
	    g_str_has_prefix (uri, "rtsp") != FALSE ||
	    g_str_has_prefix (uri, "rtmp") != FALSE)
		return NULL;

	/* Has the user specified a subtitle file manually? */
	if (strstr (uri, "#subtitle:") != NULL)
		return NULL;

	/* Does the file exist? */
	file = g_file_new_for_uri (uri);
	if (g_file_query_exists (file, NULL) != TRUE) {
		g_object_unref (file);
		return NULL;
	}

	/* Try in the cached subtitles directory */
	subtitle = xplayer_uri_get_cached_subtitle_for_uri (uri);
	if (subtitle != NULL) {
		g_object_unref (file);
		return subtitle;
	}

	/* Try in the current directory */
	subtitle = xplayer_uri_get_subtitle_for_uri (uri);
	if (subtitle != NULL) {
		g_object_unref (file);
		return subtitle;
	}

	subtitle = xplayer_uri_get_subtitle_in_subdir (file, "subtitles");
	g_object_unref (file);

	return subtitle;
}



static char *
get_text_subtitle_cb (XplayerObject                  *xplayer,
		      const char                   *mrl,
		      XplayerAutoloadSubtitlesPlugin *pi)
{
	char *sub;

	if (pi->priv->autoload_subs == FALSE)
		return NULL;

	sub = xplayer_uri_get_subtitle_uri (mrl);

	return sub;
}

static void
autoload_subs_changed (GSettings                *settings,
		       char                     *key,
		       XplayerAutoloadSubtitlesPlugin *pi)
{
	pi->priv->autoload_subs = g_settings_get_boolean (settings, "autoload-subtitles");
}

static void
impl_activate (PeasActivatable *plugin)
{
	XplayerAutoloadSubtitlesPlugin *pi = XPLAYER_AUTOLOAD_SUBTITLES_PLUGIN (plugin);

	pi->priv->xplayer = g_object_ref (g_object_get_data (G_OBJECT (plugin), "object"));
	pi->priv->settings = g_settings_new ("org.x.player");
	pi->priv->autoload_subs = g_settings_get_boolean (pi->priv->settings, "autoload-subtitles");
	g_signal_connect (pi->priv->settings, "changed::autoload-subtitles",
			  G_CALLBACK (autoload_subs_changed), pi);
	pi->priv->signal_id = g_signal_connect (G_OBJECT (pi->priv->xplayer), "get-text-subtitle",
						G_CALLBACK (get_text_subtitle_cb), pi);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	XplayerAutoloadSubtitlesPlugin *pi = XPLAYER_AUTOLOAD_SUBTITLES_PLUGIN (plugin);

	if (pi->priv->signal_id) {
		g_signal_handler_disconnect (pi->priv->xplayer, pi->priv->signal_id);
		pi->priv->signal_id = 0;
	}
	if (pi->priv->xplayer) {
		g_object_unref (pi->priv->xplayer);
		pi->priv->xplayer = NULL;
	}
	if (pi->priv->settings) {
		g_object_unref (pi->priv->settings);
		pi->priv->settings = NULL;
	}
}
