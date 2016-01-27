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
 * Monday 7th February 2005: Christian Schaller: Add excemption clause.
 * See license_change file for details.
 *
 * Author: Philip Withnall <philip@tecnocode.co.uk>
 */

#include "config.h"
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "totem-time-helpers.h"
#include "totem-time-entry.h"

static void dispose (GObject *object);
static gboolean output_cb (GtkSpinButton *self, gpointer user_data);
static gint input_cb (GtkSpinButton *self, gdouble *new_value, gpointer user_data);
static void notify_adjustment_cb (TotemTimeEntry *self, GParamSpec *pspec, gpointer user_data);
static void changed_cb (GtkAdjustment *adjustment, TotemTimeEntry *self);

struct TotemTimeEntryPrivate {
	GtkAdjustment *adjustment;
	gulong adjustment_changed_signal;
};

G_DEFINE_TYPE (TotemTimeEntry, totem_time_entry, GTK_TYPE_SPIN_BUTTON)

static gint64
totem_string_to_time (const char *time_string)
{
	int sec, min, hour, args;

	args = sscanf (time_string, C_("long time format", "%d:%02d:%02d"), &hour, &min, &sec);

	if (args == 3) {
		/* Parsed all three arguments successfully */
		return (hour * (60 * 60) + min * 60 + sec) * 1000;
	} else if (args == 2) {
		/* Only parsed the first two arguments; treat hour and min as min and sec, respectively */
		return (hour * 60 + min) * 1000;
	} else if (args == 1) {
		/* Only parsed the first argument; treat hour as sec */
		return hour * 1000;
	} else {
		/* Error! */
		return -1;
	}
}

static void
totem_time_entry_class_init (TotemTimeEntryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (TotemTimeEntryPrivate));

	object_class->dispose = dispose;
}

static void
totem_time_entry_init (TotemTimeEntry *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, TOTEM_TYPE_TIME_ENTRY, TotemTimeEntryPrivate);

	/* Connect to signals */
	g_signal_connect (self, "output", G_CALLBACK (output_cb), NULL);
	g_signal_connect (self, "input", G_CALLBACK (input_cb), NULL);
	g_signal_connect (self, "notify::adjustment", G_CALLBACK (notify_adjustment_cb), NULL);
}

static void
dispose (GObject *object)
{
	TotemTimeEntryPrivate *priv = TOTEM_TIME_ENTRY (object)->priv;

	if (priv->adjustment != NULL) {
		g_signal_handler_disconnect (priv->adjustment, priv->adjustment_changed_signal);
		g_object_unref (priv->adjustment);
	}
	priv->adjustment = NULL;

	G_OBJECT_CLASS (totem_time_entry_parent_class)->dispose (object);
}

GtkWidget *
totem_time_entry_new (GtkAdjustment *adjustment, gdouble climb_rate)
{
	return g_object_new (TOTEM_TYPE_TIME_ENTRY,
			     "adjustment", adjustment,
			     "climb-rate", climb_rate,
			     "digits", 0,
			     "numeric", FALSE,
			     NULL);
}

static gboolean
output_cb (GtkSpinButton *self, gpointer user_data)
{
	gchar *text;

	text = totem_time_to_string ((gint64) gtk_spin_button_get_value (self) * 1000);
	gtk_entry_set_text (GTK_ENTRY (self), text);
	g_free (text);

	return TRUE;
}

static gint
input_cb (GtkSpinButton *self, gdouble *new_value, gpointer user_data)
{
	gint64 val;

	val = totem_string_to_time (gtk_entry_get_text (GTK_ENTRY (self)));
	if (val == -1)
		return GTK_INPUT_ERROR;

	*new_value = val / 1000;
	return TRUE;
}

static void
notify_adjustment_cb (TotemTimeEntry *self, GParamSpec *pspec, gpointer user_data)
{
	TotemTimeEntryPrivate *priv = self->priv;

	if (priv->adjustment != NULL) {
		g_signal_handler_disconnect (priv->adjustment, priv->adjustment_changed_signal);
		g_object_unref (priv->adjustment);
	}

	priv->adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (self));
	priv->adjustment_changed_signal = 0;

	if (priv->adjustment != NULL) {
		g_object_ref (priv->adjustment);
		priv->adjustment_changed_signal = g_signal_connect (priv->adjustment, "changed", G_CALLBACK (changed_cb), self);
	}
}

static void
changed_cb (GtkAdjustment *adjustment, TotemTimeEntry *self)
{
	gchar *time_string;
	guint upper, width;

	/* Set the width of the entry according to the length of the longest string it'll now accept */
	upper = (guint) gtk_adjustment_get_upper (adjustment); /* in seconds */

	time_string = totem_time_to_string (((gint64) upper) * 1000);
	width = strlen (time_string);
	g_free (time_string);

	gtk_entry_set_width_chars (GTK_ENTRY (self), width);
}
