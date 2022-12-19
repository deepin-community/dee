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
 *              Michal Hruby <michal.hruby@canonical.com>
 *
 */

#include <glib.h>
#include <glib-object.h>
#include <dee.h>
#include <stdlib.h>

typedef struct
{
  DeeModel *model;

} RowsFixture;

static void seq_rows_setup    (RowsFixture *fix, gconstpointer data);
static void seq_rows_teardown (RowsFixture *fix, gconstpointer data);
static void proxy_rows_setup    (RowsFixture *fix, gconstpointer data);
static void proxy_rows_teardown (RowsFixture *fix, gconstpointer data);
static void txn_rows_setup    (RowsFixture *fix, gconstpointer data);
static void txn_rows_teardown (RowsFixture *fix, gconstpointer data);

static void seq_rows_asv_setup   (RowsFixture *fix, gconstpointer data);
static void proxy_rows_asv_setup (RowsFixture *fix, gconstpointer data);
static void txn_rows_asv_setup   (RowsFixture *fix, gconstpointer data);

static void test_rows_allocation (RowsFixture *fix, gconstpointer data);
static void test_rows_clear      (RowsFixture *fix, gconstpointer data);
static void test_insert_at_pos   (RowsFixture *fix, gconstpointer data);
static void test_insert_at_iter  (RowsFixture *fix, gconstpointer data);
static void test_prepend         (RowsFixture *fix, gconstpointer data);
static void test_append          (RowsFixture *fix, gconstpointer data);
static void test_get_value       (RowsFixture *fix, gconstpointer data);
static void test_no_transfer     (RowsFixture *fix, gconstpointer data);
static void test_iter_backwards  (RowsFixture *fix, gconstpointer data);
static void test_illegal_access  (RowsFixture *fix, gconstpointer data);
static void test_sorted          (RowsFixture *fix, gconstpointer data);
static void test_sort_stable     (RowsFixture *fix, gconstpointer data);
static void test_sorted_with_sizes (RowsFixture *fix, gconstpointer data);
static void test_named_cols_append (RowsFixture *fix, gconstpointer data);
static void test_named_cols_fields (RowsFixture *fix, gconstpointer data);
static void test_named_cols_duplicated_fields (RowsFixture *fix, gconstpointer data);
static void test_named_cols_error  (RowsFixture *fix, gconstpointer data);

static void test_model_iter_copy (RowsFixture *fix, gconstpointer data);
static void test_model_iter_free (RowsFixture *fix, gconstpointer data);

