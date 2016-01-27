/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) Bastien Nocera 2010 <hadess@hadess.net>
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

#include "totem-plugin.h"
#include "totem-interface.h"

#define TOTEM_TYPE_SAVE_FILE_PLUGIN		(totem_save_file_plugin_get_type ())
#define TOTEM_SAVE_FILE_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_SAVE_FILE_PLUGIN, TotemSaveFilePlugin))
#define TOTEM_SAVE_FILE_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_SAVE_FILE_PLUGIN, TotemSaveFilePluginClass))
#define TOTEM_IS_SAVE_FILE_PLUGIN(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_SAVE_FILE_PLUGIN))
#define TOTEM_IS_SAVE_FILE_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_SAVE_FILE_PLUGIN))
#define TOTEM_SAVE_FILE_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_SAVE_FILE_PLUGIN, TotemSaveFilePluginClass))

typedef struct {
	TotemObject *totem;
	GtkWidget   *bvw;

	char        *mrl;
	char        *name;
	char        *save_uri;
	gboolean     is_tmp;

	GtkActionGroup *action_group;
	guint ui_merge_id;
} TotemSaveFilePluginPrivate;

TOTEM_PLUGIN_REGISTER(TOTEM_TYPE_SAVE_FILE_PLUGIN, TotemSaveFilePlugin, totem_save_file_plugin)

static void totem_save_file_plugin_copy (GtkAction *action,
					 TotemSaveFilePlugin *pi);

static GtkActionEntry totem_save_file_plugin_actions [] = {
	{ "SaveFile", "save-as", N_("Save a Copy..."), "<Ctrl>S",
		N_("Save a copy of the movie"),
		G_CALLBACK (totem_save_file_plugin_copy) },
};

static void
copy_uris_with_nautilus (const char *source,
			 const char *src_name,
			 const char *dest)
{
	GError *error = NULL;
	GDBusProxyFlags flags;
	GDBusProxy *proxy;
	GFile *dest_file, *parent;
	char *dest_name, *dest_dir;

	g_return_if_fail (source != NULL);
	g_return_if_fail (dest != NULL);
	g_return_if_fail (src_name != NULL); /* Must be "" or something interesting, not NULL */

	flags = G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES;
	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
					       flags,
					       NULL, /* GDBusInterfaceInfo */
					       "org.gnome.Nautilus",
					       "/org/gnome/Nautilus",
					       "org.gnome.Nautilus.FileOperations",
					       NULL, /* GCancellable */
					       &error);
	if (proxy == NULL) {
		g_warning ("Could not contact nautilus: %s", error->message);
		g_error_free (error);
		return;
	}

	dest_file = g_file_new_for_uri (dest);
	dest_name = g_file_get_basename (dest_file);
	parent = g_file_get_parent (dest_file);
	g_object_unref (dest_file);
	dest_dir = g_file_get_uri (parent);
	g_object_unref (parent);

	if (g_dbus_proxy_call_sync (proxy,
				"CopyFile", g_variant_new ("(&s&s&s&s)", source, src_name, dest_dir, dest_name),
				G_DBUS_CALL_FLAGS_NONE,
				-1, NULL, &error) == FALSE) {
		g_warning ("Could not get nautilus to copy file: %s", error->message);
		g_error_free (error);
	}

	g_free (dest_dir);
	g_free (dest_name);
	g_object_unref (proxy);
}

static void
totem_save_file_plugin_copy (GtkAction *action,
			     TotemSaveFilePlugin *pi)
{
	GtkWidget *fs;
	char *filename;
	int response;

	g_assert (pi->priv->mrl != NULL);


	fs = gtk_file_chooser_dialog_new (_("Save a Copy"),
					  totem_get_main_window (pi->priv->totem),
					  GTK_FILE_CHOOSER_ACTION_SAVE,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					  NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (fs), GTK_RESPONSE_ACCEPT);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (fs), FALSE);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (fs), TRUE);

	if (pi->priv->name != NULL) {
		filename = g_strdup (pi->priv->name);
	} else {
		GFile *file;
		char *basename;

		/* Try to get a nice filename from the URI */
		file = g_file_new_for_uri (pi->priv->mrl);
		basename = g_file_get_basename (file);
		g_object_unref (file);

		if (g_utf8_validate (basename, -1, NULL) == FALSE) {
			g_free (basename);
			filename = NULL;
		} else {
			filename = basename;
		}
	}

	if (filename == NULL) {
		/* translators: Movie is the default saved movie filename,
		 * without the suffix */
		filename = g_strdup (_("Movie"));
	}

	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (fs), filename);
	g_free (filename);

	if (pi->priv->save_uri != NULL) {
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (fs),
							 pi->priv->save_uri);
	}

	response = gtk_dialog_run (GTK_DIALOG (fs));
	gtk_widget_hide (fs);

	if (response == GTK_RESPONSE_ACCEPT) {
		char *dest_uri;

		dest_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (fs));
		/* translators: "Movie stream" is what will show up in the
		 * nautilus copy dialogue as the source, when saving a streamed
		 * movie */
		copy_uris_with_nautilus (pi->priv->mrl,
					 pi->priv->is_tmp ? _("Movie stream") : "",
					 dest_uri);

		g_free (pi->priv->save_uri);
		pi->priv->save_uri = g_path_get_dirname (dest_uri);
		g_free (dest_uri);
	}
	gtk_widget_destroy (fs);
}

