/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) Philippe Rouquier 2008 <bonfire-app@wanadoo.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include "config.h"

#include <unistd.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>
#include <gdk/gdkx.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>
#include <libpeas/peas-activatable.h>

#include <libxml/xmlerror.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
#include <libxml/xmlsave.h>

#include "totem-plugin.h"
#include "totem-interface.h"

#define TOTEM_TYPE_DISC_RECORDER_PLUGIN		(totem_disc_recorder_plugin_get_type ())
#define TOTEM_DISC_RECORDER_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_DISC_RECORDER_PLUGIN, TotemDiscRecorderPlugin))
#define TOTEM_DISC_RECORDER_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_DISC_RECORDER_PLUGIN, TotemDiscRecorderPluginClass))
#define TOTEM_IS_DISC_RECORDER_PLUGIN(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_DISC_RECORDER_PLUGIN))
#define TOTEM_IS_DISC_RECORDER_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_DISC_RECORDER_PLUGIN))
#define TOTEM_DISC_RECORDER_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_DISC_RECORDER_PLUGIN, TotemDiscRecorderPluginClass))

typedef struct {
	TotemObject *totem;

	GtkActionGroup *action_group;
	guint ui_merge_id;
} TotemDiscRecorderPluginPrivate;

TOTEM_PLUGIN_REGISTER(TOTEM_TYPE_DISC_RECORDER_PLUGIN, TotemDiscRecorderPlugin, totem_disc_recorder_plugin)

static void totem_disc_recorder_plugin_burn (GtkAction *action,
					     TotemDiscRecorderPlugin *pi);
static void totem_disc_recorder_plugin_copy (GtkAction *action,
					     TotemDiscRecorderPlugin *pi);

static GtkActionEntry totem_disc_recorder_plugin_actions [] = {
	{ "VideoBurnToDisc", "media-optical-video-new", N_("_Create Video Disc..."), NULL,
		N_("Create a video DVD or a (S)VCD from the currently opened movie"),
		G_CALLBACK (totem_disc_recorder_plugin_burn) },
	{ "VideoDVDCopy", "media-optical-copy", N_("Copy Vide_o DVD..."), NULL,
		N_("Copy the currently playing video DVD"),
		G_CALLBACK (totem_disc_recorder_plugin_copy) },
	{ "VideoVCDCopy", "media-optical-copy", N_("Copy (S)VCD..."), NULL,
		N_("Copy the currently playing (S)VCD"),
		G_CALLBACK (totem_disc_recorder_plugin_copy) },
};

static gboolean
totem_disc_recorder_plugin_start_burning (TotemDiscRecorderPlugin *pi,
					  const char *path,
					  gboolean copy)
{
	GtkWindow *main_window;
	GdkScreen *screen;
	GdkDisplay *display;
	gchar *command_line;
	GList *uris;
	GAppInfo *info;
	GdkAppLaunchContext *context;
	GError *error = NULL;
	char *xid_arg;

	main_window = totem_get_main_window (pi->priv->totem);
	screen = gtk_widget_get_screen (GTK_WIDGET (main_window));
	display = gdk_display_get_default ();

	/* Build a command line to use */
	xid_arg = NULL;
#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY (display))
		xid_arg = g_strdup_printf ("-x %d", (int) gdk_x11_window_get_xid (gtk_widget_get_window (GTK_WIDGET (main_window))));
#endif /* GDK_WINDOWING_X11 */
	g_object_unref (main_window);

	if (copy != FALSE)
		command_line = g_strdup_printf ("brasero %s -c", xid_arg ? xid_arg : "");
	else
		command_line = g_strdup_printf ("brasero %s -r", xid_arg ? xid_arg : "");

	/* Build the app info */
	info = g_app_info_create_from_commandline (command_line, NULL,
	                                           G_APP_INFO_CREATE_SUPPORTS_URIS | G_APP_INFO_CREATE_SUPPORTS_STARTUP_NOTIFICATION, &error);
	g_free (command_line);

	if (error != NULL)
		goto error;

	/* Create a launch context and launch it */
	context = gdk_display_get_app_launch_context (gtk_widget_get_display (GTK_WIDGET (main_window)));
	gdk_app_launch_context_set_screen (context, screen);

	uris = g_list_prepend (NULL, (gpointer) path);
	g_app_info_launch_uris (info, uris, G_APP_LAUNCH_CONTEXT (context), &error);
	g_list_free (uris);

	g_object_unref (info);
	g_object_unref (context);

	if (error != NULL)
		goto error;

	return TRUE;

error:
	main_window = totem_get_main_window (pi->priv->totem);

	if (copy != FALSE)
		totem_interface_error (_("The video disc could not be duplicated."), error->message, main_window);
	else
		totem_interface_error (_("The movie could not be recorded."), error->message, main_window);

	g_error_free (error);
	g_object_unref (main_window);

	return FALSE;
}

