#include <gtk/gtk.h>
#include <libgd/gd-header-bar.h>

static void
on_switch_clicked (GtkWidget   *button,
                   GdHeaderBar *bar)
{
  GtkWidget *image = NULL;
  static gboolean use_custom = TRUE;

  if (use_custom)
    {
      image = gtk_image_new_from_icon_name ("face-wink-symbolic", GTK_ICON_SIZE_MENU);
      use_custom = FALSE;
    }
  else
    {
      use_custom = TRUE;
    }

  gd_header_bar_set_custom_title (bar, image);
}

gint
main (gint argc,
      gchar ** argv)
{
  GtkWidget *window, *bar, *box, *button;

  gtk_init (&argc, &argv);

  if (g_getenv ("RTL"))
    gtk_widget_set_default_direction (GTK_TEXT_DIR_RTL);
  else
    gtk_widget_set_default_direction (GTK_TEXT_DIR_LTR);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (window, 300, 300);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (window), box);

  bar = gd_header_bar_new ();
  gtk_box_pack_start (GTK_BOX (box), bar, FALSE, TRUE, 0);

  gd_header_bar_set_title (GD_HEADER_BAR (bar), "Title Title Title Title Title Title");
  gd_header_bar_set_subtitle (GD_HEADER_BAR (bar), "Subtitle Subtitle Subtitle Subtitle Subtitle Subtitle");
  button = gtk_button_new_with_label ("Switch");
  gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
  gtk_style_context_add_class (gtk_widget_get_style_context (button), GTK_STYLE_CLASS_RAISED);
  gd_header_bar_pack_start (GD_HEADER_BAR (bar), button);
  g_signal_connect (button, "clicked", G_CALLBACK (on_switch_clicked), bar);

  button = gtk_button_new_with_label ("Done");
  gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
  gtk_style_context_add_class (gtk_widget_get_style_context (button), "suggested-action");

  gd_header_bar_pack_end (GD_HEADER_BAR (bar), button);

  button = gtk_button_new_with_label ("Almost");
  gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
  gtk_style_context_add_class (gtk_widget_get_style_context (button), GTK_STYLE_CLASS_RAISED);
  gd_header_bar_pack_end (GD_HEADER_BAR (bar), button);

  gtk_widget_show_all (window);
  gtk_main ();

  gtk_widget_destroy (window);

  return 0;
}
