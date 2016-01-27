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

#include <glib.h>
#include <gtk/gtk.h>

char ** totem_get_plugin_paths (void);
char * totem_plugin_find_file (const char *plugin_name,
			       const char *file);
GtkBuilder * totem_plugin_load_interface (const char *plugin_name,
					  const char *name,
					  gboolean fatal,
					  GtkWindow *parent,
					  gpointer user_data);

