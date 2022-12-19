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
_row_removed (DeeModel *model, DeeModelIter *iter, GSList **removed)
{
  /* Yes, I _know_ that append() is slow, but this is a test! */
  *removed = g_slist_append (*removed, iter);
}

/* Expects a clone with 3 rows in it */
gint
main (gint argc, gchar *argv[])
{
  DeeModel     *model;
  GSList        *removed;
  
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

  /* Listen for removes */
  removed = NULL;
  g_signal_connect (model, "row-removed", G_CALLBACK (_row_removed), &removed);

  /* Wait for some RowsChanged signals */
  gtx_yield_main_loop (1000);

  g_assert_cmpint (g_slist_length (removed), ==, 3);
  g_assert_cmpint (dee_model_get_n_rows(model), ==, 0);
  
  g_assert_cmpint (6, ==, (guint) dee_serializable_model_get_seqnum (model));
  
  g_assert (before_begin_seqnum == after_begin_seqnum);
  g_assert (before_end_seqnum == after_end_seqnum);
  g_assert_cmpint (3, ==, (guint) before_begin_seqnum);
  g_assert_cmpint (6, ==, (guint) before_end_seqnum);
  
  gtx_assert_last_unref (model);
  g_slist_free (removed);
  
  return 0;
}
