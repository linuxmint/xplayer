/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Xplayer project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Xplayer. This
 * permission are above and beyond the permissions granted by the GPL license
 * Xplayer is covered by.
 *
 * See license_change file for details.
 *
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <string.h>

#include "xplayer-plugin.h"
#include "xplayer.h"

#define XPLAYER_TYPE_SIDEBAR_TEST_PLUGIN		(xplayer_sidebar_test_plugin_get_type ())
#define XPLAYER_SIDEBAR_TEST_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), XPLAYER_TYPE_SIDEBAR_TEST_PLUGIN, XplayerSidebarTestPlugin))
#define XPLAYER_SIDEBAR_TEST_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), XPLAYER_TYPE_SIDEBAR_TEST_PLUGIN, XplayerSidebarTestPluginClass))
#define XPLAYER_IS_SIDEBAR_TEST_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), XPLAYER_TYPE_SIDEBAR_TEST_PLUGIN))
#define XPLAYER_IS_SIDEBAR_TEST_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), XPLAYER_TYPE_SIDEBAR_TEST_PLUGIN))
#define XPLAYER_SIDEBAR_TEST_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), XPLAYER_TYPE_SIDEBAR_TEST_PLUGIN, XplayerSidebarTestPluginClass))

typedef struct {
	gpointer unused;
} XplayerSidebarTestPluginPrivate;

XPLAYER_PLUGIN_REGISTER(XPLAYER_TYPE_SIDEBAR_TEST_PLUGIN, XplayerSidebarTestPlugin, xplayer_sidebar_test_plugin)

static void
impl_activate (PeasActivatable *plugin)
{
	GtkWidget *label;

	label = gtk_label_new ("This is a test sidebar main widget");
	gtk_widget_show (label);
	xplayer_add_sidebar_page (g_object_get_data (G_OBJECT (plugin), "object"),
				"sidebar-test",
				"Sidebar Test",
				label);
	g_message ("Just added a test sidebar");
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	XplayerObject *xplayer;

	xplayer = g_object_get_data (G_OBJECT (plugin), "object");
	xplayer_remove_sidebar_page (xplayer, "sidebar-test");
	g_message ("Just removed a test sidebar");
}

