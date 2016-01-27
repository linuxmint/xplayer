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
 * Monday 7th February 2005: Christian Schaller: Add excemption clause.
 * See license_change file for details.
 *
 * Author: Bastien Nocera <hadess@hadess.net>, Philip Withnall <philip@tecnocode.co.uk>
 */

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "xplayer-dirs.h"
#include "xplayer-skipto.h"
#include "xplayer-uri.h"
#include "backend/bacon-video-widget.h"

static void xplayer_skipto_dispose	(GObject *object);

/* Callback functions for GtkBuilder */
G_MODULE_EXPORT void time_entry_activate_cb (GtkEntry *entry, XplayerSkipto *skipto);
G_MODULE_EXPORT void tstw_adjustment_value_changed_cb (GtkAdjustment *adjustment, XplayerSkipto *skipto);

struct XplayerSkiptoPrivate {
	GtkBuilder *xml;
	GtkWidget *time_entry;
	GtkLabel *seconds_label;
	gint64 time;
	Xplayer *xplayer;
};

G_DEFINE_TYPE (XplayerSkipto, xplayer_skipto, GTK_TYPE_DIALOG)
#define XPLAYER_SKIPTO_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XPLAYER_TYPE_SKIPTO, XplayerSkiptoPrivate))

static void
xplayer_skipto_class_init (XplayerSkiptoClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (XplayerSkiptoPrivate));

	object_class->dispose = xplayer_skipto_dispose;
}

static void
xplayer_skipto_response_cb (GtkDialog *dialog, gint response_id, gpointer data)
{
	XplayerSkipto *skipto;

	skipto = XPLAYER_SKIPTO (dialog);
	gtk_spin_button_update (GTK_SPIN_BUTTON (skipto->priv->time_entry));
}

static void
xplayer_skipto_init (XplayerSkipto *skipto)
{
	skipto->priv = XPLAYER_SKIPTO_GET_PRIVATE (skipto);

	gtk_dialog_set_default_response (GTK_DIALOG (skipto), GTK_RESPONSE_OK);
	g_signal_connect (skipto, "response",
				G_CALLBACK (xplayer_skipto_response_cb), NULL);
}

static void
xplayer_skipto_dispose (GObject *object)
{
	XplayerSkipto *skipto;

	skipto = XPLAYER_SKIPTO (object);
	if (skipto->priv && skipto->priv->xml != NULL) {
		g_object_unref (skipto->priv->xml);
		skipto->priv->xml = NULL;
	}

	G_OBJECT_CLASS (xplayer_skipto_parent_class)->dispose (object);
}

void
xplayer_skipto_update_range (XplayerSkipto *skipto, gint64 _time)
{
	g_return_if_fail (XPLAYER_IS_SKIPTO (skipto));

	if (_time == skipto->priv->time)
		return;

	gtk_spin_button_set_range (GTK_SPIN_BUTTON (skipto->priv->time_entry),
			0, (gdouble) _time / 1000);
	skipto->priv->time = _time;
}

gint64
xplayer_skipto_get_range (XplayerSkipto *skipto)
{
	gint64 _time;

	g_return_val_if_fail (XPLAYER_IS_SKIPTO (skipto), 0);

	_time = gtk_spin_button_get_value (GTK_SPIN_BUTTON (skipto->priv->time_entry)) * 1000;

	return _time;
}

void
xplayer_skipto_set_seekable (XplayerSkipto *skipto, gboolean seekable)
{
	g_return_if_fail (XPLAYER_IS_SKIPTO (skipto));

	gtk_dialog_set_response_sensitive (GTK_DIALOG (skipto),
			GTK_RESPONSE_OK, seekable);
}

void
xplayer_skipto_set_current (XplayerSkipto *skipto, gint64 _time)
{
	g_return_if_fail (XPLAYER_IS_SKIPTO (skipto));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (skipto->priv->time_entry),
			(gdouble) (_time / 1000));
}

void
time_entry_activate_cb (GtkEntry *entry, XplayerSkipto *skipto)
{
	gtk_dialog_response (GTK_DIALOG (skipto), GTK_RESPONSE_OK);
}

void
tstw_adjustment_value_changed_cb (GtkAdjustment *adjustment, XplayerSkipto *skipto)
{
	/* Update the "seconds" label so that it always has the correct singular/plural form */
	/* Translators: label for the seconds selector in the "Skip to" dialogue */
	gtk_label_set_label (skipto->priv->seconds_label, ngettext ("second", "seconds", (int) gtk_adjustment_get_value (adjustment)));
}

GtkWidget *
xplayer_skipto_new (XplayerObject *xplayer)
{
	XplayerSkipto *skipto;
	GtkWidget *container;
	guint label_length;

	skipto = XPLAYER_SKIPTO (g_object_new (XPLAYER_TYPE_SKIPTO, NULL));

	skipto->priv->xplayer = xplayer;
	skipto->priv->xml = xplayer_plugin_load_interface ("skipto",
							 "skipto.ui", TRUE,
							 NULL, skipto);

	if (skipto->priv->xml == NULL) {
		g_object_unref (skipto);
		return NULL;
	}
	skipto->priv->time_entry = GTK_WIDGET (gtk_builder_get_object
		(skipto->priv->xml, "tstw_skip_time_entry"));
	skipto->priv->seconds_label = GTK_LABEL (gtk_builder_get_object
		(skipto->priv->xml, "tstw_seconds_label"));

	/* Fix the label width at the maximum necessary for the plural labels, to prevent it changing size when we change the spinner value */
	/* Translators: you should translate this string to a number (written in digits) which corresponds to the longer character length of the
	 * translations for "second" and "seconds", as translated elsewhere in this file. For example, in English, "second" is 6 characters long and
	 * "seconds" is 7 characters long, so this string should be translated to "7". See: bgo#639398 */
	label_length = strtoul (C_("Skip To label length", "7"), NULL, 10);
	gtk_label_set_width_chars (skipto->priv->seconds_label, label_length);

	/* Set the initial "seconds" label */
	tstw_adjustment_value_changed_cb (GTK_ADJUSTMENT (gtk_builder_get_object
		(skipto->priv->xml, "tstw_skip_adjustment")), skipto);

	gtk_window_set_title (GTK_WINDOW (skipto), _("Skip To"));
	gtk_dialog_add_buttons (GTK_DIALOG (skipto),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);

	/* Skipto dialog */
	g_signal_connect (G_OBJECT (skipto), "delete-event",
			  G_CALLBACK (gtk_widget_destroy), skipto);

	container = GTK_WIDGET (gtk_builder_get_object (skipto->priv->xml,
				"tstw_skip_vbox"));
	gtk_container_set_border_width (GTK_CONTAINER (skipto), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (skipto))),
			    container,
			    TRUE,       /* expand */
			    TRUE,       /* fill */
			    0);         /* padding */

	gtk_window_set_transient_for (GTK_WINDOW (skipto),
				      xplayer_get_main_window (xplayer));

	gtk_widget_show_all (GTK_WIDGET (skipto));

	return GTK_WIDGET (skipto);
}
