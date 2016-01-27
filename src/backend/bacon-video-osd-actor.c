/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * On-screen-display (OSD) for Totem's video widget
 *
 * Copyright (C) 2012 Red Hat, Inc.
 *
 * Authors:
 *   Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <gtk/gtk.h>

#include "bacon-video-osd-actor.h"
#include "gsd-osd-window.h"
#include "gsd-osd-window-private.h"

#define BACON_VIDEO_OSD_ACTOR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BACON_TYPE_VIDEO_OSD_ACTOR, BaconVideoOsdActorPrivate))

struct BaconVideoOsdActorPrivate
{
	ClutterCanvas     *canvas;
	char              *icon_name;
	GtkStyleContext   *style;
	GsdOsdDrawContext *ctx;

        guint              hide_timeout_id;
        guint              fade_timeout_id;
        double             fade_out_alpha;
};

G_DEFINE_TYPE (BaconVideoOsdActor, bacon_video_osd_actor, CLUTTER_TYPE_ACTOR);

static gboolean
fade_timeout (BaconVideoOsdActor *osd)
{
        if (osd->priv->fade_out_alpha <= 0.0) {
		bacon_video_osd_actor_hide (osd);
                return FALSE;
        } else {
                clutter_actor_set_opacity (CLUTTER_ACTOR (osd),
                			   0xff * osd->priv->fade_out_alpha);
                osd->priv->fade_out_alpha -= 0.10;
        }

        return TRUE;
}

static gboolean
hide_timeout (BaconVideoOsdActor *osd)
{
	osd->priv->hide_timeout_id = 0;
	osd->priv->fade_timeout_id = g_timeout_add (FADE_FRAME_TIMEOUT,
						    (GSourceFunc) fade_timeout,
						    osd);

        return FALSE;
}

static void
remove_hide_timeout (BaconVideoOsdActor *osd)
{
        if (osd->priv->hide_timeout_id != 0) {
                g_source_remove (osd->priv->hide_timeout_id);
                osd->priv->hide_timeout_id = 0;
        }

        if (osd->priv->fade_timeout_id != 0) {
                g_source_remove (osd->priv->fade_timeout_id);
                osd->priv->fade_timeout_id = 0;
                osd->priv->fade_out_alpha = 1.0;
        }
}

static void
add_hide_timeout (BaconVideoOsdActor *osd)
{
        osd->priv->hide_timeout_id = g_timeout_add (DIALOG_FADE_TIMEOUT,
                                                    (GSourceFunc) hide_timeout,
                                                    osd);
}

static void
bacon_video_osd_actor_finalize (GObject *object)
{
	BaconVideoOsdActor *osd;

	osd = BACON_VIDEO_OSD_ACTOR (object);
	if (osd->priv->ctx) {
		g_free (osd->priv->ctx);
		osd->priv->ctx = NULL;
	}
	if (osd->priv->style) {
		g_object_unref (osd->priv->style);
		osd->priv->style = NULL;
	}

	g_free (osd->priv->icon_name);
	osd->priv->icon_name = NULL;

	G_OBJECT_CLASS (bacon_video_osd_actor_parent_class)->finalize (object);
}

static void
bacon_video_osd_actor_class_init (BaconVideoOsdActorClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

//        gobject_class->constructor = bacon_video_osd_actor_constructor;
        gobject_class->finalize = bacon_video_osd_actor_finalize;

        g_type_class_add_private (klass, sizeof (BaconVideoOsdActorPrivate));
}

static gboolean
bacon_video_osd_actor_draw (ClutterCanvas      *canvas,
			    cairo_t            *cr,
			    int                 width,
			    int                 height,
			    BaconVideoOsdActor *osd)
{
	GsdOsdDrawContext *ctx;

	g_return_val_if_fail (osd->priv->icon_name != NULL, FALSE);

	ctx = osd->priv->ctx;

	cairo_save (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);

	cairo_restore (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	ctx->size = MIN(width, height);
	ctx->icon_name = osd->priv->icon_name;

	gsd_osd_window_draw (ctx, cr);

	return FALSE;
}

static void
bacon_video_osd_actor_init (BaconVideoOsdActor *osd)
{
	ClutterActor *self;
	GtkWidgetPath *widget_path;

	self = CLUTTER_ACTOR (osd);
        osd->priv = BACON_VIDEO_OSD_ACTOR_GET_PRIVATE (osd);

	osd->priv->canvas = CLUTTER_CANVAS (clutter_canvas_new ());
	g_object_bind_property (self, "width",
				osd->priv->canvas, "width",
				G_BINDING_DEFAULT);
	g_object_bind_property (self, "height",
				osd->priv->canvas, "height",
				G_BINDING_DEFAULT);
	clutter_actor_set_content (self, CLUTTER_CONTENT (osd->priv->canvas));
	g_object_unref (osd->priv->canvas);

	osd->priv->icon_name = g_strdup ("media-playback-pause-symbolic");

	osd->priv->ctx = g_new0 (GsdOsdDrawContext, 1);

	widget_path = gtk_widget_path_new ();
	gtk_widget_path_append_type (widget_path, GTK_TYPE_WINDOW);
	osd->priv->style = gtk_style_context_new ();
	gtk_style_context_set_path (osd->priv->style, widget_path);
	gtk_widget_path_free (widget_path);

	osd->priv->ctx->direction = clutter_get_default_text_direction ();
	osd->priv->ctx->theme = gtk_icon_theme_get_default ();
	osd->priv->ctx->action = GSD_OSD_WINDOW_ACTION_CUSTOM;
	osd->priv->ctx->style = osd->priv->style;

	g_signal_connect (osd->priv->canvas, "draw", G_CALLBACK (bacon_video_osd_actor_draw), osd);
        osd->priv->fade_out_alpha = 1.0;
}

ClutterActor *
bacon_video_osd_actor_new (void)
{
        return g_object_new (BACON_TYPE_VIDEO_OSD_ACTOR, NULL);
}

void
bacon_video_osd_actor_set_icon_name (BaconVideoOsdActor *osd,
				     const char         *icon_name)
{
	g_return_if_fail (BACON_IS_VIDEO_OSD_ACTOR (osd));

	g_free (osd->priv->icon_name);
	osd->priv->icon_name = g_strdup (icon_name);

	if (icon_name != NULL)
		clutter_content_invalidate (CLUTTER_CONTENT (osd->priv->canvas));
}

void
bacon_video_osd_actor_hide (BaconVideoOsdActor *osd)
{
	g_return_if_fail (BACON_IS_VIDEO_OSD_ACTOR (osd));

	clutter_actor_hide (CLUTTER_ACTOR (osd));

	/* Reset it for the next time */
	osd->priv->fade_out_alpha = 1.0;
	osd->priv->fade_timeout_id = 0;
}

void
bacon_video_osd_actor_show (BaconVideoOsdActor *osd)
{
	g_return_if_fail (BACON_IS_VIDEO_OSD_ACTOR (osd));

	remove_hide_timeout (osd);
	clutter_actor_set_opacity (CLUTTER_ACTOR (osd), 0xff);
	clutter_actor_show (CLUTTER_ACTOR (osd));
}

void
bacon_video_osd_actor_show_and_fade (BaconVideoOsdActor *osd)
{
	g_return_if_fail (BACON_IS_VIDEO_OSD_ACTOR (osd));

	remove_hide_timeout (osd);
	clutter_actor_set_opacity (CLUTTER_ACTOR (osd), 0xff);
	clutter_actor_show (CLUTTER_ACTOR (osd));
	add_hide_timeout (osd);
}
