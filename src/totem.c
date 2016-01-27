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

#include "totem.h"
#include "totem-private.h"
#include "totem-interface.h"
#include "totem-options.h"
#include "totem-menu.h"
#include "totem-session.h"
#include "totem-uri.h"
#include "totem-preferences.h"
#include "totem-rtl-helpers.h"
#include "totem-sidebar.h"
#include "video-utils.h"

static gboolean startup_called = FALSE;

/* Debug log message handler: discards debug messages unless Totem is run with TOTEM_DEBUG=1.
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
set_rtl_icon_name (Totem *totem,
		   const char *widget_name,
		   const char *icon_name)
{
	GtkAction *action;

	action = GTK_ACTION (gtk_builder_get_object (totem->xml, widget_name));
	gtk_action_set_icon_name (action, totem_get_rtl_icon_name (icon_name));
}

static void
app_init (Totem *totem, char **argv)
{
	GtkSettings *gtk_settings;
	char *sidebar_pageid;

	if (gtk_clutter_init (NULL, NULL) != CLUTTER_INIT_SUCCESS)
		g_warning ("gtk-clutter failed to initialise, expect problems from here on.");

	gtk_settings = gtk_settings_get_default ();
	g_object_set (G_OBJECT (gtk_settings), "gtk-application-prefer-dark-theme", TRUE, NULL);

	/* Debug log handling */
	g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, (GLogFunc) debug_handler, totem->settings);

	/* Main window */
	totem->xml = totem_interface_load ("totem.ui", TRUE, NULL, totem);
	if (totem->xml == NULL)
		totem_action_exit (NULL);

	set_rtl_icon_name (totem, "play", "media-playback-start");
	set_rtl_icon_name (totem, "next-chapter", "media-skip-forward");
	set_rtl_icon_name (totem, "previous-chapter", "media-skip-backward");
	set_rtl_icon_name (totem, "skip-forward", "media-seek-forward");
	set_rtl_icon_name (totem, "skip-backwards", "media-seek-backward");

	totem->win = GTK_WIDGET (gtk_builder_get_object (totem->xml, "totem_main_window"));

	/* Menubar */
	totem_ui_manager_setup (totem);

	/* The sidebar */
	playlist_widget_setup (totem);

	/* The rest of the widgets */
	totem->state = STATE_STOPPED;
	totem->seek = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tmw_seek_hscale"));
	totem->seekadj = gtk_range_get_adjustment (GTK_RANGE (totem->seek));
	totem->volume = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tmw_volume_button"));
	totem->statusbar = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tmw_statusbar"));
	totem->seek_lock = FALSE;
	totem->fs = totem_fullscreen_new (GTK_WINDOW (totem->win));
	gtk_scale_button_set_adjustment (GTK_SCALE_BUTTON (totem->fs->volume),
					 gtk_scale_button_get_adjustment (GTK_SCALE_BUTTON (totem->volume)));
	gtk_range_set_adjustment (GTK_RANGE (totem->fs->seek), totem->seekadj);

	totem_session_setup (totem, argv);
	totem_setup_file_monitoring (totem);
	totem_setup_file_filters ();
	totem_callback_connect (totem);

	sidebar_pageid = totem_setup_window (totem);

	/* Show ! */
	if (optionstate.fullscreen == FALSE) {
		gtk_widget_show (totem->win);
		totem_gdk_window_set_waiting_cursor (gtk_widget_get_window (totem->win));
	} else {
		gtk_widget_realize (totem->win);
	}

	totem->controls_visibility = TOTEM_CONTROLS_UNDEFINED;

	/* Show ! (again) the video widget this time. */
	video_widget_create (totem);
	gtk_widget_grab_focus (GTK_WIDGET (totem->bvw));
	totem_fullscreen_set_video_widget (totem->fs, totem->bvw);

	if (optionstate.fullscreen != FALSE) {
		gtk_widget_show (totem->win);
		gdk_flush ();
		totem_action_fullscreen (totem, TRUE);
	}

	/* The prefs after the video widget is connected */
	totem->prefs_xml = totem_interface_load ("preferences.ui", TRUE, NULL, totem);

	totem_setup_preferences (totem);

	totem_setup_recent (totem);

	/* Command-line handling */
	totem_options_process_late (totem, &optionstate);

	/* Initialise all the plugins, and set the default page, in case
	 * it comes from a plugin */
	totem_object_plugins_init (totem);
	totem_sidebar_set_current_page (totem, sidebar_pageid, FALSE);
	g_free (sidebar_pageid);

	if (totem->session_restored != FALSE) {
		totem_session_restore (totem, optionstate.filenames);
	} else if (optionstate.filenames != NULL && totem_action_open_files (totem, optionstate.filenames)) {
		totem_action_play_pause (totem);
	} else {
		totem_action_set_mrl (totem, NULL, NULL);
	}

	/* Set the logo at the last minute so we won't try to show it before a video */
	bacon_video_widget_set_logo (totem->bvw, "totem");

	if (optionstate.fullscreen == FALSE)
		gdk_window_set_cursor (gtk_widget_get_window (totem->win), NULL);

	gtk_window_set_application (GTK_WINDOW (totem->win), GTK_APPLICATION (totem));
}

