/*
 * Copyright (c) 2012 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Bastien Nocera <hadess@hadess.net>
 *
 */

#include "totem-search-entry.h"
#include "libgd/gd-tagged-entry.h"

G_DEFINE_TYPE (TotemSearchEntry, totem_search_entry, GTK_TYPE_BOX)

/* To be used as the ID in the GdTaggedEntry */
#define SOURCE_ID "source-id"

enum {
	SIGNAL_ACTIVATE,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_SELECTED_ID
};

static guint signals[LAST_SIGNAL] = { 0, };

struct _TotemSearchEntryPrivate {
	GtkWidget *entry;
	GtkWidget *button;
	GtkWidget *menu;
	GSList *group;
};

static void
totem_search_entry_finalize (GObject *obj)
{
	TotemSearchEntry *self = TOTEM_SEARCH_ENTRY (obj);

	/* FIXME */

	G_OBJECT_CLASS (totem_search_entry_parent_class)->finalize (obj);
}

static void
entry_activate_cb (GtkEntry *entry,
		   TotemSearchEntry *self)
{
	const char *text;

	text = gtk_entry_get_text (GTK_ENTRY (self->priv->entry));
	if (text == NULL || *text == '\0')
		return;
	g_signal_emit (self, signals[SIGNAL_ACTIVATE], 0);
}

static void
totem_search_entry_init (TotemSearchEntry *self)
{
	GtkWidget *entry;
	GtkWidget *button;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, TOTEM_TYPE_SEARCH_ENTRY, TotemSearchEntryPrivate);

	/* Entry */
	entry = GTK_WIDGET (gd_tagged_entry_new ());
	gd_tagged_entry_set_tag_button_visible (GD_TAGGED_ENTRY (entry), FALSE);
	gtk_box_pack_start (GTK_BOX (self),
			    entry,
			    TRUE, TRUE, 0);
	gtk_widget_show (entry);

	self->priv->entry = entry;

	/* Button */
	button = gtk_menu_button_new ();
	gtk_box_pack_start (GTK_BOX (self),
			    button,
			    FALSE, TRUE, 0);
	gtk_widget_show (button);

	self->priv->button = button;

	/* Connect signals */
	g_signal_connect (self->priv->entry, "activate",
			  G_CALLBACK (entry_activate_cb), self);
}

