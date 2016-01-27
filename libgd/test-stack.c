#include <gtk/gtk.h>
#include <libgd/gd-stack.h>
#include <libgd/gd-stack-switcher.h>

GtkWidget *stack;
GtkWidget *switcher;
GtkWidget *w1;

static void
set_visible_child (GtkWidget *button, gpointer data)
{
  gd_stack_set_visible_child (GD_STACK (stack), GTK_WIDGET (data));
}

static void
set_visible_child_name (GtkWidget *button, gpointer data)
{
  gd_stack_set_visible_child_name (GD_STACK (stack), (const char *)data);
}

static void
toggle_homogeneous (GtkWidget *button, gpointer data)
{
  gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
  gd_stack_set_homogeneous (GD_STACK (stack), active);
}

static void
toggle_icon_name (GtkWidget *button, gpointer data)
{
  gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
  gtk_container_child_set (GTK_CONTAINER (stack), w1,
			   "symbolic-icon-name", active ? "edit-find-symbolic" : NULL,
			   NULL);
}

static void
toggle_transitions (GtkWidget *combo, gpointer data)
{
  int id = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
  gd_stack_set_transition_type (GD_STACK (stack), id);
}

gint
main (gint argc,
      gchar ** argv)
{
  GtkWidget *window, *box, *button, *hbox, *combo;
  GtkWidget *w2, *w3;
  GtkListStore* store;
  GtkWidget *tree_view;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkWidget *scrolled_win;
  int i;
  GtkTreeIter iter;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (window, 300, 300);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (window), box);

  switcher = gd_stack_switcher_new ();
  gtk_box_pack_start (GTK_BOX (box), switcher, FALSE, FALSE, 0);

  stack = gd_stack_new ();

  /* Make transitions longer so we can see that they work */
  gd_stack_set_transition_duration (GD_STACK (stack), 500);

  gtk_widget_set_halign (stack, GTK_ALIGN_START);
  gtk_container_add (GTK_CONTAINER (box), stack);

  gd_stack_switcher_set_stack (GD_STACK_SWITCHER (switcher), GD_STACK (stack));

  w1 = gtk_text_view_new ();
  gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (w1)),
			    "This is a\nTest\nBalh!", -1);

  gtk_container_add_with_properties (GTK_CONTAINER (stack), w1,
				     "name", "1",
				     "title", "1",
				     NULL);

  w2 = gtk_button_new_with_label ("Gazoooooooooooooooonk");
  gtk_container_add (GTK_CONTAINER (stack), w2);
  gtk_container_child_set (GTK_CONTAINER (stack), w2,
			   "name", "2",
			   "title", "2",
			   NULL);


  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (scrolled_win, 100, 200);


  store = gtk_list_store_new (1, G_TYPE_STRING);

  for (i = 0; i < 40; i++)
    gtk_list_store_insert_with_values (store, &iter, i, 0,  "Testvalule", -1);

  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

  gtk_container_add (GTK_CONTAINER (scrolled_win), tree_view);
  w3 = scrolled_win;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Target", renderer,
						     "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  gd_stack_add_titled (GD_STACK (stack), w3, "3", "3");

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (box), hbox);

  button = gtk_button_new_with_label ("1");
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child, w1);

  button = gtk_button_new_with_label ("2");
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child, w2);

  button = gtk_button_new_with_label ("3");
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child, w3);

  button = gtk_button_new_with_label ("1");
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child_name, (gpointer) "1");

  button = gtk_button_new_with_label ("2");
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child_name, (gpointer) "2");

  button = gtk_button_new_with_label ("3");
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) set_visible_child_name, (gpointer) "3");

  button = gtk_check_button_new_with_label ("homogeneous");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				gd_stack_get_homogeneous (GD_STACK (stack)));
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", (GCallback) toggle_homogeneous, NULL);

  button = gtk_toggle_button_new_with_label ("Add symbolic icon");
  g_signal_connect (button, "toggled", (GCallback) toggle_icon_name, NULL);
  gtk_container_add (GTK_CONTAINER (hbox), button);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo),
				  "NONE");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo),
				  "CROSSFADE");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo),
				  "SLIDE_RIGHT");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo),
				  "SLIDE_LEFT");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

  gtk_container_add (GTK_CONTAINER (hbox), combo);
  g_signal_connect (combo, "changed", (GCallback) toggle_transitions, NULL);

  gtk_widget_show_all (window);
  gtk_main ();

  gtk_widget_destroy (window);

  return 0;
}
