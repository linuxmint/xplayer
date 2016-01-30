/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001-2007 Bastien Nocera <hadess@hadess.net>
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

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <string.h>

#ifdef GDK_WINDOWING_X11
/* X11 headers */
#include <X11/Xlib.h>
#endif

#include "xplayer.h"
#include "xplayer-private.h"
#include "xplayer-interface.h"
#include "xplayer-options.h"
#include "xplayer-menu.h"
#include "xplayer-session.h"
#include "xplayer-uri.h"
#include "xplayer-preferences.h"
#include "xplayer-rtl-helpers.h"
#include "xplayer-sidebar.h"
#include "video-utils.h"

static gboolean startup_called = FALSE;

/* Debug log message handler: discards debug messages unless Xplayer is run with XPLAYER_DEBUG=1.
 * If we're building in the source tree, enable debug messages by default. */
static void
debug_handler (const char *log_domain,
               GLogLevelFlags log_level,
               const char *message,
               GSettings *settings)
{
	static int debug = -1;

	if (debug < 0)
		debug = g_settings_get_boolean (settings, "debug");

	if (debug)
		g_log_default_handler (log_domain, log_level, message, NULL);
}

static void
set_rtl_icon_name (Xplayer *xplayer,
		   const char *widget_name,
		   const char *icon_name)
{
	GtkAction *action;

	action = GTK_ACTION (gtk_builder_get_object (xplayer->xml, widget_name));
	gtk_action_set_icon_name (action, xplayer_get_rtl_icon_name (icon_name));
}

static void
app_init (Xplayer *xplayer, char **argv)
{
	GtkSettings *gtk_settings;
	char *sidebar_pageid;

	if (gtk_clutter_init (NULL, NULL) != CLUTTER_INIT_SUCCESS)
		g_warning ("gtk-clutter failed to initialise, expect problems from here on.");

	gtk_settings = gtk_settings_get_default ();
	g_object_set (G_OBJECT (gtk_settings), "gtk-application-prefer-dark-theme", TRUE, NULL);

	/* Debug log handling */
	g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, (GLogFunc) debug_handler, xplayer->settings);

	/* Main window */
	xplayer->xml = xplayer_interface_load ("xplayer.ui", TRUE, NULL, xplayer);
	if (xplayer->xml == NULL)
		xplayer_action_exit (NULL);

	set_rtl_icon_name (xplayer, "play", "media-playback-start");
	set_rtl_icon_name (xplayer, "next-chapter", "media-skip-forward");
	set_rtl_icon_name (xplayer, "previous-chapter", "media-skip-backward");
	set_rtl_icon_name (xplayer, "skip-forward", "media-seek-forward");
	set_rtl_icon_name (xplayer, "skip-backwards", "media-seek-backward");

	xplayer->win = GTK_WIDGET (gtk_builder_get_object (xplayer->xml, "xplayer_main_window"));

	/* Menubar */
	xplayer_ui_manager_setup (xplayer);

	/* The sidebar */
	playlist_widget_setup (xplayer);

	/* The rest of the widgets */
	xplayer->state = STATE_STOPPED;
	xplayer->seek = GTK_WIDGET (gtk_builder_get_object (xplayer->xml, "tmw_seek_hscale"));
	xplayer->seekadj = gtk_range_get_adjustment (GTK_RANGE (xplayer->seek));
	xplayer->volume = GTK_WIDGET (gtk_builder_get_object (xplayer->xml, "tmw_volume_button"));
	xplayer->statusbar = GTK_WIDGET (gtk_builder_get_object (xplayer->xml, "tmw_statusbar"));
	xplayer->seek_lock = FALSE;
	xplayer->fs = xplayer_fullscreen_new (GTK_WINDOW (xplayer->win));
	gtk_scale_button_set_adjustment (GTK_SCALE_BUTTON (xplayer->fs->volume),
					 gtk_scale_button_get_adjustment (GTK_SCALE_BUTTON (xplayer->volume)));
	gtk_range_set_adjustment (GTK_RANGE (xplayer->fs->seek), xplayer->seekadj);

	xplayer_session_setup (xplayer, argv);
	xplayer_setup_file_monitoring (xplayer);
	xplayer_setup_file_filters ();
	xplayer_callback_connect (xplayer);

	sidebar_pageid = xplayer_setup_window (xplayer);

	/* Show ! */
	if (optionstate.fullscreen == FALSE) {
		gtk_widget_show (xplayer->win);
		xplayer_gdk_window_set_waiting_cursor (gtk_widget_get_window (xplayer->win));
	} else {
		gtk_widget_realize (xplayer->win);
	}

	xplayer->controls_visibility = XPLAYER_CONTROLS_UNDEFINED;

	/* Show ! (again) the video widget this time. */
	video_widget_create (xplayer);
	gtk_widget_grab_focus (GTK_WIDGET (xplayer->bvw));
	xplayer_fullscreen_set_video_widget (xplayer->fs, xplayer->bvw);

	if (optionstate.fullscreen != FALSE) {
		gtk_widget_show (xplayer->win);
		gdk_flush ();
		xplayer_action_fullscreen (xplayer, TRUE);
	}

	/* The prefs after the video widget is connected */
	xplayer->prefs_xml = xplayer_interface_load ("preferences.ui", TRUE, NULL, xplayer);

	xplayer_setup_preferences (xplayer);

	xplayer_setup_recent (xplayer);

	/* Command-line handling */
	xplayer_options_process_late (xplayer, &optionstate);

	/* Initialise all the plugins, and set the default page, in case
	 * it comes from a plugin */
	xplayer_object_plugins_init (xplayer);
	xplayer_sidebar_set_current_page (xplayer, sidebar_pageid, FALSE);
	g_free (sidebar_pageid);

	if (xplayer->session_restored != FALSE) {
		xplayer_session_restore (xplayer, optionstate.filenames);
	} else if (optionstate.filenames != NULL && xplayer_action_open_files (xplayer, optionstate.filenames)) {
		xplayer_action_play_pause (xplayer);
	} else {
		xplayer_action_set_mrl (xplayer, NULL, NULL);
	}

	if (optionstate.fullscreen == FALSE)
		gdk_window_set_cursor (gtk_widget_get_window (xplayer->win), NULL);

	gtk_window_set_application (GTK_WINDOW (xplayer->win), GTK_APPLICATION (xplayer));
}

