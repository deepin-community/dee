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
 *              Michal Hruby <michal.hruby@canonical.com>
 *
 */

#include "config.h"
#include <glib.h>
#include <glib-object.h>

#include <gtx.h>
#include <dee.h>

guint64 before_begin_seqnum, before_end_seqnum,
        after_begin_seqnum, after_end_seqnum;

static void
_begin_txn (DeeSharedModel *model, guint64 begin_seqnum, guint64 end_seqnum)
{
  before_begin_seqnum = begin_seqnum;
  before_end_seqnum = end_seqnum;
}

static void
_end_txn (DeeSharedModel *model, guint64 begin_seqnum, guint64 end_seqnum)
{
  after_begin_seqnum = begin_seqnum;
  after_end_seqnum = end_seqnum;
}

static void
_row_added (DeeModel *model, DeeModelIter *iter, GSList **added)
{
  /* Yes, I _know_ that append() is slow, but this is a test! */
  *added = g_slist_append (*added, iter);
}

/* Expects a clone with 3 rows in it */
gint
main (gint argc, gchar *argv[])
{
  DeeModel     *model;
  DeeModelIter *iter;
  GSList       *added;
  
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init (); 
#endif

  if (argc == 2)
    model = dee_shared_model_new (argv[1]);
  else
    model = dee_shared_model_new_for_peer ((DeePeer*) dee_client_new (argv[1]));

  g_signal_connect (model, "begin-transaction", G_CALLBACK (_begin_txn), NULL);
  g_signal_connect (model, "end-transaction", G_CALLBACK (_end_txn), NULL);
  
  if (gtx_wait_for_signal (G_OBJECT (model), 100000, "notify::synchronized", NULL))
    g_error ("Helper model timed out waiting for 'ready' signal");

  g_assert_cmpint (dee_model_get_n_rows (model), ==, 3);

  /* Listen for adds */
  added = NULL;
  g_signal_connect (model, "row-added", G_CALLBACK (_row_added), &added);

  /* We expect the model to be cleared and 5 rows added */
  gtx_yield_main_loop (1000);

  /* The transaction could be optimized in the future and actually just add
   * the two missing rows */
  g_assert (g_slist_length (added) == 5 || g_slist_length (added) == 2);
  g_assert_cmpint (dee_model_get_n_rows (model), ==, 5);
  
  /* Check ordering and contents */
  iter = dee_model_get_first_iter (model);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), ==, 0);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), ==, "zero");
  
  iter = dee_model_next (model, iter);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), ==, 1);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), ==, "one");
  
  iter = dee_model_next (model, iter);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), ==, 2);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), ==, "two");
  
  iter = dee_model_next (model, iter);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), ==, 3);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), ==, "three");
  
  iter = dee_model_next (model, iter);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), ==, 4);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), ==, "four");
  
  /* Disregarding the hypothetical optimization mentioned above we need the
   * correct seqnum */
  g_assert_cmpint (11, ==, (guint) dee_serializable_model_get_seqnum (model));
  
  g_assert (before_begin_seqnum == after_begin_seqnum);
  g_assert (before_end_seqnum == after_end_seqnum);
  g_assert_cmpint (3, ==, (guint) before_begin_seqnum);
  g_assert_cmpint (11, ==, (guint) before_end_seqnum);
  
  gtx_assert_last_unref (model);
  g_slist_free (added);
  
  return 0;
}