void
test_model_rows_create_suite (void)
{
#define ITER_DOMAIN "/ModelIter/Boxing"
#define SEQ_DOMAIN "/Model/Sequence/Rows"
#define PROXY_DOMAIN "/Model/Proxy/Rows"
#define TXN_DOMAIN "/Model/Transaction/Rows"

  g_test_add (ITER_DOMAIN"/Copy", RowsFixture, 0,
              seq_rows_setup, test_model_iter_copy, seq_rows_teardown);
  g_test_add (ITER_DOMAIN"/Free", RowsFixture, 0,
              seq_rows_setup, test_model_iter_free, seq_rows_teardown);

  g_test_add (SEQ_DOMAIN"/Allocation", RowsFixture, 0,
              seq_rows_setup, test_rows_allocation, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/Allocation", RowsFixture, 0,
              proxy_rows_setup, test_rows_allocation, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/Allocation", RowsFixture, 0,
              txn_rows_setup, test_rows_allocation, txn_rows_teardown);
              
  g_test_add (SEQ_DOMAIN"/Clear", RowsFixture, 0,
              seq_rows_setup, test_rows_clear, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/Clear", RowsFixture, 0,
              proxy_rows_setup, test_rows_clear, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/Clear", RowsFixture, 0,
              txn_rows_setup, test_rows_clear, txn_rows_teardown);
              
  g_test_add (SEQ_DOMAIN"/InsertAtPos", RowsFixture, 0,
              seq_rows_setup, test_insert_at_pos, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/InsertAtPos", RowsFixture, 0,
              proxy_rows_setup, test_insert_at_pos, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/InsertAtPos", RowsFixture, 0,
              txn_rows_setup, test_insert_at_pos, txn_rows_teardown);
              
  g_test_add (SEQ_DOMAIN"/InsertAtIter", RowsFixture, 0,
              seq_rows_setup, test_insert_at_iter, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/InsertAtIter", RowsFixture, 0,
              proxy_rows_setup, test_insert_at_iter, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/InsertAtIter", RowsFixture, 0,
              txn_rows_setup, test_insert_at_iter, txn_rows_teardown);
  
  g_test_add (SEQ_DOMAIN"/Prepend", RowsFixture, 0,
              seq_rows_setup, test_prepend, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/Prepend", RowsFixture, 0,
              proxy_rows_setup, test_prepend, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/Prepend", RowsFixture, 0,
              txn_rows_setup, test_prepend, txn_rows_teardown);

  g_test_add (SEQ_DOMAIN"/Append", RowsFixture, 0,
              seq_rows_setup, test_append, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/Append", RowsFixture, 0,
              proxy_rows_setup, test_append, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/Append", RowsFixture, 0,
              txn_rows_setup, test_append, txn_rows_teardown);
  
  g_test_add (SEQ_DOMAIN"/GetValue", RowsFixture, 0,
              seq_rows_setup, test_get_value, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/GetValue", RowsFixture, 0,
              proxy_rows_setup, test_get_value, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/GetValue", RowsFixture, 0,
              txn_rows_setup, test_get_value, txn_rows_teardown);
  
  g_test_add (SEQ_DOMAIN"/NoTransfer", RowsFixture, 0,
              seq_rows_setup, test_no_transfer, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/NoTransfer", RowsFixture, 0,
              proxy_rows_setup, test_no_transfer, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/NoTransfer", RowsFixture, 0,
              txn_rows_setup, test_no_transfer, txn_rows_teardown);

  g_test_add (SEQ_DOMAIN"/IterBackwards", RowsFixture, 0,
              seq_rows_setup, test_iter_backwards, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/IterBackwards", RowsFixture, 0,
              proxy_rows_setup, test_iter_backwards, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/IterBackwards", RowsFixture, 0,
              txn_rows_setup, test_iter_backwards, txn_rows_teardown);

  g_test_add (SEQ_DOMAIN"/IllegalAccess", RowsFixture, 0,
              seq_rows_setup, test_illegal_access, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/IllegalAccess", RowsFixture, 0,
              proxy_rows_setup, test_illegal_access, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/IllegalAccess", RowsFixture, 0,
              txn_rows_setup, test_illegal_access, txn_rows_teardown);

  g_test_add (SEQ_DOMAIN"/Sorted", RowsFixture, 0,
              seq_rows_setup, test_sorted, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/Sorted", RowsFixture, 0,
              proxy_rows_setup, test_sorted, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/Sorted", RowsFixture, 0,
              txn_rows_setup, test_sorted, txn_rows_teardown);

  g_test_add (SEQ_DOMAIN"/Sorted/WithSizes", RowsFixture, 0,
              seq_rows_setup, test_sorted_with_sizes, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/Sorted/WithSizes", RowsFixture, 0,
              proxy_rows_setup, test_sorted_with_sizes, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/Sorted/WithSizes", RowsFixture, 0,
              txn_rows_setup, test_sorted_with_sizes, txn_rows_teardown);

  g_test_add (SEQ_DOMAIN"/StableSorted", RowsFixture, 0,
              seq_rows_setup, test_sort_stable, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/StableSorted", RowsFixture, 0,
              proxy_rows_setup, test_sort_stable, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/StableSorted", RowsFixture, 0,
              txn_rows_setup, test_sort_stable, txn_rows_teardown);

  g_test_add (SEQ_DOMAIN"/NamedColumns/Append", RowsFixture, 0,
              seq_rows_setup, test_named_cols_append, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/NamedColumns/Append", RowsFixture, 0,
              proxy_rows_setup, test_named_cols_append, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/NamedColumns/Append", RowsFixture, 0,
              txn_rows_setup, test_named_cols_append, txn_rows_teardown);

  g_test_add (SEQ_DOMAIN"/NamedColumns/Fields", RowsFixture, 0,
              seq_rows_asv_setup, test_named_cols_fields, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/NamedColumns/Fields", RowsFixture, 0,
              proxy_rows_asv_setup, test_named_cols_fields, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/NamedColumns/Fields", RowsFixture, 0,
              txn_rows_asv_setup, test_named_cols_fields, txn_rows_teardown);

  g_test_add (SEQ_DOMAIN"/NamedColumns/DuplicatedFields", RowsFixture, 0,
              seq_rows_asv_setup, test_named_cols_duplicated_fields, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/NamedColumns/DuplicatedFields", RowsFixture, 0,
              proxy_rows_asv_setup, test_named_cols_duplicated_fields, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/NamedColumns/DuplicatedFields", RowsFixture, 0,
              txn_rows_asv_setup, test_named_cols_duplicated_fields, txn_rows_teardown);

  g_test_add (SEQ_DOMAIN"/NamedColumns/Invalid", RowsFixture, 0,
              seq_rows_setup, test_named_cols_error, seq_rows_teardown);
  g_test_add (PROXY_DOMAIN"/NamedColumns/Invalid", RowsFixture, 0,
              proxy_rows_setup, test_named_cols_error, proxy_rows_teardown);
  g_test_add (TXN_DOMAIN"/NamedColumns/Invalid", RowsFixture, 0,
              txn_rows_setup, test_named_cols_error, txn_rows_teardown);
}

