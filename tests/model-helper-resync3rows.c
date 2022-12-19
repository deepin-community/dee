/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by:
 *              Michal Hruby <michal.hruby@canonical.com>
 *
 */

#include "config.h"
#include <glib.h>
#include <glib-object.h>

#include <gtx.h>
#include <dee.h>

static void
row_added (DeeModel *model, DeeModelIter *iter, gpointer data)
{
  gint *num_added = (gint*) data;
  (*num_added)++;
}

/* Expects a clone with 3 rows in it */
gint
main (gint argc, gchar *argv[])
{
  DeeModel     *model;
  DeeModelIter *iter;
  gint          num_added;
  
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init (); 
#endif
  g_set_prgname ("model-helper");

  if (argc == 2)
    model = dee_shared_model_new (argv[1]);
  else
    model = dee_shared_model_new_for_peer ((DeePeer*) dee_client_new (argv[1]));

  num_added = 0;
  g_signal_connect (model, "row-added", G_CALLBACK (row_added), &num_added);

  if (gtx_wait_for_signal (G_OBJECT (model), 10000, "notify::synchronized", NULL))
    g_error ("Helper model timed out waiting for 'ready' signal");

  g_assert_cmpint (dee_model_get_n_rows (model), ==, 3);

  iter = dee_model_get_iter_at_row (model, 0);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), == , 0);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), == , "zero");

  iter = dee_model_get_iter_at_row (model, 1);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), == , 1);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), == , "one");

  iter = dee_model_get_iter_at_row (model, 2);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), == , 2);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), == , "two");

  /* The swarm leader goes away and later reappears,
   * swarm-leader prop should toggle */
  if (gtx_wait_for_signal (G_OBJECT (dee_shared_model_get_peer (DEE_SHARED_MODEL (model))), 10000, "notify::swarm-leader", NULL))
    g_error ("Helper model timed out waiting for 'swarm-leader' notification");

  if (!dee_shared_model_is_leader (DEE_SHARED_MODEL (model)))
    g_error ("Helper didn't become leader");

  if (gtx_wait_for_signal (G_OBJECT (dee_shared_model_get_peer (DEE_SHARED_MODEL (model))), 10000, "notify::swarm-leader", NULL))
    g_error ("Helper model timed out waiting for second 'swarm-leader' notification");

  if (dee_shared_model_is_leader (DEE_SHARED_MODEL (model)))
    g_error ("Helper didn't loose swarm leadership");

  /* And let's wait for synchronization again */
  if (gtx_wait_for_signal (G_OBJECT (model), 10000, "notify::synchronized", NULL))
    g_error ("Helper model timed out waiting for 'ready' signal");

  g_assert_cmpint (dee_model_get_n_rows (model), ==, 3);

  iter = dee_model_get_iter_at_row (model, 0);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), == , 0);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), == , "zero");

  iter = dee_model_get_iter_at_row (model, 1);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), == , 1);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), == , "one");

  iter = dee_model_get_iter_at_row (model, 2);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), == , 2);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), == , "two");
  
  gtx_assert_last_unref (model);
  g_assert_cmpint (num_added, >, 3);
  
  return 0;
}
