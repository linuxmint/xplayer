/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001-2007 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2007 Sunil Mohan Adapa <sunilmohan@gnu.org.in>
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
 */

#include <config.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "xplayer-fullscreen.h"
#include "xplayer-interface.h"
#include "xplayer-time-label.h"
#include "bacon-video-widget.h"
#include "gd-fullscreen-filter.h"

#define FULLSCREEN_POPUP_TIMEOUT 5
#define FULLSCREEN_MOTION_TIME 200 /* in milliseconds */
#define FULLSCREEN_MOTION_NUM_EVENTS 15

static void xplayer_fullscreen_dispose (GObject *object);
static void xplayer_fullscreen_finalize (GObject *object);
static gboolean xplayer_fullscreen_popup_hide (XplayerFullscreen *fs);

/* Callback functions for GtkBuilder */
G_MODULE_EXPORT gboolean xplayer_fullscreen_vol_slider_pressed_cb (GtkWidget *widget, GdkEventButton *event, XplayerFullscreen *fs);
G_MODULE_EXPORT gboolean xplayer_fullscreen_vol_slider_released_cb (GtkWidget *widget, GdkEventButton *event, XplayerFullscreen *fs);
G_MODULE_EXPORT gboolean xplayer_fullscreen_seek_slider_pressed_cb (GtkWidget *widget, GdkEventButton *event, XplayerFullscreen *fs);
G_MODULE_EXPORT gboolean xplayer_fullscreen_seek_slider_released_cb (GtkWidget *widget, GdkEventButton *event, XplayerFullscreen *fs);
G_MODULE_EXPORT gboolean xplayer_fullscreen_control_enter_notify (GtkWidget *widget, GdkEventCrossing *event, XplayerFullscreen *fs);
G_MODULE_EXPORT gboolean xplayer_fullscreen_control_leave_notify (GtkWidget *widget, GdkEventCrossing *event, XplayerFullscreen *fs);


struct _XplayerFullscreenPrivate {
	BaconVideoWidget *bvw;
	GtkWidget        *parent_window;

	/* Fullscreen Popups */
	GtkWidget        *exit_popup;
	GtkWidget        *control_popup;

	/* Locks for keeping the popups during adjustments */
	gboolean          seek_lock;

	guint             popup_timeout;
	gboolean          popup_in_progress;
	gboolean          pointer_on_control;
	GdFullscreenFilter *filter;
	gint64             motion_start_time;
	guint              motion_num_events;

	gboolean          is_fullscreen;

	GtkBuilder       *xml;
};

#define XPLAYER_FULLSCREEN_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XPLAYER_TYPE_FULLSCREEN, XplayerFullscreenPrivate))

G_DEFINE_TYPE (XplayerFullscreen, xplayer_fullscreen, G_TYPE_OBJECT)

gboolean
xplayer_fullscreen_is_fullscreen (XplayerFullscreen *fs)
{
	g_return_val_if_fail (XPLAYER_IS_FULLSCREEN (fs), FALSE);

	return (fs->priv->is_fullscreen != FALSE);
}

static void
xplayer_fullscreen_move_popups (XplayerFullscreen *fs)
{
	int exit_width,    exit_height;
	int control_width, control_height;

	GdkScreen              *screen;
	GdkRectangle            fullscreen_rect;
	GdkWindow              *window;
	XplayerFullscreenPrivate *priv = fs->priv;

	g_return_if_fail (priv->parent_window != NULL);

	/* Obtain the screen rectangle */
	screen = gtk_window_get_screen (GTK_WINDOW (priv->parent_window));
	window = gtk_widget_get_window (priv->parent_window);
	gdk_screen_get_monitor_geometry (screen,
					 gdk_screen_get_monitor_at_window (screen, window),
					 &fullscreen_rect);

	/* Get the popup window sizes */
	gtk_window_get_size (GTK_WINDOW (priv->exit_popup),
			     &exit_width, &exit_height);
	gtk_window_get_size (GTK_WINDOW (priv->control_popup),
			     &control_width, &control_height);

	/* We take the full width of the screen */
	gtk_window_resize (GTK_WINDOW (priv->control_popup),
			   fullscreen_rect.width, control_height);

	if (gtk_widget_get_direction (priv->exit_popup) == GTK_TEXT_DIR_RTL) {
		gtk_window_move (GTK_WINDOW (priv->exit_popup),
				 fullscreen_rect.x,
				 fullscreen_rect.y);
		gtk_window_move (GTK_WINDOW (priv->control_popup),
				 fullscreen_rect.width - control_width,
				 fullscreen_rect.height + fullscreen_rect.y -
				 control_height);
	} else {
		gtk_window_move (GTK_WINDOW (priv->exit_popup),
				 fullscreen_rect.width + fullscreen_rect.x - exit_width,
				 fullscreen_rect.y);
		gtk_window_move (GTK_WINDOW (priv->control_popup),
				 fullscreen_rect.x,
				 fullscreen_rect.height + fullscreen_rect.y -
				 control_height);
	}
}