/* setup & teardown functions */
static void
seq_rows_setup (RowsFixture *fix, gconstpointer data)
{
  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "i", "s", NULL);

  g_assert (DEE_IS_SEQUENCE_MODEL (fix->model));
}

static void
seq_rows_teardown (RowsFixture *fix, gconstpointer data)
{
  g_object_unref (fix->model);
  fix->model = NULL;
}

static void
proxy_rows_setup (RowsFixture *fix, gconstpointer data)
{
  seq_rows_setup (fix, data);
  fix->model = g_object_new (DEE_TYPE_PROXY_MODEL,
                             "back-end", fix->model,
                             NULL);
  
  g_assert (DEE_IS_PROXY_MODEL (fix->model));
}

static void
proxy_rows_teardown (RowsFixture *fix, gconstpointer data)
{
  g_object_unref (fix->model);
  fix->model = NULL;
}

static void
txn_rows_setup (RowsFixture *fix, gconstpointer data)
{
  seq_rows_setup (fix, data);
  fix->model = dee_transaction_new (fix->model);

  g_assert (DEE_IS_TRANSACTION (fix->model));
}

static void
txn_rows_teardown (RowsFixture *fix, gconstpointer data)
{
  g_object_unref (fix->model);
  fix->model = NULL;
}

static void
seq_rows_asv_setup (RowsFixture *fix, gconstpointer data)
{
  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "i", "s", "a{sv}", "a{sv}", NULL);
  dee_model_set_column_names (fix->model, "count", "name",
                              "hints", "hints2", NULL);

  g_assert (DEE_IS_SEQUENCE_MODEL (fix->model));
}

static void
proxy_rows_asv_setup (RowsFixture *fix, gconstpointer data)
{
  seq_rows_asv_setup (fix, data);
  fix->model = g_object_new (DEE_TYPE_PROXY_MODEL,
                             "back-end", fix->model,
                             NULL);
  
  g_assert (DEE_IS_PROXY_MODEL (fix->model));
}

static void
txn_rows_asv_setup (RowsFixture *fix, gconstpointer data)
{
  seq_rows_asv_setup (fix, data);
  fix->model = dee_transaction_new (fix->model);

  g_assert (DEE_IS_TRANSACTION (fix->model));
}

/* test cases */
static void test_model_iter_copy (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter, *copied;
 
  iter = dee_model_append (fix->model, 10, "Rooney");

  copied = g_boxed_copy (DEE_TYPE_MODEL_ITER, iter);

  g_assert (iter == copied);
}

static void test_model_iter_free (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  gint i;

  iter = dee_model_append (fix->model, 10, "Rooney");

  g_boxed_free (DEE_TYPE_MODEL_ITER, iter);

  /* And since it's supposed to be a no-op we can do this */
  for (i = 0; i < 100; i++)
    g_boxed_free (DEE_TYPE_MODEL_ITER, iter);

  /* Didn't crash? Good! */
}

static void
test_rows_allocation (RowsFixture *fix, gconstpointer data)
{
  g_assert (DEE_IS_MODEL (fix->model));
}

static gint n_clear_sigs = 0;

static void
on_clear_row_removed (DeeModel *model, DeeModelIter *iter)
{
  n_clear_sigs++;
}

static void
test_rows_clear (RowsFixture *fix, gconstpointer data)
{
  gint i;

  for (i = 0; i < 1000; i++)
    {
      dee_model_append (fix->model, 10, "Rooney");
    }

  g_assert_cmpint (1000, ==, dee_model_get_n_rows (fix->model));

  g_signal_connect (fix->model, "row-removed",
                    G_CALLBACK (on_clear_row_removed), NULL);

  n_clear_sigs = 0;
  dee_model_clear (fix->model);

  g_assert_cmpint (0, ==, dee_model_get_n_rows (fix->model));
  g_assert_cmpint (1000, ==, n_clear_sigs);
}

static void
test_insert_at_pos (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  gint           i;
  gchar         *str;

  dee_model_append (fix->model, 10, "Rooney");
  dee_model_append (fix->model, 10, "Rooney");
  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->model));

  dee_model_insert (fix->model, 1, 27, "Not Rooney");
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  g_assert (dee_model_is_first (fix->model, iter));

  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (10, ==, i);
  g_assert_cmpstr (str, ==, "Rooney");

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (27, ==, i);
  g_assert_cmpstr (str, ==, "Not Rooney");

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (10, ==, i);
  g_assert_cmpstr (str, ==, "Rooney");

  iter = dee_model_next (fix->model, iter);
  g_assert (dee_model_is_last (fix->model, iter));
}

