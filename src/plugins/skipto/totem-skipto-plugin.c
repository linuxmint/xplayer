/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Philip Withnall <philip@tecnocode.co.uk>
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
 * Monday 7th February 2005: Christian Schaller: Add excemption clause.
 * See license_change file for details.
 *
 * Author: Bastien Nocera <hadess@hadess.net>, Philip Withnall <philip@tecnocode.co.uk>
 */

#include "config.h"

#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <libpeas/peas-activatable.h>

#include "xplayer-plugin.h"
#include "xplayer-skipto.h"

#define XPLAYER_TYPE_SKIPTO_PLUGIN		(xplayer_skipto_plugin_get_type ())
#define XPLAYER_SKIPTO_PLUGIN(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), XPLAYER_TYPE_SKIPTO_PLUGIN, XplayerSkiptoPlugin))
#define XPLAYER_SKIPTO_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), XPLAYER_TYPE_SKIPTO_PLUGIN, XplayerSkiptoPluginClass))
#define XPLAYER_IS_SKIPTO_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), XPLAYER_TYPE_SKIPTO_PLUGIN))
#define XPLAYER_IS_SKIPTO_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), XPLAYER_TYPE_SKIPTO_PLUGIN))
#define XPLAYER_SKIPTO_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), XPLAYER_TYPE_SKIPTO_PLUGIN, XplayerSkiptoPluginClass))

typedef struct {
	XplayerObject	*xplayer;
	XplayerSkipto	*st;
	guint		handler_id_stream_length;
	guint		handler_id_seekable;
	guint		handler_id_key_press;
	guint		ui_merge_id;
	GtkActionGroup	*action_group;
} XplayerSkiptoPluginPrivate;

XPLAYER_PLUGIN_REGISTER(XPLAYER_TYPE_SKIPTO_PLUGIN, XplayerSkiptoPlugin, xplayer_skipto_plugin)

static void
destroy_dialog (XplayerSkiptoPlugin *plugin)
{
	XplayerSkiptoPluginPrivate *priv = plugin->priv;

	if (priv->st != NULL) {
		g_object_remove_weak_pointer (G_OBJECT (priv->st),
					      (gpointer *)&(priv->st));
		gtk_widget_destroy (GTK_WIDGET (priv->st));
		priv->st = NULL;
	}
}

static void
xplayer_skipto_update_from_state (XplayerObject *xplayer,
				XplayerSkiptoPlugin *plugin)
{
	gint64 _time;
	gboolean seekable;
	GtkAction *action;
	XplayerSkiptoPluginPrivate *priv = plugin->priv;

	g_object_get (G_OBJECT (xplayer),
				"stream-length", &_time,
				"seekable", &seekable,
				NULL);

	if (priv->st != NULL) {
		xplayer_skipto_update_range (priv->st, _time);
		xplayer_skipto_set_seekable (priv->st, seekable);
	}

	/* Update the action's sensitivity */
	action = gtk_action_group_get_action (priv->action_group, "skip-to");
	gtk_action_set_sensitive (action, seekable);
}

static void
property_notify_cb (XplayerObject *xplayer,
		    GParamSpec *spec,
		    XplayerSkiptoPlugin *plugin)
{
	xplayer_skipto_update_from_state (xplayer, plugin);
}

static void
skip_to_response_callback (GtkDialog *dialog, gint response, XplayerSkiptoPlugin *plugin)
{
	if (response != GTK_RESPONSE_OK) {
		destroy_dialog (plugin);
		return;
	}

	gtk_widget_hide (GTK_WIDGET (dialog));

	xplayer_action_seek_time (plugin->priv->xplayer,
				xplayer_skipto_get_range (plugin->priv->st),
				TRUE);
	destroy_dialog (plugin);
}

