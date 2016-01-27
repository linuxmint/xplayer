/*
 * Copyright (c) 2011 Red Hat, Inc.
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
 * Author: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#ifndef __TOTEM_SEARCH_ENTRY_H__
#define __TOTEM_SEARCH_ENTRY_H__

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_SEARCH_ENTRY totem_search_entry_get_type()
#define TOTEM_SEARCH_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_SEARCH_ENTRY, TotemSearchEntry))
#define TOTEM_SEARCH_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_SEARCH_ENTRY, TotemSearchEntryClass))
#define TOTEM_IS_SEARCH_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_SEARCH_ENTRY))
#define TOTEM_IS_SEARCH_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_SEARCH_ENTRY))
#define TOTEM_SEARCH_ENTRY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TOTEM_TYPE_SEARCH_ENTRY, TotemSearchEntryClass))

typedef struct _TotemSearchEntry TotemSearchEntry;
typedef struct _TotemSearchEntryClass TotemSearchEntryClass;
typedef struct _TotemSearchEntryPrivate TotemSearchEntryPrivate;

struct _TotemSearchEntry
{
	GtkBox parent;

	TotemSearchEntryPrivate *priv;
};

struct _TotemSearchEntryClass
{
	GtkBoxClass parent_class;
};

GType totem_search_entry_get_type (void) G_GNUC_CONST;

TotemSearchEntry *totem_search_entry_new (void);

void totem_search_entry_add_source (TotemSearchEntry *entry,
                                  const gchar *id,
                                  const gchar *label,
                                  int priority);

void totem_search_entry_remove_source (TotemSearchEntry *self,
                                       const gchar *id);

const char *totem_search_entry_get_text (TotemSearchEntry *self);

const char *totem_search_entry_get_selected_id (TotemSearchEntry *self);
void        totem_search_entry_set_selected_id (TotemSearchEntry *self,
						const char       *id);

G_END_DECLS

#endif /* __TOTEM_SEARCH_ENTRY_H__ */