static char*
totem_disc_recorder_plugin_write_video_project (TotemDiscRecorderPlugin *pi,
						char **error)
{
	xmlTextWriter *project;
	xmlDocPtr doc = NULL;
	xmlSaveCtxt *save;
	xmlChar *escaped;
	gint success;
	char *title, *path, *uri;
	int fd;

	/* get a temporary path */
	path = g_build_filename (g_get_tmp_dir (), "brasero-tmp-project-XXXXXX",  NULL);
	fd = g_mkstemp (path);
	if (!fd) {
		g_free (path);

		*error = g_strdup (_("Unable to write a project."));
		return NULL;
	}

	project = xmlNewTextWriterDoc (&doc, 0);
	if (!project) {
		g_remove (path);
		g_free (path);
		close (fd);

		*error = g_strdup (_("Unable to write a project."));
		return NULL;
	}

	xmlTextWriterSetIndent (project, 1);
	xmlTextWriterSetIndentString (project, (xmlChar *) "\t");

	success = xmlTextWriterStartDocument (project,
					      NULL,
					      "UTF8",
					      NULL);
	if (success < 0)
		goto error;

	success = xmlTextWriterStartElement (project, (xmlChar *) "braseroproject");
	if (success < 0)
		goto error;

	/* write the name of the version */
	success = xmlTextWriterWriteElement (project,
					     (xmlChar *) "version",
					     (xmlChar *) "0.2");
	if (success < 0)
		goto error;

	title = totem_get_short_title (pi->priv->totem);
	if (title) {
		success = xmlTextWriterWriteElement (project,
						     (xmlChar *) "label",
						     (xmlChar *) title);
		g_free (title);

		if (success < 0)
			goto error;
	}

	success = xmlTextWriterStartElement (project, (xmlChar *) "track");
	if (success < 0)
		goto error;

	success = xmlTextWriterStartElement (project, (xmlChar *) "video");
	if (success < 0)
		goto error;

	uri = totem_get_current_mrl (pi->priv->totem);
	escaped = (unsigned char *) g_uri_escape_string (uri, NULL, FALSE);
	g_free (uri);

	success = xmlTextWriterWriteElement (project,
					     (xmlChar *) "uri",
					     escaped);
	g_free (escaped);
	if (success == -1)
		goto error;

	/* start of the song always 0 */
	success = xmlTextWriterWriteElement (project,
					     (xmlChar *) "start",
					     (xmlChar *) "0");
	if (success == -1)
		goto error;

	success = xmlTextWriterEndElement (project); /* video */
	if (success < 0)
		goto error;

	success = xmlTextWriterEndElement (project); /* track */
	if (success < 0)
		goto error;

	success = xmlTextWriterEndElement (project); /* braseroproject */
	if (success < 0)
		goto error;

	xmlTextWriterEndDocument (project);
	xmlFreeTextWriter (project);

	save = xmlSaveToFd (fd, "UTF8", XML_SAVE_FORMAT);
	xmlSaveDoc (save, doc);
	xmlSaveClose (save);

	xmlFreeDoc (doc);
	close (fd);

	return path;

error:

	/* cleanup */
	xmlTextWriterEndDocument (project);
	xmlFreeTextWriter (project);

	g_remove (path);
	g_free (path);
	close (fd);

	*error = g_strdup (_("Unable to write a project."));
	return NULL;
}

