#include <gtk/gtk.h>
#include <libgd/gd-revealer.h>

gint
main (gint argc,
      gchar ** argv)
{
  GtkWidget *window, *revealer, *box, *widget, *entry;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (window, 300, 300);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  widget = gtk_toggle_button_new_with_label ("Revealed");
  gtk_container_add (GTK_CONTAINER (box), widget);
  gtk_container_add (GTK_CONTAINER (window), box);

  revealer = gd_revealer_new ();
  entry = gtk_entry_new ();
  gtk_container_add (GTK_CONTAINER (revealer), entry);
  gtk_container_add (GTK_CONTAINER (box), revealer);

  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);

  gtk_widget_show_all (window);
  gtk_main ();

  gtk_widget_destroy (window);

  return 0;
}
