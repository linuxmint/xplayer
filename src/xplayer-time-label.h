
#ifndef XPLAYER_TIME_LABEL_H
#define XPLAYER_TIME_LABEL_H

#include <gtk/gtk.h>

#define XPLAYER_TYPE_TIME_LABEL            (xplayer_time_label_get_type ())
#define XPLAYER_TIME_LABEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XPLAYER_TYPE_TIME_LABEL, XplayerTimeLabel))
#define XPLAYER_TIME_LABEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XPLAYER_TYPE_TIME_LABEL, XplayerTimeLabelClass))
#define XPLAYER_IS_TIME_LABEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XPLAYER_TYPE_TIME_LABEL))
#define XPLAYER_IS_TIME_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XPLAYER_TYPE_TIME_LABEL))

typedef struct XplayerTimeLabel	      XplayerTimeLabel;
typedef struct XplayerTimeLabelClass    XplayerTimeLabelClass;
typedef struct _XplayerTimeLabelPrivate XplayerTimeLabelPrivate;

struct XplayerTimeLabel {
	GtkLabel parent;
	XplayerTimeLabelPrivate *priv;
};

struct XplayerTimeLabelClass {
	GtkLabelClass parent_class;
};

G_MODULE_EXPORT GType xplayer_time_label_get_type (void);
GtkWidget *xplayer_time_label_new                 (void);
void       xplayer_time_label_set_time            (XplayerTimeLabel *label,
                                                 gint64 time, gint64 length);
void       xplayer_time_label_set_seeking         (XplayerTimeLabel *label,
                                                 gboolean seeking);

#endif /* XPLAYER_TIME_LABEL_H */
