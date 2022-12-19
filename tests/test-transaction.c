/*
 * Copyright (C) 2011 Canonical Ltd
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

#include <glib.h>
#include <glib-object.h>
#include <dee.h>

typedef struct
{
  DeeModel       *txn;
  DeeModel       *model;

} Fixture;

static void setup        (Fixture *fix, gconstpointer data);
static void setup_proxy  (Fixture *fix, gconstpointer data);
static void setup_shared (Fixture *fix, gconstpointer data);
static void teardown     (Fixture *fix, gconstpointer data);

static void
setup (Fixture *fix, gconstpointer data)
{
  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "s", "i", NULL);

  /* The txn must be created during the tests because we need to verify how
   * it works when constructed on top of various states of the fix->model */
}

static void
setup_basic_types (Fixture *fix, gconstpointer data)
{
  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "b", "y", "i", "u", "x", "t", "d", "s", NULL);

  /* The txn must be created during the tests because we need to verify how
   * it works when constructed on top of various states of the fix->model */
}

static void
setup_proxy (Fixture *fix, gconstpointer data)
{
  DeeModel *backend = dee_sequence_model_new ();
  dee_model_set_schema (backend, "s", "i", NULL);

  fix->model = g_object_new (DEE_TYPE_PROXY_MODEL, "back-end", backend, NULL);
  
  /* The txn must be created during the tests because we need to verify how
   * it works when constructed on top of various states of the fix->model */
  
  g_object_unref (backend);
}

static void
setup_shared (Fixture *fix, gconstpointer data)
{
  fix->model = dee_shared_model_new ("my.test.Model");
  dee_model_set_schema (fix->model, "s", "i", NULL);
  
  /* The txn must be created during the tests because we need to verify how
   * it works when constructed on top of various states of the fix->model */
}

static void
teardown (Fixture *fix, gconstpointer data)
{
  g_object_unref (fix->model);

  if (fix->txn)
    g_object_unref (fix->txn);
}

static void
test_adopt_schema (Fixture *fix, gconstpointer data)
{
  fix->txn = dee_transaction_new (fix->model);

  g_assert_cmpint (dee_model_get_n_columns (fix->txn), == , 2);
  g_assert_cmpstr (dee_model_get_column_schema (fix->txn, 0), ==, "s");
  g_assert_cmpstr (dee_model_get_column_schema (fix->txn, 1), ==, "i");

  g_assert (dee_transaction_get_target (DEE_TRANSACTION (fix->txn)) == fix->model);
}

static void
test_target_0_add_1 (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter, *txn_iter;
  gchar        *s;
  int           i;
  GError       *error;

  /**
   * Target:
   * Txn:    I
   */

  fix->txn = dee_transaction_new (fix->model);

  /* Add one row to txn */
  txn_iter = dee_model_append (fix->txn, "I", 1);
  g_assert_cmpint (1, ==, dee_model_get_n_rows (fix->txn));

  dee_model_get (fix->txn, txn_iter, &s, &i);
  g_assert_cmpstr ("I", ==, s);
  g_assert_cmpint (1, ==, i);

  /* Commit and verify target */
  g_assert_cmpint (0, ==, dee_model_get_n_rows (fix->model));

  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
                  "but error was was to: %s", error->message);
    }

  iter = dee_model_get_first_iter (fix->model);
  g_assert_cmpint (1, ==, dee_model_get_n_rows (fix->model));

  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("I", ==, s);
  g_assert_cmpint (1, ==, i);

}

static void
test_target_1_add_1 (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter, *txn_iter, *txn_end_iter;
  gchar        *s;
  int           i;
  GError       *error;

  /**
   * Target: A
   * Txn:    A, I
   */

  /* The point of this test is that with 1 row in the target and 1 new row
   * in the txn we can easily test out the behaviour when stepping back and
   * forth over old and new rows in the txn */

  iter = dee_model_append (fix->model, "TwentySeven", 27);

  fix->txn = dee_transaction_new (fix->model);

  /* Assert that the unmodified txn is identical to the target */
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 1);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("TwentySeven", ==, s);
  g_assert_cmpint (27, ==, i);

  txn_iter = dee_model_get_first_iter (fix->txn);
  g_assert (txn_iter == iter);

  /* Append a row to txn and assert that txn and target are both as expected */
  txn_iter = dee_model_append (fix->txn, "Append", 7);
  g_assert (txn_iter != iter);

  /* Orig row is unmodified */
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("TwentySeven", ==, s);
  g_assert_cmpint (27, ==, i);

  /* New row is in txn */
  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->txn));
  g_assert (txn_iter == dee_model_next (fix->txn, iter));
  g_assert (iter == dee_model_get_first_iter (fix->txn));
  g_assert (dee_model_is_last (fix->txn, dee_model_next (fix->txn, txn_iter)));

  dee_model_get (fix->txn, txn_iter, &s, &i);
  g_assert_cmpstr ("Append", ==, s);
  g_assert_cmpint (7, ==, i);

  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("TwentySeven", ==, s);
  g_assert_cmpint (27, ==, i);

  /* New row not in target */
  g_assert_cmpint (1, ==, dee_model_get_n_rows (fix->model));
  g_assert (iter == dee_model_get_first_iter (fix->model));
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("TwentySeven", ==, s);
  g_assert_cmpint (27, ==, i);

  /* txn end iter is shared with target */
  txn_end_iter = dee_model_get_last_iter (fix->txn);
  g_assert (txn_end_iter == dee_model_get_last_iter (fix->model));

  /* Check that we can step backwards */
  g_assert (txn_iter == dee_model_prev (fix->txn, txn_end_iter));
  g_assert (iter == dee_model_prev (fix->txn, txn_iter));

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
          "but error was was to: %s", error->message);
    }

  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->model));
  g_assert (iter == dee_model_get_first_iter (fix->model));
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("TwentySeven", ==, s);
  g_assert_cmpint (27, ==, i);

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("Append", ==, s);
  g_assert_cmpint (7, ==, i);
}