static void
app_startup (GApplication *application,
		Totem        *totem)
{
	/* We don't do anything here, as we need to know the options
	 * when we set everything up.
	 * Note that this will break D-Bus activation of the application */
	startup_called = TRUE;
}

static int
app_command_line (GApplication             *app,
		  GApplicationCommandLine  *command_line,
		  Totem                    *totem)
{
	GOptionContext *context;
	int argc;
	char **argv;

	argv = g_application_command_line_get_arguments (command_line, &argc);

	/* Reset the options, if they were used before */
	memset (&optionstate, 0, sizeof (optionstate));

	/* Options parsing */
	context = totem_options_get_context ();
	g_option_context_set_help_enabled (context, FALSE);
	if (g_option_context_parse (context, &argc, &argv, NULL) == FALSE) {
	        g_option_context_free (context);
	        return 1;
	}
	g_option_context_free (context);

	totem_options_process_early (totem, &optionstate);

	/* Don't create another window if we're remote.
	 * We can't use g_application_get_is_remote() because it's not registered yet */
	if (startup_called != FALSE) {
		app_init (totem, argv);

		gdk_notify_startup_complete ();

		/* Don't add files again through totem_options_process_for_server() */
		g_strfreev (optionstate.filenames);
		optionstate.filenames = NULL;
		startup_called = FALSE;
	}

	/* Now do something with it */
	totem_options_process_for_server (totem, &optionstate);

	g_strfreev (argv);
	return 0;
}

int
main (int argc, char **argv)
{
	Totem *totem;

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

#ifdef GDK_WINDOWING_X11
	if (XInitThreads () == 0)
	{
		gtk_init (&argc, &argv);
		g_set_application_name (_("Videos"));
		totem_action_error_and_exit (_("Could not initialize the thread-safe libraries."), _("Verify your system installation. Totem will now exit."), NULL);
	}
#endif

	g_type_init ();

	g_set_prgname ("totem");
	g_set_application_name (_("Videos"));
	gtk_window_set_default_icon_name ("totem");
	g_setenv("PULSE_PROP_media.role", "video", TRUE);


	/* Build the main Totem object */
	totem = g_object_new (TOTEM_TYPE_OBJECT,
			      "application-id", "org.gnome.Totem",
			      "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
			      NULL);
	totem->settings = g_settings_new (TOTEM_GSETTINGS_SCHEMA);

	g_signal_connect (G_OBJECT (totem), "startup",
			  G_CALLBACK (app_startup), totem);
	g_signal_connect (G_OBJECT (totem), "command-line",
			  G_CALLBACK (app_command_line), totem);

	g_application_run (G_APPLICATION (totem), argc, argv);

	return 0;
}