static void
app_startup (GApplication *application,
		Xplayer        *xplayer)
{
	/* We don't do anything here, as we need to know the options
	 * when we set everything up.
	 * Note that this will break D-Bus activation of the application */
	startup_called = TRUE;
}

static int
app_command_line (GApplication             *app,
		  GApplicationCommandLine  *command_line,
		  Xplayer                    *xplayer)
{
	GOptionContext *context;
	int argc;
	char **argv;

	argv = g_application_command_line_get_arguments (command_line, &argc);

	/* Reset the options, if they were used before */
	memset (&optionstate, 0, sizeof (optionstate));

	/* Options parsing */
	context = xplayer_options_get_context ();
	g_option_context_set_help_enabled (context, FALSE);
	if (g_option_context_parse (context, &argc, &argv, NULL) == FALSE) {
	        g_option_context_free (context);
	        return 1;
	}
	g_option_context_free (context);

	xplayer_options_process_early (xplayer, &optionstate);

	/* Don't create another window if we're remote.
	 * We can't use g_application_get_is_remote() because it's not registered yet */
	if (startup_called != FALSE) {
		app_init (xplayer, argv);

		gdk_notify_startup_complete ();

		/* Don't add files again through xplayer_options_process_for_server() */
		g_strfreev (optionstate.filenames);
		optionstate.filenames = NULL;
		startup_called = FALSE;
	}

	/* Now do something with it */
	xplayer_options_process_for_server (xplayer, &optionstate);

	g_strfreev (argv);
	return 0;
}

int
main (int argc, char **argv)
{
	Xplayer *xplayer;

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

#ifdef GDK_WINDOWING_X11
	if (XInitThreads () == 0)
	{
		gtk_init (&argc, &argv);
		g_set_application_name (_("Videos"));
		xplayer_action_error_and_exit (_("Could not initialize the thread-safe libraries."), _("Verify your system installation. Xplayer will now exit."), NULL);
	}
#endif

	g_type_init ();

	g_set_prgname ("xplayer");
	g_set_application_name (_("Videos"));
	gtk_window_set_default_icon_name ("xplayer");
	g_setenv("PULSE_PROP_media.role", "video", TRUE);


	/* Build the main Xplayer object */
	xplayer = g_object_new (XPLAYER_TYPE_OBJECT,
			      "application-id", "org.gnome.Xplayer",
			      "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
			      NULL);
	xplayer->settings = g_settings_new (XPLAYER_GSETTINGS_SCHEMA);

	g_signal_connect (G_OBJECT (xplayer), "startup",
			  G_CALLBACK (app_startup), xplayer);
	g_signal_connect (G_OBJECT (xplayer), "command-line",
			  G_CALLBACK (app_command_line), xplayer);

	g_application_run (G_APPLICATION (xplayer), argc, argv);

	return 0;
}