static void
test_target_1_change_1 (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter, *txn_iter, *txn_end_iter;
  gchar        *s;
  int           i;
  GError       *error;

  /*
   * Target: A
   * Txn:    A'
   */

  iter = dee_model_append (fix->model, "TwentySeven", 27);

  /* Historically the pristine target seqmodel has had a bug here,
   * make sure we don't loose sleep over it ;-) */
  g_assert (iter == dee_model_get_first_iter (fix->model));
  g_assert (iter != dee_model_get_last_iter (fix->model));
  g_assert (!dee_model_is_last (fix->model, iter));

  fix->txn = dee_transaction_new (fix->model);

  /* Assert that the unmodified txn is identical to the target */
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 1);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("TwentySeven", ==, s);
  g_assert_cmpint (27, ==, i);

  txn_iter = dee_model_get_first_iter (fix->txn);
  g_assert (txn_iter == iter);

  /* Change the row in txn and assert that it looks right */
  dee_model_set (fix->txn, iter, "TwentyOne", 21);
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 1);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("TwentyOne", ==, s);
  g_assert_cmpint (21, ==, i);
  g_assert (iter == dee_model_get_first_iter (fix->txn));

  /* Change is not seen in target */
  g_assert_cmpint (dee_model_get_n_rows (fix->model), == , 1);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("TwentySeven", ==, s);
  g_assert_cmpint (27, ==, i);
  g_assert (iter == dee_model_get_first_iter (fix->model));

  /* We can step to the end iter, and it is the same as that of the target */
  txn_end_iter = dee_model_get_last_iter (fix->txn);
  g_assert (dee_model_is_last (fix->txn, txn_end_iter));
  g_assert (txn_end_iter == dee_model_next (fix->txn, iter));
  g_assert (txn_end_iter == dee_model_get_last_iter (fix->model));

  /* We can step back from the end iter to the changed row */
  g_assert (iter == dee_model_prev (fix->txn, txn_end_iter));

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
          "but error was was to: %s", error->message);
    }

  g_assert_cmpint (1, ==, dee_model_get_n_rows (fix->model));
  g_assert (iter == dee_model_get_first_iter (fix->model));
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("TwentyOne", ==, s);
  g_assert_cmpint (21, ==, i);
}

