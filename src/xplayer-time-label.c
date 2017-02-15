
#include "config.h"

#include "xplayer-time-label.h"
#include <glib/gi18n.h>
#include "xplayer-time-helpers.h"

struct _XplayerTimeLabelPrivate {
	gint64 time;
	gint64 length;
	gboolean seeking;
};

G_DEFINE_TYPE (XplayerTimeLabel, xplayer_time_label, GTK_TYPE_LABEL)
#define XPLAYER_TIME_LABEL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XPLAYER_TYPE_TIME_LABEL, XplayerTimeLabelPrivate))

static void
xplayer_time_label_init (XplayerTimeLabel *label)
{
	char *time_string;
	char *length_string;
	char *label_string;
	label->priv = G_TYPE_INSTANCE_GET_PRIVATE (label, XPLAYER_TYPE_TIME_LABEL, XplayerTimeLabelPrivate);

	time_string = xplayer_time_to_string (0);
	length_string = xplayer_time_to_string (0);
	label_string = g_strdup_printf (_("%s / %s"), time_string, length_string);

	gtk_label_set_text (GTK_LABEL (label), label_string);

	g_free (time_string);
	g_free (length_string);
	g_free (label_string);

	label->priv->time = 0;
	label->priv->length = -1;
	label->priv->seeking = FALSE;
}

GtkWidget*
xplayer_time_label_new (void)
{
	return GTK_WIDGET (g_object_new (XPLAYER_TYPE_TIME_LABEL, NULL));
}

static void
xplayer_time_label_class_init (XplayerTimeLabelClass *klass)
{
	g_type_class_add_private (klass, sizeof (XplayerTimeLabelPrivate));
}

void
xplayer_time_label_set_time (XplayerTimeLabel *label, gint64 _time, gint64 length)
{
	g_return_if_fail (XPLAYER_IS_TIME_LABEL (label));

	if ((_time / 1000 == label->priv->time / 1000) && (length / 1000 == label->priv->length / 1000))
    {
		return;
    }
    else
    {
        char *label_str;
		char *time_str;
        char *length_str;

		time_str = xplayer_time_to_string (_time);
		length_str = xplayer_time_to_string (length);
		if (label->priv->seeking == FALSE)
        {
			/* Elapsed / Total Length */
			label_str = g_strdup_printf (_("%s / %s"), time_str, length_str);
		}
        else
        {
			/* Seeking to Time / Total Length */
			label_str = g_strdup_printf (_("Seek to %s / %s"), time_str, length_str);
		}

        gtk_label_set_text (GTK_LABEL (label), label_str);

        g_free (label_str);
		g_free (time_str);
		g_free (length_str);

        label->priv->time = _time;
        label->priv->length = length;
	}
}

void
xplayer_time_label_set_seeking (XplayerTimeLabel *label, gboolean seeking)
{
	g_return_if_fail (XPLAYER_IS_TIME_LABEL (label));

	label->priv->seeking = seeking;
}