static void
xplayer_fullscreen_size_changed_cb (GdkScreen *screen, XplayerFullscreen *fs)
{
	xplayer_fullscreen_move_popups (fs);
}

static void
xplayer_fullscreen_theme_changed_cb (GtkIconTheme *icon_theme, XplayerFullscreen *fs)
{
	xplayer_fullscreen_move_popups (fs);
}

static void
xplayer_fullscreen_window_realize_cb (GtkWidget *widget, XplayerFullscreen *fs)
{
	GdkScreen *screen;

	screen = gtk_widget_get_screen (widget);
	g_signal_connect (G_OBJECT (screen), "size-changed",
			  G_CALLBACK (xplayer_fullscreen_size_changed_cb), fs);
	g_signal_connect (G_OBJECT (gtk_icon_theme_get_for_screen (screen)),
			  "changed",
			  G_CALLBACK (xplayer_fullscreen_theme_changed_cb), fs);
}

static void
xplayer_fullscreen_window_unrealize_cb (GtkWidget *widget, XplayerFullscreen *fs)
{
	GdkScreen *screen;

	screen = gtk_widget_get_screen (widget);
	g_signal_handlers_disconnect_by_func (screen,
					      G_CALLBACK (xplayer_fullscreen_size_changed_cb), fs);
	g_signal_handlers_disconnect_by_func (gtk_icon_theme_get_for_screen (screen),
					      G_CALLBACK (xplayer_fullscreen_theme_changed_cb), fs);
}

static gboolean
xplayer_fullscreen_exit_popup_draw_cb (GtkWidget *widget,
				     cairo_t *cr,
				     XplayerFullscreen *fs)
{
	GdkScreen *screen;

	screen = gtk_widget_get_screen (widget);
	if (gdk_screen_is_composited (screen) == FALSE)
		return FALSE;

	gtk_widget_set_app_paintable (widget, TRUE);

	cairo_set_source_rgba (cr, 1., 1., 1., 0.);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);

	return FALSE;
}

gboolean
xplayer_fullscreen_seek_slider_pressed_cb (GtkWidget *widget,
					 GdkEventButton *event,
					 XplayerFullscreen *fs)
{
	fs->priv->seek_lock = TRUE;
	return FALSE;
}

gboolean
xplayer_fullscreen_seek_slider_released_cb (GtkWidget *widget,
					  GdkEventButton *event,
					  XplayerFullscreen *fs)
{
	fs->priv->seek_lock = FALSE;
	return FALSE;
}

static void
xplayer_fullscreen_popup_timeout_add (XplayerFullscreen *fs)
{
	fs->priv->popup_timeout = g_timeout_add_seconds (FULLSCREEN_POPUP_TIMEOUT,
							 (GSourceFunc) xplayer_fullscreen_popup_hide, fs);
}

static void
xplayer_fullscreen_popup_timeout_remove (XplayerFullscreen *fs)
{
	if (fs->priv->popup_timeout != 0) {
		g_source_remove (fs->priv->popup_timeout);
		fs->priv->popup_timeout = 0;
	}
}

static void
xplayer_fullscreen_set_cursor (XplayerFullscreen *fs, gboolean state)
{
	if (fs->priv->bvw != NULL)
		bacon_video_widget_set_show_cursor (fs->priv->bvw, state);
}

static gboolean
xplayer_fullscreen_is_volume_popup_visible (XplayerFullscreen *fs)
{
	return gtk_widget_get_visible (gtk_scale_button_get_popup (GTK_SCALE_BUTTON (fs->volume)));
}

static void
xplayer_fullscreen_force_popup_hide (XplayerFullscreen *fs)
{
	/* Popdown the volume button if it's visible */
	if (xplayer_fullscreen_is_volume_popup_visible (fs))
		gtk_bindings_activate (G_OBJECT (fs->volume), GDK_KEY_Escape, 0);

	gtk_widget_hide (fs->priv->exit_popup);
	gtk_widget_hide (fs->priv->control_popup);

	xplayer_fullscreen_popup_timeout_remove (fs);

	xplayer_fullscreen_set_cursor (fs, FALSE);
}

