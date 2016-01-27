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
 * The Xplayer project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Xplayer. This
 * permission are above and beyond the permissions granted by the GPL license
 * Xplayer is covered by.
 */

#ifndef XPLAYER_EDIT_CHAPTER_H
#define XPLAYER_EDIT_CHAPTER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XPLAYER_TYPE_EDIT_CHAPTER			(xplayer_edit_chapter_get_type ())
#define XPLAYER_EDIT_CHAPTER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), XPLAYER_TYPE_EDIT_CHAPTER, XplayerEditChapter))
#define XPLAYER_EDIT_CHAPTER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XPLAYER_TYPE_EDIT_CHAPTER, XplayerEditChapterClass))
#define XPLAYER_IS_EDIT_CHAPTER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XPLAYER_TYPE_EDIT_CHAPTER))
#define XPLAYER_IS_EDIT_CHAPTER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XPLAYER_TYPE_EDIT_CHAPTER))

typedef struct XplayerEditChapter			XplayerEditChapter;
typedef struct XplayerEditChapterClass		XplayerEditChapterClass;
typedef struct XplayerEditChapterPrivate		XplayerEditChapterPrivate;

struct XplayerEditChapter {
	GtkDialog parent;
	XplayerEditChapterPrivate *priv;
};

struct XplayerEditChapterClass {
	GtkDialogClass parent_class;
};

GType xplayer_edit_chapter_get_type (void);
GtkWidget * xplayer_edit_chapter_new (void);
void xplayer_edit_chapter_set_title (XplayerEditChapter *edit_chapter, const gchar *title);
gchar * xplayer_edit_chapter_get_title (XplayerEditChapter *edit_chapter);

G_END_DECLS

#endif /* XPLAYER_EDIT_CHAPTER_H */
