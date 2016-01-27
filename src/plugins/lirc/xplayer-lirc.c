/* 
 *  Copyright (C) 2002 James Willcox  <jwillcox@gnome.org>
 *            (C) 2007 Jan Arne Petersen <jpetersen@jpetersen.org>
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


#include <config.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>
#include <libpeas/peas-activatable.h>
#include <unistd.h>
#include <lirc/lirc_client.h>

#include "xplayer-plugin.h"
#include "xplayer.h"
#include "xplayer-dirs.h"

#define XPLAYER_TYPE_LIRC_PLUGIN		(xplayer_lirc_plugin_get_type ())
#define XPLAYER_LIRC_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), XPLAYER_TYPE_LIRC_PLUGIN, XplayerLircPlugin))
#define XPLAYER_LIRC_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), XPLAYER_TYPE_LIRC_PLUGIN, XplayerLircPluginClass))
#define XPLAYER_IS_LIRC_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), XPLAYER_TYPE_LIRC_PLUGIN))
#define XPLAYER_IS_LIRC_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), XPLAYER_TYPE_LIRC_PLUGIN))
#define XPLAYER_LIRC_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), XPLAYER_TYPE_LIRC_PLUGIN, XplayerLircPluginClass))

typedef struct {
	GIOChannel *lirc_channel;
	struct lirc_config *lirc_config;

	XplayerObject *xplayer;
} XplayerLircPluginPrivate;

/* strings that we recognize as commands from lirc */
#define XPLAYER_IR_COMMAND_PLAY "play"
#define XPLAYER_IR_COMMAND_PAUSE "pause"
#define XPLAYER_IR_COMMAND_STOP "stop"
#define XPLAYER_IR_COMMAND_NEXT "next"
#define XPLAYER_IR_COMMAND_PREVIOUS "previous"
#define XPLAYER_IR_COMMAND_SEEK_FORWARD "seek_forward"
#define XPLAYER_IR_COMMAND_SEEK_BACKWARD "seek_backward"
#define XPLAYER_IR_COMMAND_VOLUME_UP "volume_up"
#define XPLAYER_IR_COMMAND_VOLUME_DOWN "volume_down"
#define XPLAYER_IR_COMMAND_FULLSCREEN "fullscreen"
#define XPLAYER_IR_COMMAND_QUIT "quit"
#define XPLAYER_IR_COMMAND_UP "up"
#define XPLAYER_IR_COMMAND_DOWN "down"
#define XPLAYER_IR_COMMAND_LEFT "left"
#define XPLAYER_IR_COMMAND_RIGHT "right"
#define XPLAYER_IR_COMMAND_SELECT "select"
#define XPLAYER_IR_COMMAND_MENU "menu"
#define XPLAYER_IR_COMMAND_PLAYPAUSE "play_pause"
#define XPLAYER_IR_COMMAND_ZOOM_UP "zoom_up"
#define XPLAYER_IR_COMMAND_ZOOM_DOWN "zoom_down"
#define XPLAYER_IR_COMMAND_EJECT "eject"
#define XPLAYER_IR_COMMAND_PLAY_DVD "play_dvd"
#define XPLAYER_IR_COMMAND_MUTE "mute"
#define XPLAYER_IR_COMMAND_TOGGLE_ASPECT "toggle_aspect"

#define XPLAYER_IR_SETTING "setting_"
#define XPLAYER_IR_SETTING_TOGGLE_REPEAT "setting_repeat"
#define XPLAYER_IR_SETTING_TOGGLE_SHUFFLE "setting_shuffle"

XPLAYER_PLUGIN_REGISTER(XPLAYER_TYPE_LIRC_PLUGIN, XplayerLircPlugin, xplayer_lirc_plugin)

static char *
xplayer_lirc_get_url (const char *str)
{
	char *s;

	if (str == NULL)
		return NULL;
	s = strchr (str, ':');
	if (s == NULL)
		return NULL;
	return g_strdup (s + 1);
}

static gint
xplayer_lirc_to_setting (const gchar *str, char **url)
{
	if (strcmp (str, XPLAYER_IR_SETTING_TOGGLE_REPEAT) == 0)
		return XPLAYER_REMOTE_SETTING_REPEAT;
	else if (strcmp (str, XPLAYER_IR_SETTING_TOGGLE_SHUFFLE) == 0)
		return XPLAYER_REMOTE_SETTING_SHUFFLE;
	else
		return -1;
}

