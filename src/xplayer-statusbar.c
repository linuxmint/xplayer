/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * XplayerStatusbar Copyright (C) 1998 Shawn T. Amundson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "config.h"

#include <math.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "xplayer-statusbar.h"
#include "xplayer-time-helpers.h"

#define SPACING 4
#define NORMAL_CONTEXT "text"
#define BUFFERING_CONTEXT "buffering"
#define HELP_CONTEXT "help"

static void xplayer_statusbar_finalize         (GObject             *object);
static void xplayer_statusbar_sync_description (XplayerStatusbar      *statusbar);

struct _XplayerStatusbarPrivate {
  GtkWidget *progress;
  GtkWidget *time_label;

  gint time;
  gint length;
  guint timeout;
  gdouble percentage;

  guint pushed : 1;
  guint seeking : 1;
  guint timeout_ticks : 2;
};

G_DEFINE_TYPE(XplayerStatusbar, xplayer_statusbar, GTK_TYPE_STATUSBAR)

static void
xplayer_statusbar_class_init (XplayerStatusbarClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (XplayerStatusbarPrivate));

  gobject_class->finalize = xplayer_statusbar_finalize;
}

static void
xplayer_statusbar_init (XplayerStatusbar *statusbar)
{
  XplayerStatusbarPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (statusbar, XPLAYER_TYPE_STATUSBAR, XplayerStatusbarPrivate);
  GtkStatusbar *gstatusbar = GTK_STATUSBAR (statusbar);
  GtkWidget *packer, *hbox, *vbox, *label;
  GList *children_list;

  statusbar->priv = priv;

  priv->time = 0;
  priv->length = -1;

  hbox = gtk_statusbar_get_message_area (gstatusbar);
  children_list = gtk_container_get_children (GTK_CONTAINER (hbox));
  label = children_list->data;

  gtk_box_set_child_packing (GTK_BOX (hbox), label,
			     FALSE, FALSE, 0, GTK_PACK_START);
  gtk_label_set_ellipsize (GTK_LABEL (label), FALSE);

  /* progressbar for network streams */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, TRUE, 0);
  gtk_widget_show (vbox);

  priv->progress = gtk_progress_bar_new ();
  gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR (priv->progress), 
				    gtk_widget_get_direction (priv->progress) == GTK_TEXT_DIR_LTR ?
				    FALSE : TRUE);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress), 0.);
  gtk_box_pack_start (GTK_BOX (vbox), priv->progress, TRUE, TRUE, 1);
  gtk_widget_set_size_request (priv->progress, 150, 10);
  //gtk_widget_hide (priv->progress);

  packer = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
  gtk_box_pack_start (GTK_BOX (hbox), packer, FALSE, FALSE, 0);
  gtk_widget_show (packer);

  priv->time_label = gtk_label_new (_("0:00 / 0:00"));
  gtk_misc_set_alignment (GTK_MISC (priv->time_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), priv->time_label, FALSE, FALSE, 0);
  gtk_widget_show (priv->time_label);

  xplayer_statusbar_set_text (statusbar, _("Stopped"));
}

GtkWidget* 
xplayer_statusbar_new (void)
{
  return g_object_new (XPLAYER_TYPE_STATUSBAR, NULL);
}

static void
xplayer_statusbar_update_time (XplayerStatusbar *statusbar)
{
  XplayerStatusbarPrivate *priv = statusbar->priv;
  char *time_string, *length, *label;

  time_string = xplayer_time_to_string (priv->time * 1000);

  if (priv->length < 0) {
    label = g_strdup_printf (_("%s (Streaming)"), time_string);
  } else {
    length = xplayer_time_to_string
	    (priv->length == -1 ? 0 : priv->length * 1000);

    if (priv->seeking == FALSE)
      /* Elapsed / Total Length */
      label = g_strdup_printf (_("%s / %s"), time_string, length);
    else
      /* Seeking to Time / Total Length */
      label = g_strdup_printf (_("Seek to %s / %s"), time_string, length);

    g_free (length);
  }
  g_free (time_string);

  gtk_label_set_text (GTK_LABEL (priv->time_label), label);
  g_free (label);

  xplayer_statusbar_sync_description (statusbar);
}

void
xplayer_statusbar_set_text (XplayerStatusbar *statusbar, const char *label)
{
  GtkStatusbar *gstatusbar = GTK_STATUSBAR (statusbar);
  guint id;

  id = gtk_statusbar_get_context_id (gstatusbar, NORMAL_CONTEXT);
  gtk_statusbar_pop (gstatusbar, id);
  gtk_statusbar_push (gstatusbar, id, label);

  xplayer_statusbar_sync_description (statusbar);
}

void
xplayer_statusbar_set_time (XplayerStatusbar *statusbar, gint _time)
{
  g_return_if_fail (XPLAYER_IS_STATUSBAR (statusbar));

  if (statusbar->priv->time == _time)
    return;

  statusbar->priv->time = _time;
  xplayer_statusbar_update_time (statusbar);
}