static void
test_insert_at_iter (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  gint           i;
  gchar         *str;

  dee_model_append (fix->model, 10, "Rooney");
  dee_model_append (fix->model, 10, "Rooney");
  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  iter = dee_model_next (fix->model, iter);
  dee_model_insert_before (fix->model, iter, 27, "Not Rooney");
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (10, ==, i);
  g_assert_cmpstr (str, ==, "Rooney");

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (27, ==, i);
  g_assert_cmpstr (str, ==, "Not Rooney");

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (10, ==, i);
  g_assert_cmpstr (str, ==, "Rooney");

  iter = dee_model_next (fix->model, iter);
  g_assert (dee_model_is_last (fix->model, iter));
}

static void
test_prepend (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  gint           i;
  gchar         *str;

  dee_model_prepend (fix->model, 11, "Mid");
  dee_model_append (fix->model, 12, "Last");
  dee_model_prepend (fix->model, 10, "First");
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (10, ==, i);
  g_assert_cmpstr (str, ==, "First");

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (11, ==, i);
  g_assert_cmpstr (str, ==, "Mid");

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (12, ==, i);
  g_assert_cmpstr (str, ==, "Last");

  iter = dee_model_next (fix->model, iter);
  g_assert (dee_model_is_last (fix->model, iter));
}

static void
test_append (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  gint           i;
  gchar         *str;

  dee_model_append (fix->model, 11, "First");
  dee_model_append (fix->model, 12, "Mid");
  dee_model_append (fix->model, 10, "Last");
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (11, ==, i);
  g_assert_cmpstr (str, ==, "First");

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (12, ==, i);
  g_assert_cmpstr (str, ==, "Mid");

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (10, ==, i);
  g_assert_cmpstr (str, ==, "Last");

  iter = dee_model_next (fix->model, iter);
  g_assert (dee_model_is_last (fix->model, iter));
}

static void
test_get_value (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  GVariant     *variant;

  dee_model_set_column_names (fix->model, "count", "name", NULL);

  dee_model_append (fix->model, 11, "First");
  dee_model_append (fix->model, 12, "Mid");
  dee_model_append (fix->model, 10, "Last");
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  variant = dee_model_get_value (fix->model, iter, 0);
  g_assert_cmpint (g_variant_get_int32 (variant), ==, 11);
  g_variant_unref (variant);

  iter = dee_model_next (fix->model, iter);
  variant = dee_model_get_value (fix->model, iter, 1);
  g_assert_cmpstr (g_variant_get_string (variant, NULL), ==, "Mid");
  g_variant_unref (variant);

  iter = dee_model_next (fix->model, iter);
  variant = dee_model_get_value_by_name (fix->model, iter, "count");
  g_assert_cmpint (g_variant_get_int32 (variant), ==, 10);
  g_variant_unref (variant);

  iter = dee_model_next (fix->model, iter);
  g_assert (dee_model_is_last (fix->model, iter));
}

static void
test_iter_backwards (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  gint           i;
  gchar         *str;

  dee_model_append (fix->model, 11, "First");
  dee_model_append (fix->model, 12, "Mid");
  dee_model_append (fix->model, 10, "Last");
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_last_iter (fix->model);
  g_assert (dee_model_is_last (fix->model, iter));

  iter = dee_model_prev (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (10, ==, i);
  g_assert_cmpstr (str, ==, "Last");

  iter = dee_model_prev (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (12, ==, i);
  g_assert_cmpstr (str, ==, "Mid");

  iter = dee_model_prev (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (11, ==, i);
  g_assert_cmpstr (str, ==, "First");

  g_assert (dee_model_is_first (fix->model,   iter));
}

/* Make sure the the memory allocated for strings is not reffed inside
 * the model. Also checks that strings are returned as const pointers */
static void
test_no_transfer (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  gchar         *orig, *str1, *str2;

  orig = g_strdup ("test");

  dee_model_append (fix->model, 1, orig);
  g_assert_cmpint (1, ==, dee_model_get_n_rows (fix->model));

  /* This should work, of course */
  iter = dee_model_get_first_iter (fix->model);
  dee_model_get (fix->model, iter, NULL, &str1);
  g_assert_cmpstr (str1, ==, "test");

  /* Assert that we get a const pointer to the string back.
   * Strings should behave like that */
  dee_model_get (fix->model, iter, NULL, &str2);
  g_assert (str1 == str2);
  
  /* Modify orig in place and assert it doesn't affect the model */
  orig[0] = 'P';
  dee_model_get (fix->model, iter, NULL, &str1, -1);
  g_assert_cmpstr (str1, ==, "test");
  
  /* Now free orig and make sure we can still read from the model */
  g_free (orig);
  dee_model_get (fix->model, iter, NULL, &str1, -1);
  g_assert_cmpstr (str1, ==, "test");
}

/* Try to get and set values from a removed row */
static void
test_illegal_access (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;

  dee_model_append (fix->model, 1, "Hello");
  iter = dee_model_append (fix->model, 1, "Mary");
  dee_model_append (fix->model, 1, "Lou");

  dee_model_remove (fix->model, iter);

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR))
    {
      g_assert_cmpstr (dee_model_get_string (fix->model, iter, 1), ==, "Mary");
      exit (0); /* successful test run */
    }
  g_test_trap_assert_failed ();

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR))
    {
      dee_model_set (fix->model, iter, 27, "Marie");
      exit (0); /* successful test run */
    }
  g_test_trap_assert_failed ();
}