static void
run_skip_to_dialog (XplayerSkiptoPlugin *plugin)
{
	XplayerSkiptoPluginPrivate *priv = plugin->priv;

	if (xplayer_is_seekable (priv->xplayer) == FALSE)
		return;

	if (priv->st != NULL) {
		gtk_window_present (GTK_WINDOW (priv->st));
		xplayer_skipto_set_current (priv->st, xplayer_get_current_time
					  (priv->xplayer));
		return;
	}

	priv->st = XPLAYER_SKIPTO (xplayer_skipto_new (priv->xplayer));
	g_signal_connect (G_OBJECT (priv->st), "delete-event",
			  G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect (G_OBJECT (priv->st), "response",
			  G_CALLBACK (skip_to_response_callback), plugin);
	g_object_add_weak_pointer (G_OBJECT (priv->st),
				   (gpointer *)&(priv->st));
	xplayer_skipto_update_from_state (priv->xplayer, plugin);
	xplayer_skipto_set_current (priv->st,
				  xplayer_get_current_time (priv->xplayer));
}

static void
skip_to_action_callback (GtkAction *action, XplayerSkiptoPlugin *plugin)
{
	run_skip_to_dialog (plugin);
}

static gboolean
on_window_key_press_event (GtkWidget *window, GdkEventKey *event, XplayerSkiptoPlugin *plugin)
{

	if (event->state == 0 || !(event->state & GDK_CONTROL_MASK))
		return FALSE;

	switch (event->keyval) {
		case GDK_KEY_k:
		case GDK_KEY_K:
			run_skip_to_dialog (plugin);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

static void
impl_activate (PeasActivatable *plugin)
{
	GtkWindow *window;
	GtkUIManager *manager;
	XplayerSkiptoPlugin *pi = XPLAYER_SKIPTO_PLUGIN (plugin);
	XplayerSkiptoPluginPrivate *priv = pi->priv;

	const GtkActionEntry menu_entries[] = {
		{ "skip-to", GTK_STOCK_JUMP_TO, N_("_Skip To..."), "<Control>K", N_("Skip to a specific time"), G_CALLBACK (skip_to_action_callback) }
	};

	priv->xplayer = g_object_get_data (G_OBJECT (plugin), "object");
	priv->handler_id_stream_length = g_signal_connect (G_OBJECT (priv->xplayer),
				"notify::stream-length",
				G_CALLBACK (property_notify_cb),
				pi);
	priv->handler_id_seekable = g_signal_connect (G_OBJECT (priv->xplayer),
				"notify::seekable",
				G_CALLBACK (property_notify_cb),
				pi);

	/* Key press handler */
	window = xplayer_get_main_window (priv->xplayer);
	priv->handler_id_key_press = g_signal_connect (G_OBJECT(window),
				"key-press-event",
				G_CALLBACK (on_window_key_press_event),
				pi);
	g_object_unref (window);

	/* Install the menu */
	priv->action_group = gtk_action_group_new ("skip-to_group");
	gtk_action_group_set_translation_domain (priv->action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (priv->action_group, menu_entries,
				G_N_ELEMENTS (menu_entries), pi);

	manager = xplayer_get_ui_manager (priv->xplayer);

	gtk_ui_manager_insert_action_group (manager, priv->action_group, -1);
	g_object_unref (priv->action_group);

	priv->ui_merge_id = gtk_ui_manager_new_merge_id (manager);
	gtk_ui_manager_add_ui (manager, priv->ui_merge_id,
			       "/ui/tmw-menubar/go/skip-forward", "skip-to",
			       "skip-to", GTK_UI_MANAGER_AUTO, TRUE);

	xplayer_skipto_update_from_state (priv->xplayer, pi);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	GtkWindow *window;
	GtkUIManager *manager;
	XplayerObject *xplayer;
	XplayerSkiptoPluginPrivate *priv = XPLAYER_SKIPTO_PLUGIN (plugin)->priv;

	xplayer = g_object_get_data (G_OBJECT (plugin), "object");

	g_signal_handler_disconnect (G_OBJECT (xplayer),
				     priv->handler_id_stream_length);
	g_signal_handler_disconnect (G_OBJECT (xplayer),
				     priv->handler_id_seekable);

	if (priv->handler_id_key_press != 0) {
		window = xplayer_get_main_window (xplayer);
		g_signal_handler_disconnect (G_OBJECT(window),
					     priv->handler_id_key_press);
		priv->handler_id_key_press = 0;
		g_object_unref (window);
	}

	/* Remove the menu */
	manager = xplayer_get_ui_manager (xplayer);
	gtk_ui_manager_remove_ui (manager, priv->ui_merge_id);
	gtk_ui_manager_remove_action_group (manager, priv->action_group);

	destroy_dialog (XPLAYER_SKIPTO_PLUGIN (plugin));
}