static void
test_target_2_add_3 (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter0, *iter1, *txn_iter, *txn_middle, *end;
  gchar        *s;
  int           i;
  GError       *error;

  /* Target: A, B
   * Txn:    I, A, II, B, III
   * */

  iter0 = dee_model_append (fix->model, "A", 0);
  iter1 = dee_model_append (fix->model, "B", 1);

  fix->txn = dee_transaction_new (fix->model);

  /* Assert that the unmodified txn is identical to the target */
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 2);
  txn_iter = dee_model_get_first_iter (fix->txn);
  g_assert (txn_iter == iter0);
  dee_model_get (fix->txn, txn_iter, &s, &i);
  g_assert_cmpstr ("A", ==, s);
  g_assert_cmpint (0, ==, i);

  txn_iter = dee_model_next (fix->txn, txn_iter);
  g_assert (txn_iter == iter1);
  dee_model_get (fix->txn, txn_iter, &s, &i);
  g_assert_cmpstr ("B", ==, s);
  g_assert_cmpint (1, ==, i);

  /* Assert end iters are the same */
  end = dee_model_get_last_iter (fix->txn);
  g_assert (end == dee_model_get_last_iter (fix->model));

  /* Prepend one row to txn and
   * assert that txn and target are both as expected */
  txn_iter = dee_model_prepend (fix->txn, "I", 11);
  g_assert (txn_iter != iter0 && txn_iter != iter1 && txn_iter != end);
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->txn));
  dee_model_get (fix->txn, txn_iter, &s, &i);
  g_assert_cmpstr ("I", ==, s);
  g_assert_cmpint (11, ==, i);
  g_assert (iter0 == dee_model_next (fix->txn, txn_iter));
  g_assert (iter1 == dee_model_next (fix->txn, iter0));

  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->model));


  /* Append one row to txn */
  txn_iter = dee_model_append (fix->txn, "III", 13);
  g_assert (txn_iter != iter0 && txn_iter != iter1 && txn_iter != end);
  g_assert_cmpint (4, ==, dee_model_get_n_rows (fix->txn));
  dee_model_get (fix->txn, txn_iter, &s, &i);
  g_assert_cmpstr ("III", ==, s);
  g_assert_cmpint (13, ==, i);
  g_assert (end == dee_model_next (fix->txn, txn_iter));
  g_assert (txn_iter == dee_model_next (fix->txn, iter1));
  g_assert (iter1 == dee_model_next (fix->txn, iter0));
  g_assert (iter0 == dee_model_next (fix->txn, dee_model_get_first_iter (fix->txn)));
  g_assert (txn_iter == dee_model_prev (fix->txn, end));
  g_assert (iter1 == dee_model_prev (fix->txn, txn_iter));
  g_assert (iter0 == dee_model_prev (fix->txn, iter1));
  g_assert (dee_model_is_first (fix->txn, dee_model_prev (fix->txn, iter0)));
  g_assert (dee_model_is_last (fix->txn, end));

  /* Insert one row in the middle of txn */
  txn_middle = dee_model_insert_before (fix->txn, iter1, "II", 12);
  g_assert (txn_middle != iter0 && txn_middle != iter1 && txn_middle != end);
  g_assert_cmpint (5, ==, dee_model_get_n_rows (fix->txn));
  dee_model_get (fix->txn, txn_middle, &s, &i);
  g_assert_cmpstr ("II", ==, s);
  g_assert_cmpint (12, ==, i);
  g_assert (iter0 == dee_model_prev (fix->txn, txn_middle));
  g_assert (iter1 == dee_model_next (fix->txn, txn_middle));
  g_assert (txn_middle == dee_model_prev (fix->txn, iter1));
  g_assert (txn_middle == dee_model_next (fix->txn, iter0));

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
          "but error was was to: %s", error->message);
    }

  g_assert (dee_transaction_is_committed (DEE_TRANSACTION (fix->txn)));

  g_assert_cmpint (5, ==, dee_model_get_n_rows (fix->model));
  iter0 = dee_model_get_first_iter (fix->model);
  dee_model_get (fix->model, iter0, &s, &i);
  g_assert_cmpstr ("I", ==, s);
  g_assert_cmpint (11, ==, i);

  iter0 = dee_model_next (fix->model, iter0);
  dee_model_get (fix->model, iter0, &s, &i);
  g_assert_cmpstr ("A", ==, s);
  g_assert_cmpint (0, ==, i);

  iter0 = dee_model_next (fix->model, iter0);
  dee_model_get (fix->model, iter0, &s, &i);
  g_assert_cmpstr ("II", ==, s);
  g_assert_cmpint (12, ==, i);

  iter0 = dee_model_next (fix->model, iter0);
  dee_model_get (fix->model, iter0, &s, &i);
  g_assert_cmpstr ("B", ==, s);
  g_assert_cmpint (1, ==, i);

  iter0 = dee_model_next (fix->model, iter0);
  dee_model_get (fix->model, iter0, &s, &i);
  g_assert_cmpstr ("III", ==, s);
  g_assert_cmpint (13, ==, i);

  g_assert (end == dee_model_next (fix->model, iter0));
}

static void
test_target_2_clear (Fixture *fix, gconstpointer data)
{
  GError       *error;

  /**
   * Target: A B
   * Txn:    - -
   */

  dee_model_append (fix->model, "TwentySeven", 27);
  dee_model_append (fix->model, "TwentyEight", 28);

  fix->txn = dee_transaction_new (fix->model);

  /* Assert that the unmodified txn is identical to the target */
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 2);

  /* Clear txn */
  dee_model_clear (fix->txn);
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 0);

  /* Target is unmodified */
  g_assert_cmpint (dee_model_get_n_rows (fix->model), == , 2);

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
          "but error was was to: %s", error->message);
    }

  g_assert (dee_transaction_is_committed (DEE_TRANSACTION (fix->txn)));

  g_assert_cmpint (0, ==, dee_model_get_n_rows (fix->model));
}