static gboolean
xplayer_fullscreen_popup_hide (XplayerFullscreen *fs)
{
	if (fs->priv->bvw == NULL || xplayer_fullscreen_is_fullscreen (fs) == FALSE)
		return TRUE;

	if (fs->priv->seek_lock != FALSE || xplayer_fullscreen_is_volume_popup_visible (fs) != FALSE)
		return TRUE;

	xplayer_fullscreen_force_popup_hide (fs);

	return FALSE;
}

static void
xplayer_fullscreen_motion_notify (GdFullscreenFilter *filter,
				XplayerFullscreen    *fs)
{
	gint64 motion_delay;
	gint64 curr;

	curr = g_get_monotonic_time ();
	/* Only after FULLSCREEN_MOTION_NUM_EVENTS motion events,
	   in FULLSCREEN_MOTION_TIME milliseconds will we show
	   the popups */
	motion_delay = (curr - fs->priv->motion_start_time) / 1000;

	if (fs->priv->motion_start_time == 0 ||
	    motion_delay < 0 ||
	    motion_delay > FULLSCREEN_MOTION_TIME) {
		fs->priv->motion_start_time = curr;
		fs->priv->motion_num_events = 0;
		return;
	}

	fs->priv->motion_num_events++;

	if (!fs->priv->pointer_on_control &&
	    fs->priv->motion_num_events > FULLSCREEN_MOTION_NUM_EVENTS) {
		xplayer_fullscreen_show_popups (fs, TRUE);
	}
}

void
xplayer_fullscreen_show_popups (XplayerFullscreen *fs, gboolean show_cursor)
{
	GtkWidget *item;

	if (!fs->priv->is_fullscreen)
		return;

	if (fs->priv->popup_in_progress != FALSE)
		return;

	if (gtk_window_is_active (GTK_WINDOW (fs->priv->parent_window)) == FALSE)
		return;

	fs->priv->popup_in_progress = TRUE;

	xplayer_fullscreen_popup_timeout_remove (fs);

	/* FIXME: is this really required while we are anyway going
	   to do a show_all on its parent control_popup? */
	item = GTK_WIDGET (gtk_builder_get_object (fs->priv->xml, "tcw_hbox"));
	gtk_widget_show_all (item);
	gdk_flush ();

	/* Show the popup widgets */
	xplayer_fullscreen_move_popups (fs);
	gtk_widget_show (fs->priv->exit_popup);
	gtk_widget_show_all (fs->priv->control_popup);

	if (show_cursor != FALSE) {
		/* Show the mouse cursor */
		xplayer_fullscreen_set_cursor (fs, TRUE);
	}

	/* Reset the popup timeout */
	xplayer_fullscreen_popup_timeout_add (fs);

	fs->priv->popup_in_progress = FALSE;
}

void
xplayer_fullscreen_show_popups_or_osd (XplayerFullscreen *fs,
				     const char *icon_name,
				     gboolean show_cursor)
{
	if (icon_name == NULL) {
		xplayer_fullscreen_show_popups (fs, show_cursor);
		return;
	}

	//bacon_video_widget_popup_osd (fs->priv->bvw, icon_name);
}

G_MODULE_EXPORT gboolean
xplayer_fullscreen_control_enter_notify (GtkWidget *widget,
			       GdkEventCrossing *event,
			       XplayerFullscreen *fs)
{
	fs->priv->pointer_on_control = TRUE;
	xplayer_fullscreen_popup_timeout_remove (fs);
	return TRUE;
}

G_MODULE_EXPORT gboolean
xplayer_fullscreen_control_leave_notify (GtkWidget *widget,
			       GdkEventCrossing *event,
			       XplayerFullscreen *fs)
{
	fs->priv->pointer_on_control = FALSE;
	return TRUE;
}

void
xplayer_fullscreen_set_fullscreen (XplayerFullscreen *fs,
				 gboolean fullscreen)
{
	g_return_if_fail (XPLAYER_IS_FULLSCREEN (fs));

	xplayer_fullscreen_force_popup_hide (fs);

	bacon_video_widget_set_fullscreen (fs->priv->bvw, fullscreen);
	xplayer_fullscreen_set_cursor (fs, !fullscreen);

	fs->priv->is_fullscreen = fullscreen;

	if (fullscreen == FALSE) {
		gd_fullscreen_filter_stop (fs->priv->filter);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(fs->blank_button), FALSE);
		xapp_monitor_blanker_unblank_monitors(fs->xapp_monitor_blanker);
	}
	else {
		gd_fullscreen_filter_start (fs->priv->filter);
		GdkScreen *screen = gdk_screen_get_default ();
		if (gdk_screen_get_n_monitors (screen) > 1) {
			gtk_widget_show (fs->blank_button);
		}
		else {
			gtk_widget_hide (fs->blank_button);
		}
	}
}

