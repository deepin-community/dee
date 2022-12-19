/*
 * Copyright (C) 2010 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 *              Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 * Compile with:
 *
 * gcc synced-lists.c -o synced-lists `pkg-config --libs --cflags dee-1.0 gtk+-2.0`
 *
 */
#include <time.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <dee.h>
#include <unistd.h>

static DeeModel    *model;
static GtkWidget    *window;
static GtkWidget    *list;
static GtkListStore *store;

static void
on_row_added (DeeModel *model, DeeModelIter *iter)
{
  gint         i = 0;
  gchar       *str = NULL;
  GtkTreeIter  titer;

  dee_model_get (model, iter, &i, &str);

  gtk_list_store_append (store, &titer);
  gtk_list_store_set (store, &titer,
                      0, g_strdup_printf ("%d", i),
                      1, str,
                      2, iter,
                      -1);

  g_free (str);
}

static void
on_row_removed (DeeModel *model, DeeModelIter *old_iter)
{
  GtkTreeIter iter = { 0};

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);

  do
    {
      gpointer data = NULL;

      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          2, &data,
                          -1);

      if (data == old_iter)
        {
          gtk_list_store_remove (store, &iter);
          break;
        }
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
}

static void
on_row_changed (DeeModel *model, DeeModelIter *row_iter)
{
  GtkTreeIter iter = { 0 };

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);

  do
    {
      gpointer data = NULL;

      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          2, &data,
                          -1);

      if (data == row_iter)
        {
          gint i = 0;
          gchar *str = NULL;

          dee_model_get (model, row_iter, &i, &str);

          gtk_list_store_set (store, &iter,
                              0, g_strdup_printf ("%d", i),
                              1, str,
                              -1);
          break;
        }
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
}

static void
add_row (GtkWidget *button)
{
  dee_model_append (model,
                    (gint)getpid (),
                    "Wazza");
}

static void
remove_row (GtkWidget *button)
{
  GtkTreeSelection *sel;
  GtkTreeIter       iter;

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));

  if (gtk_tree_selection_get_selected (sel,
                                       NULL,
                                       &iter))
    {
      gpointer data = NULL;

      gtk_tree_model_get (GTK_TREE_MODEL (store),
                          &iter,
                          2, &data,
                          -1);

      dee_model_remove (model, data);
    }
  else
    g_debug ("No selection to delete");
}

static void
clear_rows (GtkWidget *button)
{
  dee_model_clear (model);
}

static void
on_cell_edited (GtkCellRendererText *renderer,
                gchar               *path,
                gchar               *new_text,
                gpointer             old_data)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (store),
                                       &iter,
                                       path))
    {
      gpointer data = NULL;

      gtk_tree_model_get (GTK_TREE_MODEL (store),
                          &iter,
                          2, &data,
                          -1);

      dee_model_set (model,
                      (DeeModelIter *)data,
                      new_text);
    }
}

gint
main (gint argc, gchar *argv[])
{
  GtkWidget *vbox, *hbox, *scroll, *button;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_resize (GTK_WINDOW (window), 300, 600);
  gtk_container_set_border_width (GTK_CONTAINER (window), 12);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  button = gtk_label_new (g_strdup_printf ("My PID: <b>%d</b>", getpid()));
  g_object_set (button, "use-markup", TRUE, NULL);
  gtk_misc_set_alignment (GTK_MISC (button), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
  gtk_widget_show (scroll);

  list = gtk_tree_view_new ();
  gtk_container_add (GTK_CONTAINER (scroll), list);
  gtk_widget_show (list);

    {
      GtkCellRenderer   *cell;
      GtkTreeViewColumn *col;

      cell = gtk_cell_renderer_text_new ();
      col = gtk_tree_view_column_new_with_attributes ("0",
                                                      cell,
                                                      "text", 0,
                                                      NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (list), col);

      cell = gtk_cell_renderer_text_new ();
      g_object_set (cell, "editable", TRUE, NULL);
      g_signal_connect (cell, "edited",
                        G_CALLBACK (on_cell_edited), NULL);
      col = gtk_tree_view_column_new_with_attributes ("1",
                                                      cell,
                                                      "text", 1,
                                                      NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (list), col);
    }

  hbox = gtk_hbox_new (TRUE, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (add_row), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (remove_row), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_CLEAR);
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (clear_rows), NULL);

  gtk_widget_show_all (window);

  store = gtk_list_store_new (3,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_POINTER);
  gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (store));

  model = dee_shared_model_new ("com.canonical.Dbus.Model.Example");
  dee_model_set_schema (model, "i", "s", NULL);
  g_signal_connect (model, "row-added",
                    G_CALLBACK (on_row_added), NULL);
  g_signal_connect (model, "row-removed",
                    G_CALLBACK (on_row_removed), NULL);
  g_signal_connect (model, "row-changed",
                    G_CALLBACK (on_row_changed), NULL);

  gtk_main ();

  return 0;
}