/* Set a help message to be displayed in the status bar. */
void
xplayer_statusbar_push_help (XplayerStatusbar *statusbar, const char *message)
{
  GtkStatusbar *gstatusbar = GTK_STATUSBAR (statusbar);
  guint id;

  id = gtk_statusbar_get_context_id (gstatusbar, HELP_CONTEXT);
  gtk_statusbar_push (gstatusbar, id, message);
}

/* Remove the last help message of the status bar. */
void
xplayer_statusbar_pop_help (XplayerStatusbar *statusbar)
{
  GtkStatusbar *gstatusbar = GTK_STATUSBAR (statusbar);
  guint id;

  id = gtk_statusbar_get_context_id (gstatusbar, HELP_CONTEXT);
  gtk_statusbar_pop (gstatusbar, id);
}

static gboolean
xplayer_statusbar_timeout_pop (XplayerStatusbar *statusbar)
{
  XplayerStatusbarPrivate *priv = statusbar->priv;
  GtkStatusbar *gstatusbar = GTK_STATUSBAR (statusbar);

  if (--priv->timeout_ticks > 0)
    return TRUE;

  priv->pushed = FALSE;

  gtk_statusbar_pop (gstatusbar,
                     gtk_statusbar_get_context_id (gstatusbar, BUFFERING_CONTEXT));

  gtk_widget_hide (priv->progress);

  xplayer_statusbar_sync_description (statusbar);

  priv->percentage = 101;

  priv->timeout = 0;

  return FALSE;
}

void
xplayer_statusbar_push (XplayerStatusbar *statusbar, gdouble percentage)
{
  XplayerStatusbarPrivate *priv = statusbar->priv;
  GtkStatusbar *gstatusbar = GTK_STATUSBAR (statusbar);
  char *label;
  gboolean need_update = FALSE;

  if (priv->pushed == FALSE)
  {
    gtk_statusbar_push (gstatusbar,
                        gtk_statusbar_get_context_id (gstatusbar, BUFFERING_CONTEXT),
                        _("Buffering"));
    priv->pushed = TRUE;

    need_update = TRUE;
  }

  if (priv->percentage != percentage)
  {
    priv->percentage = percentage;

    /* eg: 75 % */
    label = g_strdup_printf (_("%lf %%"), floorf (percentage));
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress), label);
    g_free (label);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress),
                                  percentage);
    gtk_widget_show (priv->progress);

    need_update = TRUE;
  }

  priv->timeout_ticks = 3;

  if (priv->timeout == 0)
  {
    priv->timeout = g_timeout_add_seconds (1, (GSourceFunc) xplayer_statusbar_timeout_pop, statusbar);
  }

  if (need_update)
    xplayer_statusbar_sync_description (statusbar);
}

void
xplayer_statusbar_pop (XplayerStatusbar *statusbar)
{
  if (statusbar->priv->pushed != FALSE)
  {
    g_source_remove (statusbar->priv->timeout);
    xplayer_statusbar_timeout_pop (statusbar);
  }
}

void
xplayer_statusbar_set_time_and_length (XplayerStatusbar *statusbar,
				     gint _time, gint length)
{
  g_return_if_fail (XPLAYER_IS_STATUSBAR (statusbar));

  if (_time != statusbar->priv->time ||
      length != statusbar->priv->length) {
    statusbar->priv->time = _time;
    statusbar->priv->length = length;

    xplayer_statusbar_update_time (statusbar);
  }
}

void
xplayer_statusbar_set_seeking (XplayerStatusbar *statusbar,
			     gboolean seeking)
{
  g_return_if_fail (XPLAYER_IS_STATUSBAR (statusbar));

  if (statusbar->priv->seeking == seeking)
    return;

  statusbar->priv->seeking = seeking;

  xplayer_statusbar_update_time (statusbar);
}

static void
xplayer_statusbar_sync_description (XplayerStatusbar *statusbar)
{
  GtkWidget *message_area, *label;
  AtkObject *obj;
  GList *children_list;
  char *text;

  message_area = gtk_statusbar_get_message_area (GTK_STATUSBAR (statusbar));
  children_list = gtk_container_get_children (GTK_CONTAINER (message_area));
  label = children_list->data;

  obj = gtk_widget_get_accessible (GTK_WIDGET (statusbar));
  if (statusbar->priv->pushed == FALSE) {
    /* eg: Paused, 0:32 / 1:05 */
    text = g_strdup_printf (_("%s, %s"),
	gtk_label_get_text (GTK_LABEL (label)),
	gtk_label_get_text (GTK_LABEL (statusbar->priv->time_label)));
  } else {
    /* eg: Buffering, 75 % */
    text = g_strdup_printf (_("%s, %f %%"),
	gtk_label_get_text (GTK_LABEL (label)),
	floorf (statusbar->priv->percentage));
  }

  atk_object_set_name (obj, text);
  g_free (text);
}

static void
xplayer_statusbar_finalize (GObject *object)
{
  XplayerStatusbarPrivate *priv = XPLAYER_STATUSBAR (object)->priv;

  if (priv->timeout != 0)
    g_source_remove (priv->timeout);

  G_OBJECT_CLASS (xplayer_statusbar_parent_class)->finalize (object);
}

/*
 * vim: sw=2 ts=8 cindent noai bs=2
 */