static void
test_target_1_clear_add_3 (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  GError       *error;
  gchar        *s;
  gint32        i;

  /**
   * Target: A B
   * Txn:    - - I II III
   */

  dee_model_append (fix->model, "TwentySeven", 27);

  fix->txn = dee_transaction_new (fix->model);

  /* Assert that the unmodified txn is identical to the target */
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 1);

  /* Clear txn */
  dee_model_clear (fix->txn);
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 0);

  /* Target is unmodified */
  g_assert_cmpint (dee_model_get_n_rows (fix->model), == , 1);

  /* Add rows to txn, funky order for fun */
  dee_model_append (fix->txn, "II", 2);
  dee_model_append (fix->txn, "III", 3);
  dee_model_prepend (fix->txn, "I", 1);

  /* Txn looks like we expect before commit */
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 3);

  iter = dee_model_get_first_iter (fix->txn);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("I", ==, s);
  g_assert_cmpint (1, ==, i);

  iter = dee_model_next (fix->txn, iter);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("II", ==, s);
  g_assert_cmpint (2, ==, i);

  iter = dee_model_next (fix->txn, iter);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("III", ==, s);
  g_assert_cmpint (3, ==, i);

  g_assert (dee_model_is_last (fix->txn, dee_model_next (fix->txn, iter)));

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
          "but error was was to: %s", error->message);
    }

  g_assert (dee_transaction_is_committed (DEE_TRANSACTION (fix->txn)));

  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("I", ==, s);
  g_assert_cmpint (1, ==, i);

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("II", ==, s);
  g_assert_cmpint (2, ==, i);

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("III", ==, s);
  g_assert_cmpint (3, ==, i);

  g_assert (dee_model_is_last (fix->model, dee_model_next (fix->model, iter)));
}

static void
test_target_5_clear_append_2 (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  GError       *error;
  gchar        *s;
  gint32        i;

  /**
   * Target: A B C D E
   * Txn:    - - - - - F G
   */

  dee_model_append (fix->model, "A", (gint32) 'A');
  dee_model_append (fix->model, "B", (gint32) 'B');
  dee_model_append (fix->model, "C", (gint32) 'C');
  dee_model_append (fix->model, "D", (gint32) 'D');
  dee_model_append (fix->model, "E", (gint32) 'E');

  fix->txn = dee_transaction_new (fix->model);
  dee_model_clear (fix->txn);
  dee_model_append (fix->txn, "F", (gint32) 'F');
  dee_model_append (fix->txn, "G", (gint32) 'G');

  /* Txn looks like we expect before commit */
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 2);

  iter = dee_model_get_first_iter (fix->txn);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("F", ==, s);
  g_assert_cmpint ((gint32) 'F', ==, i);

  iter = dee_model_next (fix->txn, iter);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("G", ==, s);
  g_assert_cmpint ((gint32) 'G', ==, i);

  g_assert (dee_model_is_last (fix->txn, dee_model_next (fix->txn, iter)));

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
          "but error was set to: %s", error->message);
    }

  g_assert (dee_transaction_is_committed (DEE_TRANSACTION (fix->txn)));

  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("F", ==, s);
  g_assert_cmpint ((gint32) 'F', ==, i);

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("G", ==, s);
  g_assert_cmpint ((gint32) 'G', ==, i);

  g_assert (dee_model_is_last (fix->model, dee_model_next (fix->model, iter)));
}

static void
test_target_0_clear_append_2 (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  GError       *error;
  gchar        *s;
  gint32        i;

  /**
   * Target: 
   * Txn:    A B
   */

  /* The trick to this is the clear() on an empty model.
   * Or - hopefully that is not a trick... that is what we test ;-) */
  fix->txn = dee_transaction_new (fix->model);
  dee_model_clear (fix->txn);
  dee_model_append (fix->txn, "A", (gint32) 'A');
  dee_model_append (fix->txn, "B", (gint32) 'B');

  /* Txn looks like we expect before commit */
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 2);

  iter = dee_model_get_first_iter (fix->txn);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("A", ==, s);
  g_assert_cmpint ((gint32) 'A', ==, i);

  iter = dee_model_next (fix->txn, iter);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("B", ==, s);
  g_assert_cmpint ((gint32) 'B', ==, i);

  g_assert (dee_model_is_last (fix->txn, dee_model_next (fix->txn, iter)));

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
          "but error was set to: %s", error->message);
    }

  g_assert (dee_transaction_is_committed (DEE_TRANSACTION (fix->txn)));

  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("A", ==, s);
  g_assert_cmpint ((gint32) 'A', ==, i);

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("B", ==, s);
  g_assert_cmpint ((gint32) 'B', ==, i);

  g_assert (dee_model_is_last (fix->model, dee_model_next (fix->model, iter)));
}