static void
totem_save_file_file_closed (TotemObject *totem,
				 TotemSaveFilePlugin *pi)
{
	GtkAction *action;

	g_free (pi->priv->mrl);
	pi->priv->mrl = NULL;
	g_free (pi->priv->name);
	pi->priv->name = NULL;

	action = gtk_action_group_get_action (pi->priv->action_group, "SaveFile");
	gtk_action_set_sensitive (action, FALSE);
}

static void
totem_save_file_file_opened (TotemObject *totem,
			     const char *mrl,
			     TotemSaveFilePlugin *pi)
{
	TotemSaveFilePluginPrivate *priv = pi->priv;
	GtkAction *action;

	if (pi->priv->mrl != NULL) {
		g_free (pi->priv->mrl);
		pi->priv->mrl = NULL;
		g_free (pi->priv->name);
		pi->priv->name = NULL;
	}

	if (mrl == NULL)
		return;

	if (g_str_has_prefix (mrl, "file:") || g_str_has_prefix (mrl, "smb:")) {
		/* We can always copy files from file:/// URIs */
		action = gtk_action_group_get_action (priv->action_group, "SaveFile");
		gtk_action_set_sensitive (action, TRUE);
		pi->priv->mrl = g_strdup (mrl);
		pi->priv->name = totem_get_short_title (pi->priv->totem);
		pi->priv->is_tmp = FALSE;
	}
}

static void
totem_save_file_download_filename (GObject    *gobject,
				   GParamSpec *pspec,
				   TotemSaveFilePlugin *pi)
{
	GtkAction *action;
	char *filename;

	/* We're already ready to copy it */
	if (pi->priv->mrl != NULL)
		return;

	filename = NULL;
	g_object_get (G_OBJECT (pi->priv->bvw), "download-filename", &filename, NULL);
	if (filename == NULL)
		return;

	pi->priv->mrl = g_filename_to_uri (filename, NULL, NULL);
	g_free (filename);
	pi->priv->name = totem_get_short_title (pi->priv->totem);
	pi->priv->is_tmp = TRUE;

	action = gtk_action_group_get_action (pi->priv->action_group, "SaveFile");
	gtk_action_set_sensitive (action, TRUE);
}

static void
impl_activate (PeasActivatable *plugin)
{
	TotemSaveFilePlugin *pi = TOTEM_SAVE_FILE_PLUGIN (plugin);
	TotemSaveFilePluginPrivate *priv = pi->priv;
	GtkUIManager *uimanager = NULL;
	GtkAction *action;
	char *path;
	char *mrl;

	/* make sure nautilus is in the path */
	path = g_find_program_in_path ("nautilus");
	if (!path)
		return;
	g_free (path);

	priv->totem = g_object_get_data (G_OBJECT (plugin), "object");
	priv->bvw = totem_get_video_widget (priv->totem);

	g_signal_connect (priv->totem,
			  "file-opened",
			  G_CALLBACK (totem_save_file_file_opened),
			  plugin);
	g_signal_connect (priv->totem,
			  "file-closed",
			  G_CALLBACK (totem_save_file_file_closed),
			  plugin);
	g_signal_connect (priv->bvw,
			  "notify::download-filename",
			  G_CALLBACK (totem_save_file_download_filename),
			  plugin);

	/* add UI */
	priv->action_group = gtk_action_group_new ("SaveFileActions");
	gtk_action_group_set_translation_domain (priv->action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (priv->action_group,
				      totem_save_file_plugin_actions,
				      G_N_ELEMENTS (totem_save_file_plugin_actions),
				      pi);

	uimanager = totem_get_ui_manager (priv->totem);
	gtk_ui_manager_insert_action_group (uimanager, priv->action_group, -1);
	g_object_unref (priv->action_group);

	priv->ui_merge_id = gtk_ui_manager_new_merge_id (uimanager);

	gtk_ui_manager_add_ui (uimanager,
			       priv->ui_merge_id,
			       "/ui/tmw-menubar/movie/save-placeholder",
			       "SaveFile",
			       "SaveFile",
			       GTK_UI_MANAGER_MENUITEM,
			       TRUE);
	gtk_ui_manager_add_ui (uimanager,
			       priv->ui_merge_id,
			       "/ui/totem-main-popup/save-placeholder",
			       "SaveFile",
			       "SaveFile",
			       GTK_UI_MANAGER_MENUITEM,
			       TRUE);

	action = gtk_action_group_get_action (priv->action_group, "SaveFile");
	gtk_action_set_sensitive (action, FALSE);

	mrl = totem_get_current_mrl (priv->totem);
	totem_save_file_file_opened (priv->totem, mrl, pi);
	totem_save_file_download_filename (NULL, NULL, pi);
	g_free (mrl);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	TotemSaveFilePlugin *pi = TOTEM_SAVE_FILE_PLUGIN (plugin);
	TotemSaveFilePluginPrivate *priv = pi->priv;
	GtkUIManager *uimanager = NULL;

	g_signal_handlers_disconnect_by_func (priv->totem, totem_save_file_file_opened, plugin);
	g_signal_handlers_disconnect_by_func (priv->totem, totem_save_file_file_closed, plugin);
	g_signal_handlers_disconnect_by_func (priv->bvw, totem_save_file_download_filename, plugin);

	uimanager = totem_get_ui_manager (priv->totem);
	gtk_ui_manager_remove_ui (uimanager, priv->ui_merge_id);
	gtk_ui_manager_remove_action_group (uimanager, priv->action_group);

	priv->totem = NULL;
	priv->bvw = NULL;

	g_free (priv->mrl);
	priv->mrl = NULL;
	g_free (priv->name);
	priv->name = NULL;
	g_free (priv->save_uri);
	priv->save_uri = NULL;
}
