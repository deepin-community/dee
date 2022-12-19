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
 *              Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */

#include "config.h"
#include <glib.h>
#include <glib-object.h>

#include <gtx.h>
#include <dee.h>

static void
_row_removed (DeeModel *model, DeeModelIter *iter, GSList **changes_so_far)
{
  guint pos;

  pos = dee_model_get_position (model, iter);

  /* Yes, I _know_ that append() is slow, but this is a test! */
  *changes_so_far = g_slist_append (*changes_so_far, GUINT_TO_POINTER (pos));
}

/* Expects a clone with 5 rows in it. Rows 0, 4, and 2 will be removed remotely,
 * in that order. Note that removed a row shifts the order of those below it,
 * so we expect to see removals of rows 0, 3, and 1, when correcting for row
 * shifts.
 *
 * This should leave the original rows 1 and 3, now in positions 0, and 1.
 */
gint
main (gint argc, gchar *argv[])
{
  DeeModel      *model;
  DeeModelIter  *iter, *orig_iter1, *orig_iter3;
  GSList        *changes;
  guint           pos;
  
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init (); 
#endif
  if (argc == 2)
    model = dee_shared_model_new (argv[1]);
  else
    model = dee_shared_model_new_for_peer ((DeePeer*) dee_client_new (argv[1]));


  if (gtx_wait_for_signal (G_OBJECT (model), 1000, "notify::synchronized", NULL))
    g_error ("Helper model timed out waiting for 'ready' signal");

  g_assert_cmpint (dee_model_get_n_rows (model), ==, 5);

  /* We should end up with the original rows 1 and 3, so
   * save pointers to those rows */
  orig_iter1 = dee_model_get_iter_at_row (model, 1);
  orig_iter3 = dee_model_get_iter_at_row (model, 3);

  /* Listen for changes */
  changes = NULL;
  g_signal_connect (model, "row-removed", G_CALLBACK (_row_removed), &changes);

  /* Wait for some RowsRmoved signals */
  gtx_yield_main_loop (1000);

  /* Check that we got what we expected */
  g_assert_cmpint (g_slist_length (changes), == , 3);
  g_assert_cmpint (dee_model_get_n_rows (model), ==, 2);

  pos = GPOINTER_TO_UINT (g_slist_nth (changes, 0)->data);
  g_assert_cmpint (pos, == , 0);

  pos = GPOINTER_TO_UINT (g_slist_nth (changes, 1)->data);
  g_assert_cmpint (pos, == , 3);

  pos = GPOINTER_TO_UINT (g_slist_nth (changes, 2)->data);
  g_assert_cmpint (pos, == , 1);

  /* Now assert that the model contains the data from the original
   * rows 1 and 3 */
  iter = dee_model_get_iter_at_row (model, 0);
  if (orig_iter1 != iter)
    g_error ("Expected original row 1 on position 0. Found row with data '%s'",
             dee_model_get_string (model, iter, 1));

  iter = dee_model_get_iter_at_row (model, 1);
  if (orig_iter3 != iter)
      g_error ("Expected original row 3 on position 1. Found row with data '%s'",
               dee_model_get_string (model, iter, 1));
  
  gtx_assert_last_unref (model);
  g_slist_free (changes);
  
  return 0;
}