static gint
cmp_constant (GVariant **row1, GVariant **row2, gpointer user_data)
{
  g_assert_cmpstr (user_data, ==, "test-user-data");
  return 0;
}

static gint
cmp_col_0 (GVariant **row1, GVariant **row2, gpointer user_data)
{
  g_assert_cmpstr (user_data, ==, "test-user-data");
  //g_debug ("CMP %i %i", g_variant_get_int32 (row1[0]), g_variant_get_int32 (row2[0]));
  return g_variant_get_int32 (row2[0]) - g_variant_get_int32 (row1[0]);
}

static void
test_sorted (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *hter, *iter, *jter, *kter;
  gboolean      was_found;

  /* FINAL MODEL: [(28,s), (27,s), (26,s), (25,s)]
   *           ~= [hter, iter, jter, kter] */

  /* Test find() with an empty model. With NULL was_found arg */
  iter = dee_model_find_sorted (fix->model, cmp_col_0, "test-user-data", NULL,
                                0, "");
  g_assert (iter == dee_model_get_last_iter (fix->model));

  /* Test find() with an empty model. With non-NULL was_found arg */
  was_found = TRUE;
  iter = dee_model_find_sorted (fix->model, cmp_col_0, "test-user-data", &was_found,
                                0, "");
  g_assert (!was_found);
  g_assert (iter == dee_model_get_last_iter (fix->model));

  /* Insert the first row */
  iter = dee_model_insert_sorted (fix->model, cmp_col_0, "test-user-data",
                                  27, "Sorta sorted");
  g_assert (iter != dee_model_get_last_iter (fix->model));
  g_assert (iter == dee_model_get_first_iter (fix->model));

  /* Test append */
  kter = dee_model_insert_sorted (fix->model, cmp_col_0, "test-user-data",
                                  25, "Sorta sorted");
  g_assert (kter != dee_model_get_last_iter (fix->model));
  g_assert (kter != dee_model_get_first_iter (fix->model));
  g_assert (iter == dee_model_get_first_iter (fix->model));
  g_assert (kter != iter);

  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->model));
  g_assert (kter == dee_model_next (fix->model, iter));

  /* Test insert in between rows */
  jter = dee_model_insert_sorted (fix->model, cmp_col_0, "test-user-data",
                                  26, "Sorta sorted");
  g_assert (jter != dee_model_get_last_iter (fix->model));
  g_assert (jter != dee_model_get_first_iter (fix->model));
  g_assert (iter == dee_model_get_first_iter (fix->model));
  g_assert (jter != iter);
  g_assert (jter != kter);

  g_assert (jter == dee_model_next (fix->model, iter));
  g_assert (kter == dee_model_next (fix->model, jter));
  g_assert (dee_model_get_last_iter (fix->model) == dee_model_next (fix->model, kter));

  /* Test prepend */
  hter = dee_model_insert_sorted (fix->model, cmp_col_0, "test-user-data",
                                  28, "Sorta sorted");
  g_assert (hter == dee_model_get_first_iter (fix->model));
  g_assert (iter == dee_model_next (fix->model, hter));

  g_assert_cmpint (4, ==, dee_model_get_n_rows (fix->model));

  /* Test find() again now that we have data in the model */
  DeeModelIter *result;
  result = dee_model_find_sorted (fix->model, cmp_col_0, "test-user-data", NULL,
                                  24, "");
  g_assert (result == dee_model_get_last_iter (fix->model));
  result = dee_model_find_sorted (fix->model, cmp_col_0, "test-user-data", NULL,
                                  28, "");
  g_assert (result == hter);

  /* Test find(). With non-NULL was_found arg */
  was_found = TRUE;
  result = dee_model_find_sorted (fix->model, cmp_col_0, "test-user-data", &was_found,
                                  24, "");
  g_assert (result == dee_model_get_last_iter (fix->model));
  result = dee_model_find_sorted (fix->model, cmp_col_0, "test-user-data", &was_found,
                                  28, "");
  g_assert (was_found);
  g_assert (result == hter);
}

