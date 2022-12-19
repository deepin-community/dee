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
_row_added (DeeModel *model, DeeModelIter *iter, GSList **rows_so_far)
{
  /* Yes, I _know_ that append() is slow, but this is a test! */
  *rows_so_far = g_slist_append (*rows_so_far, iter);
}

/* Expects and empty clone and three rows-added signals */
gint
main (gint argc, gchar *argv[])
{
  DeeModel     *model;
  DeeModelIter *iter;
  GSList        *rows_added;
  
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init (); 
#endif

  if (argc == 2)
    model = dee_shared_model_new (argv[1]);
  else
    model = dee_shared_model_new_for_peer ((DeePeer*) dee_client_new (argv[1]));


  /* Wait until we find the leader */
  if (gtx_wait_for_signal (G_OBJECT (model), 1000, "notify::synchronized", NULL))
    g_error ("Helper model timed out waiting for 'ready' signal");

  g_assert_cmpint (dee_model_get_n_rows (model), ==, 3);

  /* Listen for changes */
  rows_added = NULL;
  g_signal_connect (model, "row-added", G_CALLBACK (_row_added), &rows_added);

  /* Wait for some RowsAdded signals */
  gtx_yield_main_loop (1000);

  /* Check that we got what we expected */
  g_assert_cmpint (g_slist_length (rows_added), == , 1);
  g_assert_cmpint (dee_model_get_n_rows (model), ==, 4);

  iter = (DeeModelIter*) g_slist_nth (rows_added, 0)->data;
  g_assert_cmpint (dee_model_get_position (model, iter), == , 1);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), == , 27);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), == , "twentyseven");
  
  gtx_assert_last_unref (model);
  g_slist_free (rows_added);
  
  return 0;
}