static void
xplayer_fullscreen_parent_window_notify (GtkWidget *parent_window,
				       GParamSpec *property,
				       XplayerFullscreen *fs)
{
	GtkWidget *popup;

	if (xplayer_fullscreen_is_fullscreen (fs) == FALSE)
		return;

	popup = gtk_scale_button_get_popup (GTK_SCALE_BUTTON (fs->volume));
	if (parent_window == fs->priv->parent_window &&
	    gtk_window_is_active (GTK_WINDOW (parent_window)) == FALSE &&
	    gtk_widget_get_visible (popup) == FALSE) {
		xplayer_fullscreen_force_popup_hide (fs);
		xplayer_fullscreen_set_cursor (fs, TRUE);
	} else {
		xplayer_fullscreen_set_cursor (fs, FALSE);
	}
}

XplayerFullscreen *
xplayer_fullscreen_new (GtkWindow *toplevel_window)
{
        XplayerFullscreen *fs = XPLAYER_FULLSCREEN (g_object_new 
						(XPLAYER_TYPE_FULLSCREEN, NULL));

	if (fs->priv->xml == NULL) {
		g_object_unref (fs);
		return NULL;
	}

	xplayer_fullscreen_set_parent_window (fs, toplevel_window);

	fs->time_label = GTK_WIDGET (gtk_builder_get_object (fs->priv->xml,
				"tcw_time_display_label"));
	fs->buttons_box = GTK_WIDGET (gtk_builder_get_object (fs->priv->xml,
				"tcw_buttons_hbox"));
	fs->exit_button = GTK_WIDGET (gtk_builder_get_object (fs->priv->xml,
				"tefw_fs_exit_button"));
	fs->blank_button = GTK_WIDGET (gtk_builder_get_object (fs->priv->xml,
				"tefw_fs_blank_button"));

	fs->xapp_monitor_blanker = xapp_monitor_blanker_new();

	/* Volume */
	fs->volume = GTK_WIDGET (gtk_builder_get_object (fs->priv->xml, "tcw_volume_button"));

	/* Seek */
	fs->seek = GTK_WIDGET (gtk_builder_get_object (fs->priv->xml, "tcw_seek_hscale"));

	/* Motion notify */
	fs->priv->filter = gd_fullscreen_filter_new ();
	g_signal_connect (G_OBJECT (fs->priv->filter), "motion-event",
			  G_CALLBACK (xplayer_fullscreen_motion_notify), fs);
	gtk_widget_add_events (fs->seek, GDK_POINTER_MOTION_MASK);
	gtk_widget_add_events (fs->exit_button, GDK_POINTER_MOTION_MASK);
	gtk_widget_add_events (fs->blank_button, GDK_POINTER_MOTION_MASK);

	return fs;
}

void
xplayer_fullscreen_set_video_widget (XplayerFullscreen *fs,
				   BaconVideoWidget *bvw)
{
	g_return_if_fail (XPLAYER_IS_FULLSCREEN (fs));
	g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
	g_return_if_fail (fs->priv->bvw == NULL);

	fs->priv->bvw = bvw;
}

void
xplayer_fullscreen_set_parent_window (XplayerFullscreen *fs, GtkWindow *parent_window)
{
	g_return_if_fail (XPLAYER_IS_FULLSCREEN (fs));
	g_return_if_fail (GTK_IS_WINDOW (parent_window));
	g_return_if_fail (fs->priv->parent_window == NULL);

	fs->priv->parent_window = GTK_WIDGET (parent_window);

	/* Screen size and Theme changes */
	g_signal_connect (fs->priv->parent_window, "realize",
			  G_CALLBACK (xplayer_fullscreen_window_realize_cb), fs);
	g_signal_connect (fs->priv->parent_window, "unrealize",
			  G_CALLBACK (xplayer_fullscreen_window_unrealize_cb), fs);
	g_signal_connect (G_OBJECT (fs->priv->parent_window), "notify::is-active",
			  G_CALLBACK (xplayer_fullscreen_parent_window_notify), fs);
}