static void
test_sort_stable (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *hter, *iter, *jter, *kter;

  /* FINAL MODEL: [(25,s), (26,s), (27,s), (28,s)]
   *           ~= [hter, iter, jter, kter] */

  /* Insert the first row */
  iter = dee_model_insert_sorted (fix->model, cmp_constant, "test-user-data",
                                  25, "Stable");
  g_assert (iter != dee_model_get_last_iter (fix->model));
  g_assert (iter == dee_model_get_first_iter (fix->model));

  /* Second row */
  kter = dee_model_insert_sorted (fix->model, cmp_constant, "test-user-data",
                                  26, "Stable");
  g_assert (kter != dee_model_get_last_iter (fix->model));
  g_assert (kter != dee_model_get_first_iter (fix->model));
  g_assert (iter == dee_model_get_first_iter (fix->model));
  g_assert (kter != iter);

  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->model));
  g_assert (kter == dee_model_next (fix->model, iter));

  /* Third row */
  jter = dee_model_insert_sorted (fix->model, cmp_constant, "test-user-data",
                                  27, "Stable");
  g_assert (jter != dee_model_get_last_iter (fix->model));
  g_assert (jter != dee_model_get_first_iter (fix->model));
  g_assert (iter == dee_model_get_first_iter (fix->model));
  g_assert (jter != iter);
  g_assert (jter != kter);

  g_assert (jter == dee_model_next (fix->model, kter));
  g_assert (dee_model_get_last_iter (fix->model) == dee_model_next (fix->model, jter));

  /* Fourth row */
  hter = dee_model_insert_sorted (fix->model, cmp_constant, "test-user-data",
                                  28, "Stable");
  g_assert (hter == dee_model_next (fix->model, jter));

  g_assert_cmpint (4, ==, dee_model_get_n_rows (fix->model));
}

static gint
sized_cmp_col_0 (GVariant **row1, guint row1_length,
                 GVariant **row2, guint row2_length, gpointer user_data)
{
  g_assert_cmpstr (user_data, ==, "test-user-data");
  g_assert_cmpuint (row1_length, ==, 2);
  g_assert_cmpuint (row2_length, ==, 2);
  g_assert_cmpuint (row1_length, ==, row2_length);
  return g_variant_get_int32 (row2[0]) - g_variant_get_int32 (row1[0]);
}

static void
test_sorted_with_sizes (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *hter, *iter, *jter, *kter;
  gboolean      was_found;
  GVariant     *row_spec[2];

  /* FINAL MODEL: [(28,s), (27,s), (26,s), (25,s)]
   *           ~= [hter, iter, jter, kter] */

  row_spec[0] = g_variant_new_int32 (0);
  row_spec[1] = g_variant_new_string ("");
  /* Test find() with an empty model. With NULL was_found arg */
  iter = dee_model_find_row_sorted_with_sizes (fix->model, row_spec,
                                               sized_cmp_col_0,
                                               "test-user-data",
                                               NULL);
  g_assert (iter == dee_model_get_last_iter (fix->model));

  /* Test find() with an empty model. With non-NULL was_found arg */
  was_found = TRUE;
  iter = dee_model_find_row_sorted_with_sizes (fix->model, row_spec,
                                               sized_cmp_col_0,
                                               "test-user-data",
                                               &was_found);
  g_assert (!was_found);
  g_assert (iter == dee_model_get_last_iter (fix->model));

  /* Insert the first row */
  row_spec[0] = g_variant_new_int32 (27);
  row_spec[1] = g_variant_new_string ("Sorta sorted");
  iter = dee_model_insert_row_sorted_with_sizes (fix->model, row_spec,
                                                 sized_cmp_col_0,
                                                 "test-user-data");
  g_assert (iter != dee_model_get_last_iter (fix->model));
  g_assert (iter == dee_model_get_first_iter (fix->model));

  /* Test append */
  row_spec[0] = g_variant_new_int32 (25);
  row_spec[1] = g_variant_new_string ("Sorta sorted");
  kter = dee_model_insert_row_sorted_with_sizes (fix->model, row_spec,
                                                 sized_cmp_col_0,
                                                 "test-user-data");
  g_assert (kter != dee_model_get_last_iter (fix->model));
  g_assert (kter != dee_model_get_first_iter (fix->model));
  g_assert (iter == dee_model_get_first_iter (fix->model));
  g_assert (kter != iter);

  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->model));
  g_assert (kter == dee_model_next (fix->model, iter));

  /* Test insert in between rows */
  row_spec[0] = g_variant_new_int32 (26);
  row_spec[1] = g_variant_new_string ("Sorta sorted");
  jter = dee_model_insert_row_sorted_with_sizes (fix->model, row_spec,
                                                 sized_cmp_col_0,
                                                 "test-user-data");
  g_assert (jter != dee_model_get_last_iter (fix->model));
  g_assert (jter != dee_model_get_first_iter (fix->model));
  g_assert (iter == dee_model_get_first_iter (fix->model));
  g_assert (jter != iter);
  g_assert (jter != kter);

  g_assert (jter == dee_model_next (fix->model, iter));
  g_assert (kter == dee_model_next (fix->model, jter));
  g_assert (dee_model_get_last_iter (fix->model) == dee_model_next (fix->model, kter));

  /* Test prepend */
  row_spec[0] = g_variant_new_int32 (28);
  row_spec[1] = g_variant_new_string ("Sorta sorted");
  hter = dee_model_insert_row_sorted_with_sizes (fix->model, row_spec,
                                                 sized_cmp_col_0,
                                                 "test-user-data");
  g_assert (hter == dee_model_get_first_iter (fix->model));
  g_assert (iter == dee_model_next (fix->model, hter));

  g_assert_cmpint (4, ==, dee_model_get_n_rows (fix->model));

  /* Test find() again now that we have data in the model */
  DeeModelIter *result;
  row_spec[0] = g_variant_new_int32 (24);
  row_spec[1] = g_variant_new_string ("");
  result = dee_model_find_row_sorted_with_sizes (fix->model, row_spec,
                                                 sized_cmp_col_0,
                                                 "test-user-data", NULL);
  g_assert (result == dee_model_get_last_iter (fix->model));
  row_spec[0] = g_variant_new_int32 (28);
  row_spec[1] = g_variant_new_string ("");
  result = dee_model_find_row_sorted_with_sizes (fix->model, row_spec,
                                                 sized_cmp_col_0,
                                                 "test-user-data", NULL);
  g_assert (result == hter);

  /* Test find(). With non-NULL was_found arg */
  was_found = FALSE;
  row_spec[0] = g_variant_new_int32 (24);
  row_spec[1] = g_variant_new_string ("");
  result = dee_model_find_row_sorted_with_sizes (fix->model, row_spec,
                                                 sized_cmp_col_0,
                                                 "test-user-data", &was_found);
  g_assert (result == dee_model_get_last_iter (fix->model));
  row_spec[0] = g_variant_new_int32 (28);
  row_spec[1] = g_variant_new_string ("");
  result = dee_model_find_row_sorted_with_sizes (fix->model, row_spec,
                                                 sized_cmp_col_0,
                                                 "test-user-data", &was_found);
  g_assert (was_found);
  g_assert (result == hter);
}