static void
totem_disc_recorder_plugin_burn (GtkAction *action,
				 TotemDiscRecorderPlugin *pi)
{
	char *path;
	char *error = NULL;

	path = totem_disc_recorder_plugin_write_video_project (pi, &error);
	if (!path) {
		totem_interface_error (_("The movie could not be recorded."),
				       error,
				       totem_get_main_window (pi->priv->totem));
		g_free (error);
		return;
	}

	if (!totem_disc_recorder_plugin_start_burning (pi, path, FALSE))
		g_remove (path);

	g_free (path);
}

static void
totem_disc_recorder_plugin_copy (GtkAction *action,
				 TotemDiscRecorderPlugin *pi)
{
	char *mrl;

	mrl = totem_get_current_mrl (pi->priv->totem);
	if (!g_str_has_prefix (mrl, "dvd:") && !g_str_has_prefix (mrl, "vcd:")) {
		g_free (mrl);
		g_assert_not_reached ();
		return;
	}

	totem_disc_recorder_plugin_start_burning (pi, mrl + 6, TRUE);
}

static void
totem_disc_recorder_file_closed (TotemObject *totem,
				 TotemDiscRecorderPlugin *pi)
{
	GtkAction *action;

	action = gtk_action_group_get_action (pi->priv->action_group, "VideoBurnToDisc");
	gtk_action_set_visible (action, FALSE);
	action = gtk_action_group_get_action (pi->priv->action_group, "VideoDVDCopy");
	gtk_action_set_visible (action, FALSE);
	action = gtk_action_group_get_action (pi->priv->action_group, "VideoVCDCopy");
	gtk_action_set_visible (action, FALSE);
}

static void
totem_disc_recorder_file_opened (TotemObject *totem,
				 const char *mrl,
				 TotemDiscRecorderPlugin *pi)
{
	TotemDiscRecorderPluginPrivate *priv = pi->priv;
	GtkAction *action;

	/* Check if that stream is supported by brasero */
	if (g_str_has_prefix (mrl, "file:")) {
		/* If the file is supported we can always burn, even if there
		 * aren't any burner since we can still create an image. */
		action = gtk_action_group_get_action (priv->action_group, "VideoBurnToDisc");
		gtk_action_set_visible (action, TRUE);
		action = gtk_action_group_get_action (priv->action_group, "VideoDVDCopy");
		gtk_action_set_visible (action, FALSE);
		action = gtk_action_group_get_action (priv->action_group, "VideoVCDCopy");
		gtk_action_set_visible (action, FALSE);
	}
	else if (g_str_has_prefix (mrl, "dvd:")) {
		action = gtk_action_group_get_action (priv->action_group, "VideoBurnToDisc");
		gtk_action_set_visible (action, FALSE);
		action = gtk_action_group_get_action (priv->action_group, "VideoDVDCopy");
		gtk_action_set_visible (action, TRUE);
		action = gtk_action_group_get_action (priv->action_group, "VideoVCDCopy");
		gtk_action_set_visible (action, FALSE);
	}
	else if (g_str_has_prefix (mrl, "vcd:")) {
		action = gtk_action_group_get_action (priv->action_group, "VideoBurnToDisc");
		gtk_action_set_visible (action, FALSE);
		action = gtk_action_group_get_action (priv->action_group, "VideoDVDCopy");
		gtk_action_set_visible (action, FALSE);
		action = gtk_action_group_get_action (priv->action_group, "VideoVCDCopy");
		gtk_action_set_visible (action, TRUE);
	}
	else {
		action = gtk_action_group_get_action (priv->action_group, "VideoBurnToDisc");
		gtk_action_set_visible (action, FALSE);
		action = gtk_action_group_get_action (priv->action_group, "VideoDVDCopy");
		gtk_action_set_visible (action, FALSE);
		action = gtk_action_group_get_action (priv->action_group, "VideoVCDCopy");
		gtk_action_set_visible (action, FALSE);
	}
}