static void
totem_search_entry_set_property (GObject *object,
				 guint property_id,
                                 const GValue *value,
                                 GParamSpec * pspec)
{
	switch (property_id) {
	case PROP_SELECTED_ID:
		totem_search_entry_set_selected_id (TOTEM_SEARCH_ENTRY (object),
						    g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
totem_search_entry_get_property (GObject    *object,
				 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_SELECTED_ID:
		g_value_set_string (value,
				    totem_search_entry_get_selected_id (TOTEM_SEARCH_ENTRY (object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
totem_search_entry_class_init (TotemSearchEntryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->finalize = totem_search_entry_finalize;
	gobject_class->set_property = totem_search_entry_set_property;
	gobject_class->get_property = totem_search_entry_get_property;

	signals[SIGNAL_ACTIVATE] =
		g_signal_new ("activate",
			      TOTEM_TYPE_SEARCH_ENTRY,
			      G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
			      0, NULL, NULL, NULL,
			      G_TYPE_NONE,
			      0, G_TYPE_NONE);

	g_object_class_install_property (gobject_class, PROP_SELECTED_ID,
					 g_param_spec_string ("selected-id", "Selected ID", "The ID for the currently selected source.",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (klass, sizeof (TotemSearchEntryPrivate));
}

TotemSearchEntry *
totem_search_entry_new (void)
{
	return g_object_new (TOTEM_TYPE_SEARCH_ENTRY, NULL);
}

static void
item_toggled (GtkCheckMenuItem *item,
	      TotemSearchEntry *self)
{
	const char *label;

	if (gtk_check_menu_item_get_active (item)) {
		label = g_object_get_data (G_OBJECT (item), "label");
		gd_tagged_entry_set_tag_label (GD_TAGGED_ENTRY (self->priv->entry),
					       SOURCE_ID, label);
		g_object_notify (G_OBJECT (self), "selected-id");
	}
}

static void
insert_item_sorted (TotemSearchEntry *self,
		    int               priority,
		    GtkWidget        *item)
{
	/* FIXME really do that sorted */
	gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->menu), item);
	gtk_widget_show (item);
}

void
totem_search_entry_add_source (TotemSearchEntry *self,
			       const gchar      *id,
			       const gchar      *label,
			       int               priority)
{
	GtkWidget *item;

	g_return_if_fail (TOTEM_IS_SEARCH_ENTRY (self));

	if (self->priv->menu == NULL) {
		self->priv->menu = gtk_menu_new ();
		gtk_menu_button_set_popup (GTK_MENU_BUTTON (self->priv->button),
					   self->priv->menu);
		gd_tagged_entry_add_tag (GD_TAGGED_ENTRY (self->priv->entry),
					 SOURCE_ID, label);
	}

	item = gtk_radio_menu_item_new_with_label (self->priv->group, label);
	self->priv->group = g_slist_prepend (self->priv->group, item);

	g_object_set_data_full (G_OBJECT (item), "id", g_strdup (id), g_free);
	g_object_set_data_full (G_OBJECT (item), "label", g_strdup (label), g_free);
	g_object_set_data (G_OBJECT (item), "priority", GINT_TO_POINTER (priority));

	g_signal_connect (item, "toggled",
			  G_CALLBACK (item_toggled), self);

	insert_item_sorted (self, priority, item);
}

void
totem_search_entry_remove_source (TotemSearchEntry *self,
				  const gchar *id)
{
	guint num_items;

	g_return_if_fail (TOTEM_IS_SEARCH_ENTRY (self));

	/* FIXME
	 * - implement
	 * - don't forget to remove tag
	 * - check if it's the currently selected source and notify of the change if so */

	num_items = 1;

	if (num_items == 0) {
		gtk_menu_button_set_popup (GTK_MENU_BUTTON (self->priv->button), NULL);
		g_clear_object (&self->priv->menu);
		gd_tagged_entry_remove_tag (GD_TAGGED_ENTRY (self->priv->entry), SOURCE_ID);
	}
}

const char *
totem_search_entry_get_text (TotemSearchEntry *self)
{
	g_return_val_if_fail (TOTEM_IS_SEARCH_ENTRY (self), NULL);

	return gtk_entry_get_text (GTK_ENTRY (self->priv->entry));
}

const char *
totem_search_entry_get_selected_id (TotemSearchEntry *self)
{
	GSList *l;

	g_return_val_if_fail (TOTEM_IS_SEARCH_ENTRY (self), NULL);

	for (l = self->priv->group ; l != NULL; l = l->next) {
		GtkCheckMenuItem *item = l->data;

		if (gtk_check_menu_item_get_active (item) != FALSE)
			return g_object_get_data (G_OBJECT (item), "id");
	}

	return NULL;
}

void
totem_search_entry_set_selected_id (TotemSearchEntry *self,
				    const char       *id)
{
	GSList *l;

	g_return_if_fail (TOTEM_IS_SEARCH_ENTRY (self));
	g_return_if_fail (id != NULL);

	for (l = self->priv->group ; l != NULL; l = l->next) {
		GtkCheckMenuItem *item = l->data;
		const char *item_id;

		item_id = g_object_get_data (G_OBJECT (item), "id");
		if (g_strcmp0 (item_id, id) == 0) {
			gtk_check_menu_item_set_active (item, TRUE);
			return;
		}
	}

	g_warning ("Could not find ID '%s' in TotemSearchEntry %p",
		   id, self);
}