static void
test_named_cols_append (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  gint          i;
  gchar        *str;
  GVariant     *row_members[2];

  dee_model_set_column_names (fix->model, "count", "name", NULL);

  dee_model_build_named_row (fix->model, row_members,
                             "count", 11, "name", "First", NULL);
  dee_model_append_row (fix->model, row_members);
  dee_model_build_named_row (fix->model, row_members,
                             "name", "Mid", "count", 12, NULL);
  dee_model_append_row (fix->model, row_members);
  dee_model_build_named_row (fix->model, row_members,
                             "count", 10, "name", "Last", NULL);
  dee_model_append_row (fix->model, row_members);
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (11, ==, i);
  g_assert_cmpstr (str, ==, "First");

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (12, ==, i);
  g_assert_cmpstr (str, ==, "Mid");

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str);
  g_assert_cmpint (10, ==, i);
  g_assert_cmpstr (str, ==, "Last");

  iter = dee_model_next (fix->model, iter);
  g_assert (dee_model_is_last (fix->model, iter));
}

static void
test_named_cols_fields (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  gint          i;
  gchar        *str;
  GVariant     *row_members[4];
  GVariant     *dict, *dummy;
  GHashTable   *fields_schemas;

  fields_schemas = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (fields_schemas, "object-path", "o");
  g_hash_table_insert (fields_schemas, "id", "i");

  dee_model_register_vardict_schema (fix->model, 2, fields_schemas);
  g_hash_table_unref (fields_schemas);

  dee_model_build_named_row (fix->model, row_members,
                             "count", 11, "name", "First", NULL);
  dee_model_append_row (fix->model, row_members);
  dee_model_build_named_row (fix->model, row_members,
                             "name", "Mid", "count", 12,
                             "object-path", "/org/example",
                             "hints::id", 8123, NULL);
  dee_model_append_row (fix->model, row_members);
  dee_model_build_named_row (fix->model, row_members,
                             "count", 10, "name", "Last", "id", 90, NULL);
  dee_model_append_row (fix->model, row_members);

  /* Commence checks */
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_get (fix->model, iter, &i, &str, &dict, &dummy);
  g_assert_cmpint (11, ==, i);
  g_assert_cmpstr (str, ==, "First");
  g_assert_cmpuint (0, ==, g_variant_n_children (dict));
  g_assert (dee_model_get_value_by_name (fix->model, iter, "object-path") == NULL);

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str, &dict, &dummy);
  g_assert_cmpint (12, ==, i);
  g_assert_cmpstr (str, ==, "Mid");
  g_assert_cmpuint (2, ==, g_variant_n_children (dict));
  g_assert_cmpstr ("/org/example", ==, g_variant_get_string (g_variant_lookup_value (dict, "object-path", G_VARIANT_TYPE_OBJECT_PATH), NULL));
  g_assert_cmpint (8123, ==, g_variant_get_int32 (dee_model_get_value_by_name (fix->model, iter, "id")));

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str, &dict, &dummy);
  g_assert_cmpint (10, ==, i);
  g_assert_cmpstr (str, ==, "Last");
  g_assert_cmpuint (1, ==, g_variant_n_children (dict));
  g_assert_cmpint (90, ==, g_variant_get_int32 (dee_model_get_value_by_name (fix->model, iter, "id")));

  iter = dee_model_next (fix->model, iter);
  g_assert (dee_model_is_last (fix->model, iter));
}

