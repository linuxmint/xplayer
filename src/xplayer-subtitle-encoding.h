/* Encoding stuff */

#ifndef XPLAYER_SUBTITLE_ENCODING_H
#define XPLAYER_SUBTITLE_ENCODING_H

#include <gtk/gtk.h>

void xplayer_subtitle_encoding_init (GtkComboBox *combo);
void xplayer_subtitle_encoding_set (GtkComboBox *combo, const char *encoding);
const char * xplayer_subtitle_encoding_get_selected (GtkComboBox *combo);

#endif /* SUBTITLE_ENCODING_H */