static XplayerRemoteCommand
xplayer_lirc_to_command (const gchar *str, char **url)
{
	if (strcmp (str, XPLAYER_IR_COMMAND_PLAY) == 0)
		return XPLAYER_REMOTE_COMMAND_PLAY;
	else if (strcmp (str, XPLAYER_IR_COMMAND_PAUSE) == 0)
		return XPLAYER_REMOTE_COMMAND_PAUSE;
	else if (strcmp (str, XPLAYER_IR_COMMAND_PLAYPAUSE) == 0)
		return XPLAYER_REMOTE_COMMAND_PLAYPAUSE;
	else if (strcmp (str, XPLAYER_IR_COMMAND_STOP) == 0)
		return XPLAYER_REMOTE_COMMAND_STOP;
	else if (strcmp (str, XPLAYER_IR_COMMAND_NEXT) == 0)
		return XPLAYER_REMOTE_COMMAND_NEXT;
	else if (strcmp (str, XPLAYER_IR_COMMAND_PREVIOUS) == 0)
		return XPLAYER_REMOTE_COMMAND_PREVIOUS;
	else if (g_str_has_prefix (str, XPLAYER_IR_COMMAND_SEEK_FORWARD) != FALSE) {
		*url = xplayer_lirc_get_url (str);
		return XPLAYER_REMOTE_COMMAND_SEEK_FORWARD;
	} else if (g_str_has_prefix (str, XPLAYER_IR_COMMAND_SEEK_BACKWARD) != FALSE) {
		*url = xplayer_lirc_get_url (str);
		return XPLAYER_REMOTE_COMMAND_SEEK_BACKWARD;
	} else if (strcmp (str, XPLAYER_IR_COMMAND_VOLUME_UP) == 0)
		return XPLAYER_REMOTE_COMMAND_VOLUME_UP;
	else if (strcmp (str, XPLAYER_IR_COMMAND_VOLUME_DOWN) == 0)
		return XPLAYER_REMOTE_COMMAND_VOLUME_DOWN;
	else if (strcmp (str, XPLAYER_IR_COMMAND_FULLSCREEN) == 0)
		return XPLAYER_REMOTE_COMMAND_FULLSCREEN;
	else if (strcmp (str, XPLAYER_IR_COMMAND_QUIT) == 0)
		return XPLAYER_REMOTE_COMMAND_QUIT;
	else if (strcmp (str, XPLAYER_IR_COMMAND_UP) == 0)
		return XPLAYER_REMOTE_COMMAND_UP;
	else if (strcmp (str, XPLAYER_IR_COMMAND_DOWN) == 0)
		return XPLAYER_REMOTE_COMMAND_DOWN;
	else if (strcmp (str, XPLAYER_IR_COMMAND_LEFT) == 0)
		return XPLAYER_REMOTE_COMMAND_LEFT;
	else if (strcmp (str, XPLAYER_IR_COMMAND_RIGHT) == 0)
		return XPLAYER_REMOTE_COMMAND_RIGHT;
	else if (strcmp (str, XPLAYER_IR_COMMAND_SELECT) == 0)
		return XPLAYER_REMOTE_COMMAND_SELECT;
	else if (strcmp (str, XPLAYER_IR_COMMAND_MENU) == 0)
		return XPLAYER_REMOTE_COMMAND_DVD_MENU;
	else if (strcmp (str, XPLAYER_IR_COMMAND_ZOOM_UP) == 0)
		return XPLAYER_REMOTE_COMMAND_ZOOM_UP;
	else if (strcmp (str, XPLAYER_IR_COMMAND_ZOOM_DOWN) == 0)
		return XPLAYER_REMOTE_COMMAND_ZOOM_DOWN;
	else if (strcmp (str, XPLAYER_IR_COMMAND_EJECT) == 0)
		return XPLAYER_REMOTE_COMMAND_EJECT;
	else if (strcmp (str, XPLAYER_IR_COMMAND_PLAY_DVD) == 0)
		return XPLAYER_REMOTE_COMMAND_PLAY_DVD;
	else if (strcmp (str, XPLAYER_IR_COMMAND_MUTE) == 0)
		return XPLAYER_REMOTE_COMMAND_MUTE;
	else if (strcmp (str, XPLAYER_IR_COMMAND_TOGGLE_ASPECT) == 0)
		return XPLAYER_REMOTE_COMMAND_TOGGLE_ASPECT;
	else
		return XPLAYER_REMOTE_COMMAND_UNKNOWN;
}

