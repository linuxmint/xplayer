/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2008 Philip Withnall <philip@tecnocode.co.uk>
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
 */

#ifndef TOTEM_SCREENSHOT_PLUGIN_H
#define TOTEM_SCREENSHOT_PLUGIN_H

#include <glib.h>

#include "totem.h"
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

G_BEGIN_DECLS

gchar *totem_screenshot_plugin_setup_file_chooser (const char *filename_format, const char *movie_name) G_GNUC_WARN_UNUSED_RESULT;
void totem_screenshot_plugin_update_file_chooser (const char *filename);

G_END_DECLS

#endif /* !TOTEM_SCREENSHOT_PLUGIN_H */
