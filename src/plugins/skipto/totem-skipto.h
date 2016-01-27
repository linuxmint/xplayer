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

#ifndef XPLAYER_SKIPTO_H
#define XPLAYER_SKIPTO_H

#include <gtk/gtk.h>

#include "xplayer.h"

G_BEGIN_DECLS

#define XPLAYER_TYPE_SKIPTO		(xplayer_skipto_get_type ())
#define XPLAYER_SKIPTO(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), XPLAYER_TYPE_SKIPTO, XplayerSkipto))
#define XPLAYER_SKIPTO_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), XPLAYER_TYPE_SKIPTO, XplayerSkiptoClass))
#define XPLAYER_IS_SKIPTO(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XPLAYER_TYPE_SKIPTO))
#define XPLAYER_IS_SKIPTO_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XPLAYER_TYPE_SKIPTO))

GType xplayer_skipto_register_type	(GTypeModule *module);

typedef struct XplayerSkipto		XplayerSkipto;
typedef struct XplayerSkiptoClass		XplayerSkiptoClass;
typedef struct XplayerSkiptoPrivate	XplayerSkiptoPrivate;

struct XplayerSkipto {
	GtkDialog parent;
	XplayerSkiptoPrivate *priv;
};

struct XplayerSkiptoClass {
	GtkDialogClass parent_class;
};

GType xplayer_skipto_get_type	(void);
GtkWidget *xplayer_skipto_new	(XplayerObject *xplayer);
gint64 xplayer_skipto_get_range	(XplayerSkipto *skipto);
void xplayer_skipto_update_range	(XplayerSkipto *skipto, gint64 _time);
void xplayer_skipto_set_seekable	(XplayerSkipto *skipto, gboolean seekable);
void xplayer_skipto_set_current	(XplayerSkipto *skipto, gint64 _time);

G_END_DECLS

#endif /* XPLAYER_SKIPTO_H */
