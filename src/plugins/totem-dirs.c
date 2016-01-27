/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * heavily based on code from Rhythmbox and Gedit
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Sunday 13th May 2007: Bastien Nocera: Add exception clause.
 * See license_change file for details.
 *
 */

/**
 * SECTION:totem-plugin
 * @short_description: base plugin class and loading/unloading functions
 * @stability: Unstable
 * @include: totem-dirs.h
 *
 * libpeas is used as a general-purpose architecture for adding plugins to Totem, with
 * derived support for different programming languages.
 *
 * The functions in totem-dirs.h are used to allow plugins to find and load files installed alongside the plugins, such as UI files.
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include "totem-dirs.h"
#include "totem-plugins-engine.h"
#include "totem-uri.h"
#include "totem-interface.h"

#define UNINSTALLED_PLUGINS_LOCATION "plugins"

/**
 * totem_get_plugin_paths:
 *
 * Return a %NULL-terminated array of paths to directories which can contain Totem plugins. This respects the GSettings disable_user_plugins setting.
 *
 * Return value: (transfer full): a %NULL-terminated array of paths to plugin directories
 *
 * Since: 2.90.0
 **/
char **
totem_get_plugin_paths (void)
{
	GPtrArray *paths;
	char  *path;
	GSettings *settings;
	gboolean uninstalled;

	paths = g_ptr_array_new ();
	uninstalled = FALSE;

#ifdef TOTEM_RUN_IN_SOURCE_TREE
	path = g_build_filename (UNINSTALLED_PLUGINS_LOCATION, NULL);
	if (g_file_test (path, G_FILE_TEST_IS_DIR) != FALSE) {
		uninstalled = TRUE;
		g_ptr_array_add (paths, path);
	}
#endif

	settings = g_settings_new (TOTEM_GSETTINGS_SCHEMA);
	if (g_settings_get_boolean (settings, "disable-user-plugins") == FALSE) {
		path = g_build_filename (totem_data_dot_dir (), "plugins", NULL);
		g_ptr_array_add (paths, path);
	}

	g_object_unref (settings);

	if (uninstalled == FALSE) {
		path = g_strdup (TOTEM_PLUGIN_DIR);
		g_ptr_array_add (paths, path);
	}

	/* And null-terminate the array */
	g_ptr_array_add (paths, NULL);

	return (char **) g_ptr_array_free (paths, FALSE);
}

/**
 * totem_plugin_find_file:
 * @plugin_name: the plugin name
 * @file: the file to find
 *
 * Finds the specified @file by looking in the plugin paths
 * listed by totem_get_plugin_paths() and then in the system
 * Totem data directory.
 *
 * This should be used by plugins to find plugin-specific
 * resource files.
 *
 * Return value: a newly-allocated absolute path for the file, or %NULL
 **/
char *
totem_plugin_find_file (const char *plugin_name,
			const char *file)
{
	TotemPluginsEngine *engine;
	PeasPluginInfo *info;
	const char *dir;
	char *tmp;
	char *ret = NULL;

	engine = totem_plugins_engine_get_default (NULL);
	info = peas_engine_get_plugin_info (PEAS_ENGINE (engine), plugin_name);

	dir = peas_plugin_info_get_module_dir (info);
	tmp = g_build_filename (dir, file, NULL);
	if (g_file_test (tmp, G_FILE_TEST_EXISTS))
		ret = tmp;
	else
		g_free (tmp);

	if (ret == NULL) {
		dir = peas_plugin_info_get_data_dir (info);
		tmp = g_build_filename (dir, file, NULL);
		if (g_file_test (tmp, G_FILE_TEST_EXISTS))
			ret = tmp;
		else
			g_free (tmp);
	}

	/* global data files */
	if (ret == NULL)
		ret = totem_interface_get_full_path (file);

	g_object_unref (engine);

	//FIXME
#if 0
	/* ensure it's an absolute path, so doesn't confuse rb_glade_new et al */
	if (ret && ret[0] != '/') {
		char *pwd = g_get_current_dir ();
		char *path = g_strconcat (pwd, G_DIR_SEPARATOR_S, ret, NULL);
		g_free (ret);
		g_free (pwd);
		ret = path;
	}
#endif
	return ret;
}

/**
 * totem_plugin_load_interface:
 * @plugin_name: the plugin name
 * @name: interface filename
 * @fatal: %TRUE if it's a fatal error if the interface can't be loaded
 * @parent: (allow-none): the interface's parent #GtkWindow
 * @user_data: (allow-none): a pointer to be passed to each signal handler in the interface when they're called
 *
 * Loads an interface file (GtkBuilder UI file) for a plugin, given its filename and
 * assuming it's installed in the plugin's data directory.
 *
 * This should be used instead of attempting to load interfaces manually in plugins.
 *
 * Return value: (transfer full): the #GtkBuilder instance for the interface
 **/
GtkBuilder *
totem_plugin_load_interface (const char *plugin_name,
			     const char *name,
			     gboolean fatal,
			     GtkWindow *parent,
			     gpointer user_data)
{
	GtkBuilder *builder = NULL;
	char *filename;

	filename = totem_plugin_find_file (plugin_name, name);
	builder = totem_interface_load_with_full_path (filename,
						       fatal,
						       parent,
						       user_data);
	g_free (filename);

	return builder;
}