static void
xplayer_fullscreen_init (XplayerFullscreen *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, XPLAYER_TYPE_FULLSCREEN, XplayerFullscreenPrivate);

        self->priv->seek_lock = FALSE;
	self->priv->xml = xplayer_interface_load ("fullscreen.ui", TRUE, NULL, self);

	if (self->priv->xml == NULL)
		return;

	self->priv->pointer_on_control = FALSE;

	self->priv->exit_popup = GTK_WIDGET (gtk_builder_get_object (self->priv->xml,
				"xplayer_exit_fullscreen_window"));
	g_signal_connect (G_OBJECT (self->priv->exit_popup), "draw",
			  G_CALLBACK (xplayer_fullscreen_exit_popup_draw_cb), self);
	self->priv->control_popup = GTK_WIDGET (gtk_builder_get_object (self->priv->xml,
				"xplayer_controls_window"));

	/* Motion notify */
	gtk_widget_add_events (self->priv->exit_popup, GDK_POINTER_MOTION_MASK);
	gtk_widget_add_events (self->priv->control_popup, GDK_POINTER_MOTION_MASK);
}

static void
xplayer_fullscreen_dispose (GObject *object)
{
        XplayerFullscreenPrivate *priv = XPLAYER_FULLSCREEN_GET_PRIVATE (object);

	if (priv->xml != NULL) {
		g_object_unref (priv->xml);
		priv->xml = NULL;
		gtk_widget_destroy (priv->exit_popup);
		gtk_widget_destroy (priv->control_popup);
	}

	G_OBJECT_CLASS (xplayer_fullscreen_parent_class)->dispose (object);
}

static void
xplayer_fullscreen_finalize (GObject *object)
{
        XplayerFullscreen *fs = XPLAYER_FULLSCREEN (object);

	xplayer_fullscreen_popup_timeout_remove (fs);
	if (fs->priv->filter) {
		g_object_unref (fs->priv->filter);
		fs->priv->filter = NULL;
	}

	g_signal_handlers_disconnect_by_func (fs->priv->parent_window,
					      G_CALLBACK (xplayer_fullscreen_window_realize_cb),
					      fs);
	g_signal_handlers_disconnect_by_func (fs->priv->parent_window,
					      G_CALLBACK (xplayer_fullscreen_window_unrealize_cb),
					      fs);

	G_OBJECT_CLASS (xplayer_fullscreen_parent_class)->finalize (object);
}

static void
xplayer_fullscreen_class_init (XplayerFullscreenClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (XplayerFullscreenPrivate));

	object_class->dispose = xplayer_fullscreen_dispose;
        object_class->finalize = xplayer_fullscreen_finalize;
}

void
xplayer_fullscreen_set_title (XplayerFullscreen *fs, const char *title)
{
	GtkLabel *widget;
	char *text;

	g_return_if_fail (XPLAYER_IS_FULLSCREEN (fs));

	widget = GTK_LABEL (gtk_builder_get_object (fs->priv->xml, "tcw_title_label"));

	if (title != NULL) {
		char *escaped;

		escaped = g_markup_escape_text (title, -1);
		text = g_strdup_printf
			("<span size=\"medium\"><b>%s</b></span>", escaped);
		g_free (escaped);
	} else {
		text = g_strdup_printf
			("<span size=\"medium\"><b>%s</b></span>",
			 _("No File"));
	}

	gtk_label_set_markup (widget, text);
	g_free (text);
}

void
xplayer_fullscreen_set_seekable (XplayerFullscreen *fs, gboolean seekable)
{
	GtkWidget *item;

	g_return_if_fail (XPLAYER_IS_FULLSCREEN (fs));

	item = GTK_WIDGET (gtk_builder_get_object (fs->priv->xml, "tcw_time_hbox"));
	gtk_widget_set_sensitive (item, seekable);

	gtk_widget_set_sensitive (fs->seek, seekable);
}

void
xplayer_fullscreen_set_can_set_volume (XplayerFullscreen *fs, gboolean can_set_volume)
{
	g_return_if_fail (XPLAYER_IS_FULLSCREEN (fs));

	gtk_widget_set_sensitive (fs->volume, can_set_volume);
}

void
xplayer_fullscreen_toggle_blank_monitors (XplayerFullscreen *fs, GtkWidget *window)
{
	if (xapp_monitor_blanker_are_monitors_blanked(fs->xapp_monitor_blanker)) {
		xapp_monitor_blanker_unblank_monitors(fs->xapp_monitor_blanker);
	}
	else {
		xapp_monitor_blanker_blank_other_monitors(fs->xapp_monitor_blanker, window);
	}
	xplayer_fullscreen_move_popups (fs);
}