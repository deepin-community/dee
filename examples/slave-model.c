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
 */

#include <time.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>

#include <dee.h>

static void
print_row (DeeModel *self, DeeModelIter *iter)
{
  GVariant *value;
  gchar    *s;
  gint   i;  

  for (i = 0; i < dee_model_get_n_columns (self); i++)
    {
      value = dee_model_get_value (self, iter, i);
      s = g_variant_print (value, FALSE);
      
     if (i == 0)
       g_printf ("%s", s);
     else
       g_printf (", %s", s);

      g_variant_unref (value);
      g_free (s);
    }
}

static void
on_row_added (DeeModel *self, DeeModelIter *iter)
{  
  g_printf ("ADDED: ");
  print_row (self, iter);
  g_printf ("\n");
  
}


gint
main (gint argc, gchar *argv[])
{
  GMainLoop *loop;
  DeeModel *model;
  const gchar *model_name;
  
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init ();
#endif
#if !GLIB_CHECK_VERSION(2, 32, 0)
  g_thread_init (NULL);
#endif

  if (argc < 2)
    model_name = "com.canonical.Dee.Model.Example";
  else
    model_name = argv[1];

  g_debug ("Joining model '%s'", model_name);

  model = dee_shared_model_new (model_name);
  g_assert (DEE_IS_MODEL (model));

  g_signal_connect (model, "row-added",
                    G_CALLBACK (on_row_added), NULL);

  loop = g_main_loop_new (g_main_context_default (), TRUE);
  g_main_loop_run (loop);

  return 0;
}
