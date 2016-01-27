/* xplayer-session.c

   Copyright (C) 2004 Bastien Nocera

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Author: Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"

#include "xplayer.h"
#include "xplayer-private.h"
#include "xplayer-session.h"
#include "xplayer-uri.h"

#ifdef WITH_SMCLIENT

#include <unistd.h>

#include "eggsmclient.h"

#ifdef GDK_WINDOWING_X11
#include "eggdesktopfile.h"
#endif

static char *
xplayer_session_create_key (void)
{
	char *filename, *path;

	filename = g_strdup_printf ("playlist-%d-%d-%u.pls",
			(int) getpid (),
			(int) time (NULL),
			g_random_int ());
	path = g_build_filename (xplayer_dot_dir (), filename, NULL);
	g_free (filename);

	return path;
}

static void
xplayer_save_state_cb (EggSMClient *client,
	             GKeyFile *key_file,
	             Xplayer *xplayer)
{
	const char *argv[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	int i = 0;
	char *path_id, *current, *seek, *uri;
	int current_index;

	current_index = xplayer_playlist_get_current (xplayer->playlist);

	if (current_index == -1)
		return;

	path_id = xplayer_session_create_key ();
	xplayer_playlist_save_current_playlist (xplayer->playlist, path_id);

	/* How to discard the save */
	argv[i++] = "rm";
	argv[i++] = "-f";
	argv[i++] = path_id;
	egg_sm_client_set_discard_command (client, i, (const char **) argv);

	/* How to clone or restart */
	i = 0;
	current = g_strdup_printf ("%d", current_index);
	seek = g_strdup_printf ("%"G_GINT64_FORMAT,
			bacon_video_widget_get_current_time (xplayer->bvw));
	argv[i++] = xplayer->argv0;
	argv[i++] = "--playlist-idx";
	argv[i++] = current;
	argv[i++] = "--seek";
	argv[i++] = seek;

	uri = g_filename_to_uri (path_id, NULL, NULL);
	argv[i++] = uri;

	/* FIXME: what's this used for? */
	/* egg_sm_client_set_clone_command (client, i, argv); */

	egg_sm_client_set_restart_command (client, i, (const char **) argv);

	g_free (path_id);
	g_free (current);
	g_free (seek);
	g_free (uri);
}

G_GNUC_NORETURN static void
xplayer_quit_cb (EggSMClient *client,
	       Xplayer *xplayer)
{
	xplayer_action_exit (xplayer);
}

void
xplayer_session_add_options (GOptionContext *context)
{
#ifdef GDK_WINDOWING_X11
	egg_set_desktop_file_without_defaults (DATADIR "/applications/" PACKAGE ".desktop");
#endif

	g_option_context_add_group (context, egg_sm_client_get_option_group ());
}

void
xplayer_session_setup (Xplayer *xplayer, char **argv)
{
	EggSMClient *sm_client;

	xplayer->argv0 = argv[0];

	sm_client = egg_sm_client_get ();
	g_signal_connect (sm_client, "save-state",
	                  G_CALLBACK (xplayer_save_state_cb), xplayer);
	g_signal_connect (sm_client, "quit",
	                  G_CALLBACK (xplayer_quit_cb), xplayer);

	if (egg_sm_client_is_resumed (sm_client))
		xplayer->session_restored = TRUE;
}

void
xplayer_session_restore (Xplayer *xplayer, char **filenames)
{
	char *mrl, *uri, *subtitle;

	g_return_if_fail (filenames != NULL);
	g_return_if_fail (filenames[0] != NULL);
	uri = filenames[0];
	subtitle = NULL;

	xplayer_signal_block_by_data (xplayer->playlist, xplayer);

	/* Possibly the only place in Xplayer where it makes sense to add an MRL to the playlist synchronously, since we haven't yet entered
	 * the GTK+ main loop, and thus can't freeze the application. */
	if (xplayer_playlist_add_mrl_sync (xplayer->playlist, uri, NULL) == FALSE) {
		xplayer_signal_unblock_by_data (xplayer->playlist, xplayer);
		xplayer_action_set_mrl (xplayer, NULL, NULL);
		g_free (uri);
		return;
	}

	xplayer_signal_unblock_by_data (xplayer->playlist, xplayer);

	if (xplayer->index != 0)
		xplayer_playlist_set_current (xplayer->playlist, xplayer->index);
	mrl = xplayer_playlist_get_current_mrl (xplayer->playlist, &subtitle);

	xplayer_action_set_mrl_with_warning (xplayer, mrl, subtitle, FALSE);

	/* We do the seeking after being told that the stream is seekable,
	 * not straight away */

	g_free (mrl);
	g_free (subtitle);
}

#else

void
xplayer_session_add_options (GOptionContext *context)
{
}

void
xplayer_session_setup (Xplayer *xplayer, char **argv)
{
}

void
xplayer_session_restore (Xplayer *xplayer, char **argv)
{
}

#endif /* WITH_SMCLIENT */