static void
test_target_1_change_1_add_2 (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter, *txn_iter;
  gchar        *s;
  int           i;
  GError       *error;

  /*
   * Target: -  A  -
   * Txn:    I  A' II
   */

  iter = dee_model_append (fix->model, "TwentySeven", 27);

  g_assert (iter == dee_model_get_first_iter (fix->model));

  fix->txn = dee_transaction_new (fix->model);

  /* Assert that the unmodified txn is identical to the target */
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 1);

  txn_iter = dee_model_get_first_iter (fix->txn);
  g_assert (txn_iter == iter);

  /* Change the row in txn and assert that it looks right */
  dee_model_set (fix->txn, iter, "TwentyOne", 21);
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 1);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("TwentyOne", ==, s);
  g_assert_cmpint (21, ==, i);
  g_assert (iter == dee_model_get_first_iter (fix->txn));

  /* Change is not seen in target */
  g_assert_cmpint (dee_model_get_n_rows (fix->model), == , 1);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("TwentySeven", ==, s);
  g_assert_cmpint (27, ==, i);
  g_assert (iter == dee_model_get_first_iter (fix->model));

  /* Add two more rows to txn */
  dee_model_append (fix->txn, "ThirtyFour", 34);
  dee_model_prepend (fix->txn, "Eleven", 11);

  g_assert_cmpint (dee_model_get_n_rows (fix->model), ==, 1);
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), ==, 3);

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
          "but error was was to: %s", error->message);
    }

  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("TwentyOne", ==, s);
  g_assert_cmpint (21, ==, i);
  dee_model_get (fix->model, dee_model_get_first_iter (fix->model), &s, &i);
  g_assert_cmpstr ("Eleven", ==, s);
  g_assert_cmpint (11, ==, i);
  dee_model_get (fix->model, dee_model_get_iter_at_row (fix->model, 2), &s, &i);
  g_assert_cmpstr ("ThirtyFour", ==, s);
  g_assert_cmpint (34, ==, i);
}

static void
test_target_1_change_1_clear (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter, *txn_iter;
  gchar        *s;
  int           i;
  GError       *error;

  /*
   * Target: A
   * Txn:    -
   */

  iter = dee_model_append (fix->model, "TwentySeven", 27);

  g_assert (iter == dee_model_get_first_iter (fix->model));

  fix->txn = dee_transaction_new (fix->model);

  /* Assert that the unmodified txn is identical to the target */
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 1);

  txn_iter = dee_model_get_first_iter (fix->txn);
  g_assert (txn_iter == iter);

  /* Change the row in txn and assert that it looks right */
  dee_model_set (fix->txn, iter, "TwentyOne", 21);
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 1);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("TwentyOne", ==, s);
  g_assert_cmpint (21, ==, i);
  g_assert (iter == dee_model_get_first_iter (fix->txn));

  /* Change is not seen in target */
  g_assert_cmpint (dee_model_get_n_rows (fix->model), == , 1);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("TwentySeven", ==, s);
  g_assert_cmpint (27, ==, i);
  g_assert (iter == dee_model_get_first_iter (fix->model));

  /* Clear the model */
  dee_model_clear (fix->txn);

  g_assert_cmpint (dee_model_get_n_rows (fix->model), ==, 1);
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), ==, 0);

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
          "but error was was to: %s", error->message);
    }

  g_assert_cmpint (0, ==, dee_model_get_n_rows (fix->model));
}

static void
test_target_2_change_1_remove_1 (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter, *txn_iter;
  gchar        *s;
  int           i;
  GError       *error;

  /*
   * Target: A B
   * Txn:    - B'
   */

  iter = dee_model_append (fix->model, "TwentySeven", 27);
  dee_model_prepend (fix->model, "Nineteen", 19);


  fix->txn = dee_transaction_new (fix->model);

  /* Assert that the unmodified txn is identical to the target */
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 2);

  txn_iter = dee_model_get_iter_at_row (fix->txn, 1);
  g_assert (txn_iter == iter);

  /* Change the last row in txn and assert that it looks right */
  dee_model_set (fix->txn, iter, "TwentyOne", 21);
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 2);
  dee_model_get (fix->txn, iter, &s, &i);
  g_assert_cmpstr ("TwentyOne", ==, s);
  g_assert_cmpint (21, ==, i);
  g_assert (iter == dee_model_get_iter_at_row (fix->txn, 1));

  /* Change is not seen in target */
  g_assert_cmpint (dee_model_get_n_rows (fix->model), == , 2);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("TwentySeven", ==, s);
  g_assert_cmpint (27, ==, i);
  g_assert (iter == dee_model_get_iter_at_row (fix->model, 1));

  /* Remove the first row */
  dee_model_remove (fix->txn, dee_model_get_first_iter (fix->txn));

  g_assert_cmpint (dee_model_get_n_rows (fix->model), ==, 2);
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), ==, 1);

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
          "but error was was to: %s", error->message);
    }

  g_assert_cmpint (1, ==, dee_model_get_n_rows (fix->model));
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr ("TwentyOne", ==, s);
  g_assert_cmpint (21, ==, i);
}

