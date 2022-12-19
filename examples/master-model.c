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

#include <dee.h>

static gboolean
add_ (DeeModel *model)
{
  dee_model_append (model, 10, "Rooney");

  return TRUE;
}

static void
on_row_added (DeeModel *self, DeeModelIter *iter)
{
  gint i = 0;
  gchar *s = NULL;

  dee_model_get (self, iter, &i, &s);

  if (!g_str_equal (s, "Rooney"))
    g_debug ("Master: Row Added: %d %s", i, s);
}

gint
main (gint argc, gchar *argv[])
{
  GMainLoop *loop;
  DeeModel *model;
  
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init ();
#endif
#if !GLIB_CHECK_VERSION(2, 32, 0)
  g_thread_init (NULL);
#endif

  model = dee_shared_model_new ("com.canonical.Dee.Model.Example");
  dee_model_set_schema (model, "i", "s", NULL);
  g_assert (DEE_IS_MODEL (model));

  g_signal_connect (model, "row-added",
                    G_CALLBACK (on_row_added), NULL);

  g_timeout_add_seconds (2, (GSourceFunc)add_, model);

  loop = g_main_loop_new (g_main_context_default (), TRUE);
  g_main_loop_run (loop);

  return 0;
}
