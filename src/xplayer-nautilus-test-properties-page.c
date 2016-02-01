/*
 * Copyright (C) 2005  Bastien Nocera <hadess@hadess.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * The Xplayer project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Xplayer. This
 * permission are above and beyond the permissions granted by the GPL license
 * Xplayer is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

#include <config.h>
#include <string.h>
#define GST_USE_UNSTABLE_API 1
#include <gst/gst.h>
#include <glib/gi18n-lib.h>
#include "xplayer-nautilus-properties-view.h"

static GtkWidget *window, *props, *label;

static void
create_props (const char *url)
{
	label = gtk_label_new ("Audio/Video");
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (gtk_main_quit), NULL);
	gtk_window_set_default_size (GTK_WINDOW (window), 450, 550);
	props = xplayer_properties_view_new (url, label);
	gtk_container_add (GTK_CONTAINER (window), props);

	gtk_widget_show_all (window);
}

static void
destroy_props (void)
{
	gtk_widget_destroy (label);
}

int main (int argc, char **argv)
{
	GFile *file;
	char *url;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gst_init (&argc, &argv);
	gtk_init (&argc, &argv);

	if (argc != 2) {
		g_print ("Usage: %s [URI]\n", argv[0]);
		return 1;
	}

	file = g_file_new_for_commandline_arg (argv[1]);
	url = g_file_get_uri (file);
	g_object_unref (file);

	create_props (url);
	g_free (url);

	gtk_main ();

	destroy_props ();

	return 0;
}