static void
test_target_2_change_remove_append (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter, *iter_removed;
  GError       *error;
  gchar        *s;
  gint32        i;

  /**
   * Target: A  B
   * Txn:    A' - C
   */

  dee_model_append (fix->model, "TwentySeven", 27);
  dee_model_append (fix->model, "TwentyEight", 28);

  fix->txn = dee_transaction_new (fix->model);

  /* Assert that the unmodified txn is identical to the target */
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 2);

  /* Change the first row */
  iter = dee_model_get_first_iter (fix->txn);
  dee_model_set_value (fix->txn, iter, 0, g_variant_new_string ("***"));
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 2);
  g_assert (dee_model_is_first (fix->txn, iter));

  /* Remove second row */
  iter_removed = dee_model_next (fix->txn, iter);
  dee_model_remove (fix->txn, iter_removed);
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 1);
  g_assert (dee_model_is_first (fix->txn, iter));

  /* Append a new row */
  dee_model_append (fix->txn, "TehNew", 11);
  g_assert_cmpint (dee_model_get_n_rows (fix->txn), == , 2);
  g_assert (dee_model_is_first (fix->txn, iter));

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
                  "but error was was to: %s", error->message);
    }

  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr (s, ==, "***");
  g_assert_cmpint (i, ==, 27);

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &s, &i);
  g_assert_cmpstr (s, ==, "TehNew");
  g_assert_cmpint (i, ==, 11);
}

static int txn_remaining_rows = 2;

void
txn_on_row_added (DeeModel *txn, DeeModelIter *iter)
{ 
  if (txn_remaining_rows == 2)
    {
      g_assert_cmpstr (dee_model_get_string (txn, iter, 0), ==, "A");
      g_assert_cmpint (dee_model_get_int32 (txn, iter, 1), ==, (gint32) 'A');
    }
  else if (txn_remaining_rows == 1)
    {
      g_assert_cmpstr (dee_model_get_string (txn, iter, 0), ==, "B");
      g_assert_cmpint (dee_model_get_int32 (txn, iter, 1), ==, (gint32) 'B');
    }
  else
    {
      g_critical ("Unexpected row-added signal on txn with %i remaining rows",
                  txn_remaining_rows);
    }
  
  txn_remaining_rows--;
}

static int target_remaining_rows = 2;

void
target_on_row_added (DeeModel *target, DeeModelIter *iter)
{ 
  if (target_remaining_rows == 2)
    {
      g_assert_cmpstr (dee_model_get_string (target, iter, 0), ==, "A");
      g_assert_cmpint (dee_model_get_int32 (target, iter, 1), ==, (gint32) 'A');
    }
  else if (target_remaining_rows == 1)
    {
      g_assert_cmpstr (dee_model_get_string (target, iter, 0), ==, "B");
      g_assert_cmpint (dee_model_get_int32 (target, iter, 1), ==, (gint32) 'B');
    }
  else
    {
      g_critical ("Unexpected row-added signal on target with %i remaining rows",
                  target_remaining_rows);
    }
  
  target_remaining_rows--;
}

static void
test_signal_order (Fixture *fix, gconstpointer data)
{
  /* Reset global static state */
  txn_remaining_rows = 2;
  target_remaining_rows = 2;
  
  
  GError       *error;

  /**
   * Target: 
   * Txn:    A B
   */
  
  fix->txn = dee_transaction_new (fix->model);

  g_assert_cmpint (txn_remaining_rows, ==, 2);
  g_assert_cmpint (target_remaining_rows, ==, 2);

  g_signal_connect (fix->model, "row-added",
                    G_CALLBACK (target_on_row_added), NULL);
  g_signal_connect (fix->txn, "row-added",
                    G_CALLBACK (txn_on_row_added), NULL);
  
  dee_model_append (fix->txn, "A", (gint32) 'A');
  
  g_assert_cmpint (txn_remaining_rows, ==, 1);
  g_assert_cmpint (target_remaining_rows, ==, 2);
  
  dee_model_append (fix->txn, "B", (gint32) 'B');
  
  g_assert_cmpint (txn_remaining_rows, ==, 0);
  g_assert_cmpint (target_remaining_rows, ==, 2);

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit with: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
                  "but error was set to: %s", error->message);
    }

  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->model));
  
  g_assert_cmpint (txn_remaining_rows, ==, 0);
  g_assert_cmpint (target_remaining_rows, ==, 0);
}  

