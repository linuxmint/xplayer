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
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
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

#include "totem-plugin.h"
#include "totem-skipto.h"

#define TOTEM_TYPE_SKIPTO_PLUGIN		(totem_skipto_plugin_get_type ())
#define TOTEM_SKIPTO_PLUGIN(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_SKIPTO_PLUGIN, TotemSkiptoPlugin))
#define TOTEM_SKIPTO_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_SKIPTO_PLUGIN, TotemSkiptoPluginClass))
#define TOTEM_IS_SKIPTO_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_SKIPTO_PLUGIN))
#define TOTEM_IS_SKIPTO_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_SKIPTO_PLUGIN))
#define TOTEM_SKIPTO_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_SKIPTO_PLUGIN, TotemSkiptoPluginClass))

typedef struct {
	TotemObject	*totem;
	TotemSkipto	*st;
	guint		handler_id_stream_length;
	guint		handler_id_seekable;
	guint		handler_id_key_press;
	guint		ui_merge_id;
	GtkActionGroup	*action_group;
} TotemSkiptoPluginPrivate;

TOTEM_PLUGIN_REGISTER(TOTEM_TYPE_SKIPTO_PLUGIN, TotemSkiptoPlugin, totem_skipto_plugin)

static void
destroy_dialog (TotemSkiptoPlugin *plugin)
{
	TotemSkiptoPluginPrivate *priv = plugin->priv;

	if (priv->st != NULL) {
		g_object_remove_weak_pointer (G_OBJECT (priv->st),
					      (gpointer *)&(priv->st));
		gtk_widget_destroy (GTK_WIDGET (priv->st));
		priv->st = NULL;
	}
}

static void
totem_skipto_update_from_state (TotemObject *totem,
				TotemSkiptoPlugin *plugin)
{
	gint64 _time;
	gboolean seekable;
	GtkAction *action;
	TotemSkiptoPluginPrivate *priv = plugin->priv;

	g_object_get (G_OBJECT (totem),
				"stream-length", &_time,
				"seekable", &seekable,
				NULL);

	if (priv->st != NULL) {
		totem_skipto_update_range (priv->st, _time);
		totem_skipto_set_seekable (priv->st, seekable);
	}

	/* Update the action's sensitivity */
	action = gtk_action_group_get_action (priv->action_group, "skip-to");
	gtk_action_set_sensitive (action, seekable);
}

static void
property_notify_cb (TotemObject *totem,
		    GParamSpec *spec,
		    TotemSkiptoPlugin *plugin)
{
	totem_skipto_update_from_state (totem, plugin);
}

static void
skip_to_response_callback (GtkDialog *dialog, gint response, TotemSkiptoPlugin *plugin)
{
	if (response != GTK_RESPONSE_OK) {
		destroy_dialog (plugin);
		return;
	}

	gtk_widget_hide (GTK_WIDGET (dialog));

	totem_action_seek_time (plugin->priv->totem,
				totem_skipto_get_range (plugin->priv->st),
				TRUE);
	destroy_dialog (plugin);
}

static void
run_skip_to_dialog (TotemSkiptoPlugin *plugin)
{
	TotemSkiptoPluginPrivate *priv = plugin->priv;

	if (totem_is_seekable (priv->totem) == FALSE)
		return;

	if (priv->st != NULL) {
		gtk_window_present (GTK_WINDOW (priv->st));
		totem_skipto_set_current (priv->st, totem_get_current_time
					  (priv->totem));
		return;
	}

	priv->st = TOTEM_SKIPTO (totem_skipto_new (priv->totem));
	g_signal_connect (G_OBJECT (priv->st), "delete-event",
			  G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect (G_OBJECT (priv->st), "response",
			  G_CALLBACK (skip_to_response_callback), plugin);
	g_object_add_weak_pointer (G_OBJECT (priv->st),
				   (gpointer *)&(priv->st));
	totem_skipto_update_from_state (priv->totem, plugin);
	totem_skipto_set_current (priv->st,
				  totem_get_current_time (priv->totem));
}

static void
skip_to_action_callback (GtkAction *action, TotemSkiptoPlugin *plugin)
{
	run_skip_to_dialog (plugin);
}

static gboolean
on_window_key_press_event (GtkWidget *window, GdkEventKey *event, TotemSkiptoPlugin *plugin)
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
	TotemSkiptoPlugin *pi = TOTEM_SKIPTO_PLUGIN (plugin);
	TotemSkiptoPluginPrivate *priv = pi->priv;

	const GtkActionEntry menu_entries[] = {
		{ "skip-to", GTK_STOCK_JUMP_TO, N_("_Skip To..."), "<Control>K", N_("Skip to a specific time"), G_CALLBACK (skip_to_action_callback) }
	};

	priv->totem = g_object_get_data (G_OBJECT (plugin), "object");
	priv->handler_id_stream_length = g_signal_connect (G_OBJECT (priv->totem),
				"notify::stream-length",
				G_CALLBACK (property_notify_cb),
				pi);
	priv->handler_id_seekable = g_signal_connect (G_OBJECT (priv->totem),
				"notify::seekable",
				G_CALLBACK (property_notify_cb),
				pi);

	/* Key press handler */
	window = totem_get_main_window (priv->totem);
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

	manager = totem_get_ui_manager (priv->totem);

	gtk_ui_manager_insert_action_group (manager, priv->action_group, -1);
	g_object_unref (priv->action_group);

	priv->ui_merge_id = gtk_ui_manager_new_merge_id (manager);
	gtk_ui_manager_add_ui (manager, priv->ui_merge_id,
			       "/ui/tmw-menubar/go/skip-forward", "skip-to",
			       "skip-to", GTK_UI_MANAGER_AUTO, TRUE);

	totem_skipto_update_from_state (priv->totem, pi);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	GtkWindow *window;
	GtkUIManager *manager;
	TotemObject *totem;
	TotemSkiptoPluginPrivate *priv = TOTEM_SKIPTO_PLUGIN (plugin)->priv;

	totem = g_object_get_data (G_OBJECT (plugin), "object");

	g_signal_handler_disconnect (G_OBJECT (totem),
				     priv->handler_id_stream_length);
	g_signal_handler_disconnect (G_OBJECT (totem),
				     priv->handler_id_seekable);

	if (priv->handler_id_key_press != 0) {
		window = totem_get_main_window (totem);
		g_signal_handler_disconnect (G_OBJECT(window),
					     priv->handler_id_key_press);
		priv->handler_id_key_press = 0;
		g_object_unref (window);
	}

	/* Remove the menu */
	manager = totem_get_ui_manager (totem);
	gtk_ui_manager_remove_ui (manager, priv->ui_merge_id);
	gtk_ui_manager_remove_action_group (manager, priv->action_group);

	destroy_dialog (TOTEM_SKIPTO_PLUGIN (plugin));
}