static void
impl_activate (PeasActivatable *plugin)
{
	TotemDiscRecorderPlugin *pi = TOTEM_DISC_RECORDER_PLUGIN (plugin);
	TotemDiscRecorderPluginPrivate *priv = pi->priv;
	GtkUIManager *uimanager = NULL;
	GtkAction *action;
	char *path;

	/* make sure brasero is in the path */
	path = g_find_program_in_path ("brasero");
	if (!path)
		return;
	g_free (path);

	//FIXME this shouldn't be necessary
#if 0
	/* Set up to use brasero icons */
	path = g_build_path (G_DIR_SEPARATOR_S, LIBBRASERO_MEDIA_INSTALL, "/share/brasero/icons", NULL);
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), path);
	g_free (path);
#endif

	priv->totem = g_object_get_data (G_OBJECT (plugin), "object");

	g_signal_connect (priv->totem,
			  "file-opened",
			  G_CALLBACK (totem_disc_recorder_file_opened),
			  plugin);
	g_signal_connect (priv->totem,
			  "file-closed",
			  G_CALLBACK (totem_disc_recorder_file_closed),
			  plugin);

	/* add UI */
	priv->action_group = gtk_action_group_new ("DiscRecorderActions");
	gtk_action_group_set_translation_domain (priv->action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (priv->action_group,
				      totem_disc_recorder_plugin_actions,
				      G_N_ELEMENTS (totem_disc_recorder_plugin_actions),
				      pi);

	uimanager = totem_get_ui_manager (priv->totem);
	gtk_ui_manager_insert_action_group (uimanager, priv->action_group, -1);
	g_object_unref (priv->action_group);

	priv->ui_merge_id = gtk_ui_manager_new_merge_id (uimanager);

	gtk_ui_manager_add_ui (uimanager,
			       priv->ui_merge_id,
			       "/ui/tmw-menubar/movie/burn-placeholder",
			       "VideoBurnToDisc",
			       "VideoBurnToDisc",
			       GTK_UI_MANAGER_MENUITEM,
			       TRUE);

	gtk_ui_manager_add_ui (uimanager,
			       priv->ui_merge_id,
			       "/ui/tmw-menubar/movie/burn-placeholder",
			       "VideoDVDCopy",
			       "VideoDVDCopy",
			       GTK_UI_MANAGER_MENUITEM,
			       TRUE);

	gtk_ui_manager_add_ui (uimanager,
			       priv->ui_merge_id,
			       "/ui/tmw-menubar/movie/burn-placeholder",
			       "VideoVCDCopy",
			       "VideoVCDCopy",
			       GTK_UI_MANAGER_MENUITEM,
			       TRUE);

	if (!totem_is_paused (priv->totem) && !totem_is_playing (priv->totem)) {
		action = gtk_action_group_get_action (priv->action_group, "VideoBurnToDisc");
		gtk_action_set_visible (action, FALSE);
		action = gtk_action_group_get_action (priv->action_group, "VideoDVDCopy");
		gtk_action_set_visible (action, FALSE);
		action = gtk_action_group_get_action (priv->action_group, "VideoVCDCopy");
		gtk_action_set_visible (action, FALSE);	}
	else {
		char *mrl;

		mrl = totem_get_current_mrl (priv->totem);
		totem_disc_recorder_file_opened (priv->totem, mrl, pi);
		g_free (mrl);
	}
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	TotemDiscRecorderPlugin *pi = TOTEM_DISC_RECORDER_PLUGIN (plugin);
	TotemDiscRecorderPluginPrivate *priv = pi->priv;
	GtkUIManager *uimanager = NULL;

	g_signal_handlers_disconnect_by_func (priv->totem, totem_disc_recorder_file_opened, plugin);
	g_signal_handlers_disconnect_by_func (priv->totem, totem_disc_recorder_file_closed, plugin);

	uimanager = totem_get_ui_manager (priv->totem);
	gtk_ui_manager_remove_ui (uimanager, priv->ui_merge_id);
	gtk_ui_manager_remove_action_group (uimanager, priv->action_group);

	priv->totem = NULL;
}
