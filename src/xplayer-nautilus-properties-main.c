/*
 * Copyright (C) 2000, 2001 Eazel Inc.
 * Copyright (C) 2003  Andrew Sobala <aes@gnome.org>
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
 */

#include <config.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#define GST_USE_UNSTABLE_API 1
#include <gst/gst.h>

#include "xplayer-nautilus-properties-view.h"
#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-property-page-provider.h>

#include "xplayer-mime-types.h"

static GType tpp_type = 0;
static gboolean backend_inited = FALSE;
static void property_page_provider_iface_init
	(NautilusPropertyPageProviderIface *iface);
static GList *xplayer_properties_get_pages
	(NautilusPropertyPageProvider *provider, GList *files);

static void
xplayer_properties_plugin_register_type (GTypeModule *module)
{
	const GTypeInfo info = {
		sizeof (GObjectClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) NULL,
		NULL,
		NULL,
		sizeof (GObject),
		0,
		(GInstanceInitFunc) NULL
	};
	const GInterfaceInfo property_page_provider_iface_info = {
		(GInterfaceInitFunc)property_page_provider_iface_init,
		NULL,
		NULL
	};

	tpp_type = g_type_module_register_type (module, G_TYPE_OBJECT,
			"XplayerPropertiesPlugin",
			&info, 0);
	g_type_module_add_interface (module,
			tpp_type,
			NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER,
			&property_page_provider_iface_info);
}

static void
property_page_provider_iface_init (NautilusPropertyPageProviderIface *iface)
{
	iface->get_pages = xplayer_properties_get_pages;
}

static GList *
xplayer_properties_get_pages (NautilusPropertyPageProvider *provider,
			     GList *files)
{
	GList *pages = NULL;
	NautilusFileInfo *file;
	char *uri = NULL;
	GtkWidget *page, *label;
	NautilusPropertyPage *property_page;
	guint i;
	gboolean found = FALSE;

	/* only add properties page if a single file is selected */
	if (files == NULL || files->next != NULL)
		goto end;
	file = files->data;

	/* only add the properties page to these mime types */
	for (i = 0; mime_types[i] != NULL; i++)
	{
		if (nautilus_file_info_is_mime_type (file, mime_types[i]))
		{
			found = TRUE;
			break;
		}
	}
	if (found == FALSE)
		goto end;

	/* okay, make the page, init'ing the backend first if necessary */
	if (backend_inited == FALSE) {
		gst_init (NULL, NULL);
		backend_inited = TRUE;
	}
	uri = nautilus_file_info_get_uri (file);
	label = gtk_label_new (_("Audio/Video"));
	page = xplayer_properties_view_new (uri, label);
	gtk_container_set_border_width (GTK_CONTAINER (page), 6);
	property_page = nautilus_property_page_new ("video-properties",
			label, page);

	pages = g_list_prepend (pages, property_page);

end:
	g_free (uri);
	return pages;
}

/* --- extension interface --- */
void
nautilus_module_initialize (GTypeModule *module)
{
	/* set up translation catalog */
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	xplayer_properties_plugin_register_type (module);
	xplayer_properties_view_register_type (module);
}

void
nautilus_module_shutdown (void)
{
}

void
nautilus_module_list_types (const GType **types,
                            int          *num_types)
{
	static GType type_list[1];

	type_list[0] = tpp_type;
	*types = type_list;
	*num_types = G_N_ELEMENTS (type_list);
}