static gboolean
xplayer_lirc_read_code (GIOChannel *source, GIOCondition condition, XplayerLircPlugin *pi)
{
	char *code;
	char *str = NULL, *url = NULL;
	int ok;
	XplayerRemoteCommand cmd;

	if (condition & (G_IO_ERR | G_IO_HUP)) {
		/* LIRC connection broken. */
		return FALSE;
	}

	/* this _could_ block, but it shouldn't */
	lirc_nextcode (&code);

	if (code == NULL) {
		/* the code was incomplete or something */
		return TRUE;
	}

	do {
		ok = lirc_code2char (pi->priv->lirc_config, code, &str);

		if (ok != 0) {
			/* Couldn't convert lirc code to string. */
			break;
		}

		if (str == NULL) {
			/* there was no command associated with the code */
			break;
		}

		if (g_str_has_prefix (str, XPLAYER_IR_SETTING) != FALSE) {
			gint setting = xplayer_lirc_to_setting (str, &url);
			if (setting >= 0) {
				gboolean value;

				value = xplayer_action_remote_get_setting (pi->priv->xplayer, setting);
				xplayer_action_remote_set_setting (pi->priv->xplayer, setting, !value);
			}
		} else {
			cmd = xplayer_lirc_to_command (str, &url);
			xplayer_action_remote (pi->priv->xplayer, cmd, url);
		}
		g_free (url);
	} while (TRUE);

	g_free (code);

	return TRUE;
}

static void
impl_activate (PeasActivatable *plugin)
{
	XplayerLircPlugin *pi = XPLAYER_LIRC_PLUGIN (plugin);
	char *path;
	int fd;

	pi->priv->xplayer = g_object_ref (g_object_get_data (G_OBJECT (plugin), "object"));

	fd = lirc_init ((char*) "Xplayer", 0);
	if (fd < 0) {
		//FIXME
#if 0
		g_set_error_literal (error, XPLAYER_PLUGIN_ERROR, XPLAYER_PLUGIN_ERROR_ACTIVATION,
                                     _("Couldn't initialize lirc."));
		return FALSE;
#endif
	}

	/* Load the default Xplayer setup */
	path = xplayer_plugin_find_file ("lirc", "xplayer_lirc_default");
	if (path == NULL || lirc_readconfig (path, &pi->priv->lirc_config, NULL) == -1) {
		g_free (path);
		//FIXME
#if 0
		g_set_error_literal (error, XPLAYER_PLUGIN_ERROR, XPLAYER_PLUGIN_ERROR_ACTIVATION,
                                     _("Couldn't read lirc configuration."));
#endif
		close (fd);
		return;
	}
	g_free (path);

	/* Load the user config, doesn't matter if it's not there */
	lirc_readconfig (NULL, &pi->priv->lirc_config, NULL);

	pi->priv->lirc_channel = g_io_channel_unix_new (fd);
	g_io_channel_set_encoding (pi->priv->lirc_channel, NULL, NULL);
	g_io_channel_set_buffered (pi->priv->lirc_channel, FALSE);
	g_io_add_watch (pi->priv->lirc_channel, G_IO_IN | G_IO_ERR | G_IO_HUP,
			(GIOFunc) xplayer_lirc_read_code, pi);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	XplayerLircPlugin *pi = XPLAYER_LIRC_PLUGIN (plugin);
	GError *error = NULL;

	if (pi->priv->lirc_channel) {
		g_io_channel_shutdown (pi->priv->lirc_channel, FALSE, &error);
		if (error != NULL) {
			g_warning ("Couldn't destroy lirc connection: %s",
				   error->message);
			g_error_free (error);
		}
		pi->priv->lirc_channel = NULL;
	}

	if (pi->priv->lirc_config) {
		lirc_freeconfig (pi->priv->lirc_config);
		pi->priv->lirc_config = NULL;

		lirc_deinit ();
	}

	if (pi->priv->xplayer) {
		g_object_unref (pi->priv->xplayer);
		pi->priv->xplayer = NULL;
	}
}

