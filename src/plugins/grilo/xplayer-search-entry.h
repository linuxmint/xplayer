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

#ifndef __XPLAYER_SEARCH_ENTRY_H__
#define __XPLAYER_SEARCH_ENTRY_H__

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XPLAYER_TYPE_SEARCH_ENTRY xplayer_search_entry_get_type()
#define XPLAYER_SEARCH_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), XPLAYER_TYPE_SEARCH_ENTRY, XplayerSearchEntry))
#define XPLAYER_SEARCH_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), XPLAYER_TYPE_SEARCH_ENTRY, XplayerSearchEntryClass))
#define XPLAYER_IS_SEARCH_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XPLAYER_TYPE_SEARCH_ENTRY))
#define XPLAYER_IS_SEARCH_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XPLAYER_TYPE_SEARCH_ENTRY))
#define XPLAYER_SEARCH_ENTRY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), XPLAYER_TYPE_SEARCH_ENTRY, XplayerSearchEntryClass))

typedef struct _XplayerSearchEntry XplayerSearchEntry;
typedef struct _XplayerSearchEntryClass XplayerSearchEntryClass;
typedef struct _XplayerSearchEntryPrivate XplayerSearchEntryPrivate;

struct _XplayerSearchEntry
{
	GtkBox parent;

	XplayerSearchEntryPrivate *priv;
};

struct _XplayerSearchEntryClass
{
	GtkBoxClass parent_class;
};

GType xplayer_search_entry_get_type (void) G_GNUC_CONST;

XplayerSearchEntry *xplayer_search_entry_new (void);

void xplayer_search_entry_add_source (XplayerSearchEntry *entry,
                                  const gchar *id,
                                  const gchar *label,
                                  int priority);

void xplayer_search_entry_remove_source (XplayerSearchEntry *self,
                                       const gchar *id);

const char *xplayer_search_entry_get_text (XplayerSearchEntry *self);

const char *xplayer_search_entry_get_selected_id (XplayerSearchEntry *self);
void        xplayer_search_entry_set_selected_id (XplayerSearchEntry *self,
						const char       *id);

G_END_DECLS

#endif /* __XPLAYER_SEARCH_ENTRY_H__ */
