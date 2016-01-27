
#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "bacon-video-widget.h"
#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#endif

static char **filenames;
static const char *argument;
static char *mrl;

static void
test_bvw_set_mrl (GtkWidget *bvw, const char *path)
{
	mrl = g_strdup (path);
	bacon_video_widget_open (BACON_VIDEO_WIDGET (bvw), mrl);
}

static void
on_redirect (GtkWidget *bvw, const char *redirect_mrl, gpointer data)
{
	g_message ("Redirect to: %s", redirect_mrl);
}

static void
on_eos_event (GtkWidget *bvw, gpointer user_data)
{
	bacon_video_widget_stop (BACON_VIDEO_WIDGET (bvw));
	bacon_video_widget_close (BACON_VIDEO_WIDGET (bvw));
	g_free (mrl);

	test_bvw_set_mrl (bvw, argument);

	bacon_video_widget_play (BACON_VIDEO_WIDGET (bvw), NULL);
}

static void
on_got_metadata (BaconVideoWidget *bvw, gpointer data)
{
	GValue value = { 0, };
	char *title, *artist;

	bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw),
			BVW_INFO_TITLE, &value);
	title = g_value_dup_string (&value);
	g_value_unset (&value);

	bacon_video_widget_get_metadata (BACON_VIDEO_WIDGET (bvw),
			BVW_INFO_ARTIST, &value);
	artist = g_value_dup_string (&value);
	g_value_unset (&value);

	g_message ("Got metadata: title = %s artist = %s", title, artist);
}

static void
error_cb (GtkWidget *bvw, const char *message,
		gboolean playback_stopped, gboolean fatal)
{
	g_message ("Error: %s, playback stopped: %d, fatal: %d",
			message, playback_stopped, fatal);
}

static GOptionEntry option_entries [] = {
	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY /* STRING? */, &filenames, NULL },
	{ NULL }
};

int main
(int argc, char **argv)
{
	GOptionContext *context;
	GOptionGroup *baconoptiongroup;
	GError *error = NULL;
	GtkWidget *win, *bvw;
	GtkSettings *gtk_settings;

#ifdef GDK_WINDOWING_X11
	XInitThreads ();
#endif

	if (gtk_clutter_init (NULL, NULL) != CLUTTER_INIT_SUCCESS)
		g_assert_not_reached ();

	context = g_option_context_new ("- Play audio and video inside a web browser");
	baconoptiongroup = bacon_video_widget_get_option_group();
	g_option_context_add_main_entries (context, option_entries, GETTEXT_PACKAGE);
	g_option_context_set_translation_domain(context, GETTEXT_PACKAGE);
	g_option_context_add_group (context, baconoptiongroup);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));

	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_print ("Failed to parse options: %s\n", error->message);
		g_error_free (error);
		return 1;
	}
	if (filenames != NULL &&
	    g_strv_length (filenames) > 1) {
		char *help;
		help = g_option_context_get_help (context, TRUE, NULL);
		g_print ("%s", help);
		g_free (help);
		return 1;
	}

	gtk_settings = gtk_settings_get_default ();
	g_object_set (G_OBJECT (gtk_settings), "gtk-application-prefer-dark-theme", TRUE, NULL);

	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (win), 500, 500);
	g_signal_connect (G_OBJECT (win), "destroy",
			G_CALLBACK (gtk_main_quit), NULL);

	bvw = bacon_video_widget_new (NULL);
	bacon_video_widget_set_logo (BACON_VIDEO_WIDGET (bvw), "totem");
	bacon_video_widget_set_show_visualizations (BACON_VIDEO_WIDGET (bvw), TRUE);

	g_signal_connect (G_OBJECT (bvw), "eos", G_CALLBACK (on_eos_event), NULL);
	g_signal_connect (G_OBJECT (bvw), "got-metadata", G_CALLBACK (on_got_metadata), NULL);
	g_signal_connect (G_OBJECT (bvw), "got-redirect", G_CALLBACK (on_redirect), NULL);
	g_signal_connect (G_OBJECT (bvw), "error", G_CALLBACK (error_cb), NULL);

	gtk_container_add (GTK_CONTAINER (win),bvw);

	gtk_widget_realize (GTK_WIDGET (win));
	gtk_widget_realize (bvw);

	gtk_widget_show (win);
	gtk_widget_show (bvw);

	if (filenames && filenames[0]) {
		test_bvw_set_mrl (bvw, filenames[0]);
		argument = g_strdup (filenames[0]);
		bacon_video_widget_play (BACON_VIDEO_WIDGET (bvw), NULL);
	} else {
		bacon_video_widget_set_logo_mode (BACON_VIDEO_WIDGET (bvw), TRUE);
	}

	gtk_main ();

	return 0;
}

