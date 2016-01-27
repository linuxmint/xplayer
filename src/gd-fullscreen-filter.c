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

#include "gd-fullscreen-filter.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <X11/extensions/XInput2.h>

G_DEFINE_TYPE (GdFullscreenFilter, gd_fullscreen_filter, G_TYPE_OBJECT)

enum {
  MOTION_EVENT = 1,
  NUM_SIGNALS
};

static guint signals[NUM_SIGNALS] = { 0, };

struct _GdFullscreenFilterPrivate {
  gboolean is_filtering;
};

static void
gd_fullscreen_filter_dispose (GObject *object)
{
  GdFullscreenFilter *self = GD_FULLSCREEN_FILTER (object);

  gd_fullscreen_filter_stop (self);

  G_OBJECT_CLASS (gd_fullscreen_filter_parent_class)->dispose (object);
}

static void
gd_fullscreen_filter_init (GdFullscreenFilter *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GD_TYPE_FULLSCREEN_FILTER,
                                            GdFullscreenFilterPrivate);
}

static void
gd_fullscreen_filter_class_init (GdFullscreenFilterClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->dispose = gd_fullscreen_filter_dispose;

  signals[MOTION_EVENT] =
    g_signal_new ("motion-event",
                  GD_TYPE_FULLSCREEN_FILTER,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (GdFullscreenFilterPrivate));
}

static GdkFilterReturn
event_filter_func (GdkXEvent *gdk_xevent,
                   GdkEvent *event,
                   gpointer user_data)
{
  GdFullscreenFilter *self = user_data;
  XEvent *xevent = (XEvent *) gdk_xevent;

  if (xevent->xany.type == ButtonPress ||
      xevent->xany.type == ButtonRelease ||
      xevent->xany.type == MotionNotify)
    {
      g_signal_emit (self, signals[MOTION_EVENT], 0);
    }
  else if (xevent->xany.type == GenericEvent)
    {
        /* we just assume this is an XI2 event */
        XIEvent *ev = (XIEvent *) xevent->xcookie.data;

        if (ev->evtype == XI_Motion ||
            ev->evtype == XI_ButtonRelease ||
            ev->evtype == XI_ButtonPress)
          {
            g_signal_emit (self, signals[MOTION_EVENT], 0);
          }
    }

  return GDK_FILTER_CONTINUE;
}

void
gd_fullscreen_filter_start (GdFullscreenFilter *self)
{
  if (self->priv->is_filtering)
    return;

  self->priv->is_filtering = TRUE;
  gdk_window_add_filter (NULL,
                         event_filter_func, self);
}

void
gd_fullscreen_filter_stop (GdFullscreenFilter *self)
{
  if (!self->priv->is_filtering)
    return;

  self->priv->is_filtering = FALSE;
  gdk_window_remove_filter (NULL,
                            event_filter_func, self);
}

GdFullscreenFilter *
gd_fullscreen_filter_new (void)
{
  return g_object_new (GD_TYPE_FULLSCREEN_FILTER, NULL);
}