static void
test_concurrent_modification (Fixture *fix, gconstpointer data)
{
  GError       *error;

  /**
   * Target: -   (add A while txn open)
   * Txn:    I
   */

  fix->txn = dee_transaction_new (fix->model);

  dee_model_append (fix->model, "TwentySeven", 27);
  dee_model_clear (fix->txn);

  /* COMMIT */
  error = NULL;
  if (dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction committed successfully. "
                  "Expected concurrent modification error.");
    }
  if (!error)
    {
      g_critical ("dee_transaction_commit() returned FALSE, "
                  "but error was not set.");
    }

  g_assert (g_error_matches (error,
                             DEE_TRANSACTION_ERROR,
                             DEE_TRANSACTION_ERROR_CONCURRENT_MODIFICATION));

  g_assert (!dee_transaction_is_committed (DEE_TRANSACTION (fix->txn)));

  /* Target model should not have been cleared */
  g_assert_cmpint (dee_model_get_n_rows (fix->model), ==, 1);
}

static void
test_double_commit (Fixture *fix, gconstpointer data)
{
  GError       *error;

  /**
   * Target: -   (add A while txn open)
   * Txn:    I
   */

  fix->txn = dee_transaction_new (fix->model);

  dee_model_append (fix->txn, "TwentySeven", 27);

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_critical ("dee_transaction_commit() returned TRUE, "
                  "but error was set.");
    }

  /* COMMIT.... AGAIN! */
  error = NULL;
  if (dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction committed successfully. "
          "Expected because of double commit.");
    }
  if (!error)
    {
      g_critical ("dee_transaction_commit() returned FALSE, "
          "but error was not set.");
    }

  g_assert (g_error_matches (error,
                             DEE_TRANSACTION_ERROR,
                             DEE_TRANSACTION_ERROR_COMMITTED));

  /* Target model should be good with 1 row from the first commit */
  g_assert_cmpint (dee_model_get_n_rows (fix->model), ==, 1);
}

static void
test_basic_types (Fixture *fix, gconstpointer data)
{
  GError *error;
  fix->txn = dee_transaction_new (fix->model);

  dee_model_append (fix->txn, TRUE, 27, 28, 29,
                    G_GINT64_CONSTANT (30), G_GUINT64_CONSTANT (31),
                    32.0, "ThirtyThree");

  DeeModelIter *iter = dee_model_get_first_iter (fix->txn);
  g_assert (dee_model_get_bool (fix->txn, iter, 0) == TRUE);
  g_assert (dee_model_get_uchar (fix->txn, iter, 1) == 27);
  g_assert (dee_model_get_int32 (fix->txn, iter, 2) == 28);
  g_assert (dee_model_get_uint32 (fix->txn, iter, 3) == 29);
  g_assert (dee_model_get_int64 (fix->txn, iter, 4) == 30);
  g_assert (dee_model_get_uint64 (fix->txn, iter, 5) == 31);
  g_assert (ABS (dee_model_get_double (fix->txn, iter, 6) - 32.0) <= 0.001);
  g_assert_cmpstr ("ThirtyThree", ==, dee_model_get_string (fix->txn, iter, 7));
  g_assert_cmpstr ("s", ==, g_variant_get_type_string (dee_model_get_value (fix->txn, iter, 7)));

  /* COMMIT */
  error = NULL;
  if (!dee_transaction_commit (DEE_TRANSACTION (fix->txn), &error))
    {
      g_critical ("Transaction failed to commit: %s", error->message);
      g_error_free (error);
    }
  if (error)
    {
      g_assert_not_reached ();
    }
}

// FIXME tags