static gboolean
expected_error_handler (const gchar *log_domain, GLogLevelFlags log_level,
                        const gchar *msg, gpointer user_data)
{
  return FALSE;
}

static void
ignore_error_handler (const gchar *log_domain, GLogLevelFlags log_level,
                      const gchar *msg, gpointer user_data)
{
}

static void
test_named_cols_duplicated_fields (RowsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  gint          i;
  guint         handler_id;
  gchar        *str;
  GVariant     *row_members[4];
  GVariant     *dict1, *dict2;
  GHashTable   *fields_schemas;

  g_test_log_set_fatal_handler (expected_error_handler, NULL);
  handler_id = g_log_set_handler ("dee", G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL,
                                  ignore_error_handler, NULL);

  fields_schemas = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (fields_schemas, "id", "i");

  dee_model_register_vardict_schema (fix->model, 2, fields_schemas);

  g_hash_table_insert (fields_schemas, "id", "s");
  g_hash_table_insert (fields_schemas, "extra-id", "i");
  dee_model_register_vardict_schema (fix->model, 3, fields_schemas);
  g_hash_table_unref (fields_schemas);

  dee_model_build_named_row (fix->model, row_members,
                             "count", 11, "name", "First", NULL);
  dee_model_append_row (fix->model, row_members);
  dee_model_build_named_row (fix->model, row_members,
                             "name", "Mid", "count", 12,
                             "hints::id", 8123,
                             "hints2::id", "8123", NULL);
  dee_model_append_row (fix->model, row_members);
  dee_model_build_named_row (fix->model, row_members,
                             "count", 10, "name", "Last",
                             "hints2::id", "foo", NULL);
  dee_model_append_row (fix->model, row_members);

  /* Commence checks */
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_get (fix->model, iter, &i, &str, &dict1, &dict2);
  g_assert_cmpint (11, ==, i);
  g_assert_cmpstr (str, ==, "First");
  g_assert_cmpuint (0, ==, g_variant_n_children (dict1));
  g_assert_cmpuint (0, ==, g_variant_n_children (dict2));
  g_assert (dee_model_get_value_by_name (fix->model, iter, "hints::id") == NULL);

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str, &dict1, &dict2);
  g_assert_cmpint (12, ==, i);
  g_assert_cmpstr (str, ==, "Mid");
  g_assert_cmpuint (1, ==, g_variant_n_children (dict1));
  g_assert_cmpuint (1, ==, g_variant_n_children (dict2));
  g_assert_cmpint (8123, ==, g_variant_get_int32 (dee_model_get_value_by_name (fix->model, iter, "hints::id")));
  g_assert_cmpstr ("8123", ==, g_variant_get_string (dee_model_get_value_by_name (fix->model, iter, "hints2::id"), NULL));

  iter = dee_model_next (fix->model, iter);
  dee_model_get (fix->model, iter, &i, &str, &dict1, &dict2);
  g_assert_cmpint (10, ==, i);
  g_assert_cmpstr (str, ==, "Last");
  g_assert_cmpuint (0, ==, g_variant_n_children (dict1));
  g_assert_cmpuint (1, ==, g_variant_n_children (dict2));
  g_assert_cmpstr ("foo", ==, g_variant_get_string (dee_model_get_value_by_name (fix->model, iter, "hints2::id"), NULL));

  iter = dee_model_next (fix->model, iter);
  g_assert (dee_model_is_last (fix->model, iter));

  g_log_remove_handler ("dee", handler_id);
}

static void
test_named_cols_error (RowsFixture *fix, gconstpointer data)
{
  GVariant     *row_members[2];
  GVariant    **result;
  guint         handler_id;

  dee_model_set_column_names (fix->model, "count", "name", NULL);

  g_test_log_set_fatal_handler (expected_error_handler, NULL);
  handler_id = g_log_set_handler ("dee", G_LOG_LEVEL_CRITICAL |
                                  G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL,
                                  ignore_error_handler, NULL);

  /* Only first col set */
  result = dee_model_build_named_row (fix->model, row_members,
                                      "name", "First", NULL);
  g_assert (result == NULL);

  /* Only second col set */
  result = dee_model_build_named_row (fix->model, row_members,
                                      "count", 12, NULL);
  g_assert (result == NULL);

  /* Unregistered cols set */
  result = dee_model_build_named_row (fix->model, row_members,
                                      "nm", "Foo", "cnt", 12, NULL);
  g_assert (result == NULL);

  g_log_remove_handler ("dee", handler_id);
}

