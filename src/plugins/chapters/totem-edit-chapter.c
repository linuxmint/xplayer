/*
 * Copyright (C) 2010 Alexander Saprykin <xelfium@gmail.com>
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
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 */

/*
 * SECTION:totem-edit-chapter
 * @short_description: dialog to add new chapters in chapters plugin
 * @stability: Unstable
 * @include: totem-edit-chapter.h
 *
 * #TotemEditChapter dialog is used for entering names of new chapters in the chapters plugin
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "totem.h"
#include "totem-interface.h"
#include "totem-dirs.h"
#include "totem-edit-chapter.h"
#include <string.h>

struct TotemEditChapterPrivate {
	GtkEntry	*title_entry;
	GtkWidget	*container;
};


#define TOTEM_EDIT_CHAPTER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TOTEM_TYPE_EDIT_CHAPTER, TotemEditChapterPrivate))

G_DEFINE_TYPE (TotemEditChapter, totem_edit_chapter, GTK_TYPE_DIALOG)

/* GtkBuilder callbacks */
void title_entry_changed_cb (GtkEditable *entry, gpointer user_data);

static void
totem_edit_chapter_class_init (TotemEditChapterClass *klass)
{
	g_type_class_add_private (klass, sizeof (TotemEditChapterPrivate));
}

static void
totem_edit_chapter_init (TotemEditChapter *self)
{
	GtkBuilder	*builder;

	self->priv = TOTEM_EDIT_CHAPTER_GET_PRIVATE (self);
	builder = totem_plugin_load_interface ("chapters", "chapters-edit.ui", FALSE, NULL, self);

	if (builder == NULL) {
		self->priv->container = NULL;
		return;
	}

	self->priv->container = GTK_WIDGET (gtk_builder_get_object (builder, "main_vbox"));
	g_object_ref (self->priv->container);
	self->priv->title_entry = GTK_ENTRY (gtk_builder_get_object (builder, "title_entry"));

	g_object_unref (builder);
}

GtkWidget*
totem_edit_chapter_new (void)
{
	TotemEditChapter	*edit_chapter;
	GtkWidget		*dialog_area;

	edit_chapter = TOTEM_EDIT_CHAPTER (g_object_new (TOTEM_TYPE_EDIT_CHAPTER, NULL));

	if (G_UNLIKELY (edit_chapter->priv->container == NULL)) {
		g_object_unref (edit_chapter);
		return NULL;
	}

	gtk_window_set_title (GTK_WINDOW (edit_chapter), _("Add Chapter"));

	gtk_dialog_add_buttons (GTK_DIALOG (edit_chapter),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);
	gtk_container_set_border_width (GTK_CONTAINER (edit_chapter), 5);
	gtk_dialog_set_default_response (GTK_DIALOG (edit_chapter), GTK_RESPONSE_OK);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (edit_chapter), GTK_RESPONSE_OK, FALSE);

	dialog_area = gtk_dialog_get_content_area (GTK_DIALOG (edit_chapter));
	gtk_box_pack_start (GTK_BOX (dialog_area),
			    edit_chapter->priv->container,
			    FALSE, TRUE, 0);

	gtk_widget_show_all (dialog_area);

	return GTK_WIDGET (edit_chapter);
}

void
totem_edit_chapter_set_title (TotemEditChapter	*edit_chapter,
			      const gchar	*title)
{
	g_return_if_fail (TOTEM_IS_EDIT_CHAPTER (edit_chapter));

	gtk_entry_set_text (edit_chapter->priv->title_entry, title);
}

gchar *
totem_edit_chapter_get_title (TotemEditChapter *edit_chapter)
{
	g_return_val_if_fail (TOTEM_IS_EDIT_CHAPTER (edit_chapter), NULL);

	return g_strdup (gtk_entry_get_text (edit_chapter->priv->title_entry));
}

void
title_entry_changed_cb (GtkEditable	*entry,
			gpointer	user_data)
{
	GtkDialog	*dialog;
	gboolean	sens;

	g_return_if_fail (GTK_IS_ENTRY (entry));
	g_return_if_fail (GTK_IS_DIALOG (user_data));

	dialog = GTK_DIALOG (user_data);
	sens = (gtk_entry_get_text_length (GTK_ENTRY (entry)) > 0);

	gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_OK, sens);
}