void
test_transaction_create_suite (void)
{
#define DOMAIN "/Model/Transaction"
#define PROXY_DOMAIN "/Model/Transaction/Proxy"
#define SHARED_DOMAIN "/Model/Transaction/Shared"

  g_test_add (DOMAIN"/AdoptSchema", Fixture, 0,
              setup, test_adopt_schema, teardown);
  g_test_add (PROXY_DOMAIN"/AdoptSchema", Fixture, 0,
              setup_proxy, test_adopt_schema, teardown);
              
  g_test_add (DOMAIN"/Target0Add1", Fixture, 0,
              setup, test_target_0_add_1, teardown);
  g_test_add (PROXY_DOMAIN"/Target0Add1", Fixture, 0,
              setup_proxy, test_target_0_add_1, teardown);
              
  g_test_add (DOMAIN"/Target1Add1", Fixture, 0,
              setup, test_target_1_add_1, teardown);
  g_test_add (PROXY_DOMAIN"/Target1Add1", Fixture, 0,
              setup_proxy, test_target_1_add_1, teardown);
  
  g_test_add (DOMAIN"/Target1Change1", Fixture, 0,
              setup, test_target_1_change_1, teardown);
  g_test_add (PROXY_DOMAIN"/Target1Change1", Fixture, 0,
              setup_proxy, test_target_1_change_1, teardown);
  
  g_test_add (DOMAIN"/Target2Add3", Fixture, 0,
              setup, test_target_2_add_3, teardown);
  g_test_add (PROXY_DOMAIN"/Target2Add3", Fixture, 0,
              setup_proxy, test_target_2_add_3, teardown);
  
  g_test_add (DOMAIN"/Target2Clear", Fixture, 0,
              setup, test_target_2_clear, teardown);
  g_test_add (PROXY_DOMAIN"/Target2Clear", Fixture, 0,
              setup_proxy, test_target_2_clear, teardown);
  
  g_test_add (DOMAIN"/Target1ClearAdd3", Fixture, 0,
              setup, test_target_1_clear_add_3, teardown);
  g_test_add (PROXY_DOMAIN"/Target1ClearAdd3", Fixture, 0,
              setup_proxy, test_target_1_clear_add_3, teardown);
  
  g_test_add (DOMAIN"/Target5ClearAppend2", Fixture, 0,
              setup, test_target_5_clear_append_2, teardown);
  g_test_add (PROXY_DOMAIN"/Target5ClearAppend2", Fixture, 0,
              setup_proxy, test_target_5_clear_append_2, teardown);
  g_test_add (SHARED_DOMAIN"/Target5ClearAppend2", Fixture, 0,
              setup_shared, test_target_5_clear_append_2, teardown);
  
  g_test_add (DOMAIN"/Target0ClearAppend2", Fixture, 0,
              setup, test_target_0_clear_append_2, teardown);
  g_test_add (PROXY_DOMAIN"/Target0ClearAppend2", Fixture, 0,
              setup_proxy, test_target_0_clear_append_2, teardown);
  g_test_add (SHARED_DOMAIN"/Target0ClearAppend2", Fixture, 0,
              setup_shared, test_target_0_clear_append_2, teardown);
  
  g_test_add (DOMAIN"/Target1Change1Add2", Fixture, 0,
              setup, test_target_1_change_1_add_2, teardown);
  g_test_add (PROXY_DOMAIN"/Target1Change1Add2", Fixture, 0,
              setup_proxy, test_target_1_change_1_add_2, teardown);
  g_test_add (SHARED_DOMAIN"/Target1Change1Add2", Fixture, 0,
              setup_shared, test_target_1_change_1_add_2, teardown);
  
  g_test_add (DOMAIN"/Target1Change1Clear", Fixture, 0,
              setup, test_target_1_change_1_clear, teardown);
  g_test_add (PROXY_DOMAIN"/Target1Change1Clear", Fixture, 0,
              setup_proxy, test_target_1_change_1_clear, teardown);
  g_test_add (SHARED_DOMAIN"/Target1Change1Clear", Fixture, 0,
              setup_shared, test_target_1_change_1_clear, teardown);
  
  g_test_add (DOMAIN"/Target2Change1Remove1", Fixture, 0,
              setup, test_target_2_change_1_remove_1, teardown);
  g_test_add (PROXY_DOMAIN"/Target2Change1Remove1", Fixture, 0,
              setup_proxy, test_target_2_change_1_remove_1, teardown);
  
  g_test_add (DOMAIN"/Target2ChangeRemoveAppend", Fixture, 0,
              setup, test_target_2_change_remove_append, teardown);
  g_test_add (PROXY_DOMAIN"/Target2ChangeRemoveAppend", Fixture, 0,
              setup_proxy, test_target_2_change_remove_append, teardown);
  
  g_test_add (DOMAIN"/SignalOrder", Fixture, 0,
              setup, test_signal_order, teardown);
  g_test_add (PROXY_DOMAIN"/SignalOrder", Fixture, 0,
              setup_proxy, test_signal_order, teardown);
  g_test_add (SHARED_DOMAIN"/SignalOrder", Fixture, 0,
              setup_shared, test_signal_order, teardown);
  
  g_test_add (DOMAIN"/ConcurrentModification", Fixture, 0,
              setup, test_concurrent_modification, teardown);
  g_test_add (PROXY_DOMAIN"/ConcurrentModification", Fixture, 0,
              setup_proxy, test_concurrent_modification, teardown);
  
  g_test_add (DOMAIN"/DoubleCommit", Fixture, 0,
              setup, test_double_commit, teardown);
  g_test_add (PROXY_DOMAIN"/DoubleCommit", Fixture, 0,
              setup_proxy, test_double_commit, teardown);

  g_test_add (DOMAIN"/BasicTypes", Fixture, 0,
              setup_basic_types, test_basic_types, teardown);
}

