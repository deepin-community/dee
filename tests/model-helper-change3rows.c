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
_row_changed (DeeModel *model, DeeModelIter *iter, GSList **changes_so_far)
{
  /* Yes, I _know_ that append() is slow, but this is a test! */
  *changes_so_far = g_slist_append (*changes_so_far, iter);
}

/* Expects a clone with 3 rows in it */
gint
main (gint argc, gchar *argv[])
{
  DeeModel     *model;
  DeeModelIter *iter;
  GSList        *changes;
  
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init (); 
#endif

  if (argc == 2)
    model = dee_shared_model_new (argv[1]);
  else
    model = dee_shared_model_new_for_peer ((DeePeer*) dee_client_new (argv[1]));

  
  if (gtx_wait_for_signal (G_OBJECT (model), 1000, "notify::synchronized", NULL))
    g_error ("Helper model timed out waiting for 'ready' signal");

  g_assert_cmpint (dee_model_get_n_rows (model), ==, 3);

  /* Listen for changes */
  changes = NULL;
  g_signal_connect (model, "row-changed", G_CALLBACK (_row_changed), &changes);

  /* Wait for some RowsChanged signals */
  gtx_yield_main_loop (1000);

  /* Check that we got what we expected */
  g_assert_cmpint (g_slist_length (changes), == , 3);
  g_assert_cmpint (dee_model_get_n_rows (model), ==, 3);

  iter = (DeeModelIter*) g_slist_nth (changes, 0)->data;
  g_assert_cmpint (dee_model_get_position (model, iter), == , 0);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), == , 0);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), == , "changed_zero");  

  iter = (DeeModelIter*) g_slist_nth (changes, 1)->data;
  g_assert_cmpint (dee_model_get_position (model, iter), == , 1);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), == , 1);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), == , "changed_one");

  iter = (DeeModelIter*) g_slist_nth (changes, 2)->data;
  g_assert_cmpint (dee_model_get_position (model, iter), == , 2);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), == , 2);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), == , "changed_two");
  
  gtx_assert_last_unref (model);
  g_slist_free (changes);
  
  return 0;
}
