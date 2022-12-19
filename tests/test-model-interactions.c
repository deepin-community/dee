/*
 * Copyright (C) 2009 Canonical Ltd
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
 * Authored by Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */

#include <glib.h>
#include <glib-object.h>
#include <dee.h>
#include <gtx.h>

#define TIMEOUT 500
#define MODEL_NAME "com.canonical.DeeModel.Tests.Interactions"

/* A command line that launches the appropriaye model-helper-* executable,
 * giving $name as first argument */
#define MODEL_HELPER(helper,name) \
  (gchar *[]) { "./model-helper-"#helper, name, NULL }


typedef struct
{
  DeeModel *model;

} Fixture;

static void model_setup         (Fixture *fix, gconstpointer data);
static void model_teardown      (Fixture *fix, gconstpointer data);
static void model_setup_null    (Fixture *fix, gconstpointer data);
static void model_teardown_null (Fixture *fix, gconstpointer data);
static void model_setup_with_meta (Fixture *fix, gconstpointer data);

static void test_ready          (Fixture *fix, gconstpointer data);
static void test_clone          (Fixture *fix, gconstpointer data);
static void test_clone_meta     (Fixture *fix, gconstpointer data);
static void test_row_added      (Fixture *fix, gconstpointer data);
static void test_row_changed    (Fixture *fix, gconstpointer data);
static void test_row_removed    (Fixture *fix, gconstpointer data);
static void test_model_clear    (Fixture *fix, gconstpointer data);
static void test_clear_add      (Fixture *fix, gconstpointer data);
static void test_clear_add_txn  (Fixture *fix, gconstpointer data);
static void test_clear_then_add (Fixture *fix, gconstpointer data);
static void test_add_clear      (Fixture *fix, gconstpointer data);
static void test_row_inserted   (Fixture *fix, gconstpointer data);
static void test_schemaless_leader (Fixture *fix, gconstpointer data);
static void test_introspect     (Fixture *fix, gconstpointer data);
static void test_ownership_stealing (Fixture *fix, gconstpointer data);
static void test_remote_append  (Fixture *fix, gconstpointer data);
static void test_disabled_writes (Fixture *fix, gconstpointer data);
static void test_commit_before_clone (Fixture *fix, gconstpointer data);
static void test_force_resync   (Fixture *fix, gconstpointer data);
static void test_manual_flush   (Fixture *fix, gconstpointer data);

void
test_model_interactions_create_suite (void)
{
#define DOMAIN "/Model/Interactions"

  g_test_add (DOMAIN"/Ready", Fixture, 0,
              model_setup, test_ready, model_teardown);
  g_test_add (DOMAIN"/Clone", Fixture, 0,
              model_setup, test_clone, model_teardown);
  g_test_add (DOMAIN"/Clone/WithMeta", Fixture, 0,
              model_setup_with_meta, test_clone_meta, model_teardown);
  g_test_add (DOMAIN"/RowAdded", Fixture, 0,
              model_setup, test_row_added, model_teardown);
  g_test_add (DOMAIN"/RowChanged", Fixture, 0,
              model_setup, test_row_changed, model_teardown);
  g_test_add (DOMAIN"/RowRemoved", Fixture, 0,
              model_setup, test_row_removed, model_teardown);
  g_test_add (DOMAIN"/Clear", Fixture, 0,
              model_setup, test_model_clear, model_teardown);
  g_test_add (DOMAIN"/ClearAndAdd", Fixture, 0,
              model_setup, test_clear_add, model_teardown);
  g_test_add (DOMAIN"/TxnClearAndAdd", Fixture, 0,
              model_setup, test_clear_add_txn, model_teardown);
  g_test_add (DOMAIN"/ClearThenAdd", Fixture, 0,
              model_setup, test_clear_then_add, model_teardown);
  g_test_add (DOMAIN"/AddAndClear", Fixture, 0,
              model_setup, test_add_clear, model_teardown);
  g_test_add (DOMAIN"/RowInserted", Fixture, 0,
              model_setup, test_row_inserted, model_teardown);
  g_test_add (DOMAIN"/SchemalessLeader", Fixture, 0,
              model_setup_null, test_schemaless_leader, model_teardown_null);
  g_test_add (DOMAIN"/Introspect", Fixture, 0,
              model_setup, test_introspect, model_teardown);
  g_test_add (DOMAIN"/OwnershipStealing", Fixture, 0,
              model_setup, test_ownership_stealing, model_teardown);
  g_test_add (DOMAIN"/RemoteAppend", Fixture, 0,
              model_setup, test_remote_append, model_teardown);
  g_test_add (DOMAIN"/DisabledWrites", Fixture, 0,
              model_setup_null, test_disabled_writes, model_teardown_null);
  g_test_add (DOMAIN"/CommitBeforeClone", Fixture, 0,
              model_setup, test_commit_before_clone, model_teardown);
  g_test_add (DOMAIN"/ForceResync", Fixture, 0,
              model_setup, test_force_resync, model_teardown);
  g_test_add (DOMAIN"/ManualFlush", Fixture, 0,
              model_setup, test_manual_flush, model_teardown);
}

static void
model_setup (Fixture *fix, gconstpointer data)
{
  fix->model = dee_shared_model_new (MODEL_NAME);
  dee_model_set_schema (fix->model, "i", "s", NULL);

  g_assert (DEE_IS_MODEL (fix->model));
}

static void
model_teardown (Fixture *fix, gconstpointer data)
{
  gtx_assert_last_unref (fix->model);
}

static void
model_setup_null (Fixture *fix, gconstpointer data)
{   
  fix->model = NULL;
}

static void
model_teardown_null (Fixture *fix, gconstpointer data)
{
  g_assert (fix->model == NULL);
}

static void
model_setup_with_meta (Fixture *fix, gconstpointer data)
{
  GHashTable *vardict_schema;

  fix->model = dee_shared_model_new (MODEL_NAME);
  dee_model_set_schema (fix->model, "i", "s", "a{sv}", "a{sv}", NULL);
  dee_model_set_column_names (fix->model, "count", "title", "hints", "extra-hints", NULL);

  vardict_schema = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (vardict_schema, "uri", "s");

  dee_model_register_vardict_schema (fix->model, 2, vardict_schema);

  g_assert (DEE_IS_MODEL (fix->model));

  g_hash_table_unref (vardict_schema);
}

static void
test_ready (Fixture *fix, gconstpointer data)
{
  GParamSpec *pspec;
  gboolean    synchronized;

  /* Test the GObject property reports FALSE */
  g_object_get (fix->model, "synchronized", &synchronized, NULL);
  g_assert_cmpint (0, == , synchronized);

  /* Check that convenience getter now reports FALSE */
  g_assert_cmpint (0, ==, dee_shared_model_is_synchronized (DEE_SHARED_MODEL (fix->model)));

  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", &pspec))
    g_critical ("Model never synchronized");
  else
    g_param_spec_unref (pspec);

  /* Test the GObject property reports TRUE */
  g_object_get (fix->model, "synchronized", &synchronized, NULL);
  g_assert_cmpint (0, != , synchronized);

  /* Check that convenience getter now reports TRUE */
  g_assert_cmpint (0, !=, dee_shared_model_is_synchronized (DEE_SHARED_MODEL (fix->model)));

  if (!gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    {
      g_critical ("Model changed synchronization state twice");
      g_param_spec_unref (pspec);
    }
}

static gboolean
_add3rows (DeeModel *model)
{
  g_return_val_if_fail (DEE_IS_MODEL (model), FALSE);

  dee_model_append (model, 0, "zero");
  dee_model_append (model, 1, "one");
  dee_model_append (model, 2, "two");
  return FALSE;
}

static gboolean
_add5rows (DeeModel *model)
{
  g_return_val_if_fail (DEE_IS_MODEL (model), FALSE);

  dee_model_append (model, 0, "zero");
  dee_model_append (model, 1, "one");
  dee_model_append (model, 2, "two");
  dee_model_append (model, 3, "three");
  dee_model_append (model, 4, "four");
  return FALSE;
}

static gboolean
_change3rows (DeeModel *model)
{
  DeeModelIter *iter;

  g_return_val_if_fail (DEE_IS_MODEL (model), FALSE);

  iter = dee_model_get_iter_at_row (model, 0);
  dee_model_set_value (model, iter, 1, g_variant_new_string ("changed_zero"));

  iter = dee_model_get_iter_at_row (model, 1);
  dee_model_set_value (model, iter, 1, g_variant_new_string ("changed_one"));

  iter = dee_model_get_iter_at_row (model, 2);
  dee_model_set_value (model, iter, 1, g_variant_new_string ("changed_two"));
  
  return FALSE;
}

/* Assumes a model with 5 rows. Removes rows 0, 4, and 2
 * in that order. Accounting for rows shifts this becomes
 * 0, 3, and 1. Leaving the original rows 1 and 3 now in
 * positions 0 and 1 */
static gboolean
_remove3rows (DeeModel *model)
{
  DeeModelIter *iter0, *iter4, *iter2;
  DeeModelIter *orig_iter1, *orig_iter3;

  g_return_val_if_fail (DEE_IS_MODEL (model), FALSE);
  g_return_val_if_fail (dee_model_get_n_rows (model) == 5, FALSE);

  iter0 = dee_model_get_iter_at_row (model, 0);
  iter4 = dee_model_get_iter_at_row (model, 4);
  iter2 = dee_model_get_iter_at_row (model, 2);

  orig_iter1 = dee_model_get_iter_at_row (model, 1);
  orig_iter3 = dee_model_get_iter_at_row (model, 3);

  dee_model_remove (model, iter0);
  dee_model_remove (model, iter4);
  dee_model_remove (model, iter2);

  g_assert_cmpint (dee_model_get_n_rows (model), ==, 2);
  g_assert (dee_model_get_iter_at_row (model, 0) == orig_iter1);
  g_assert (dee_model_get_iter_at_row (model, 1) == orig_iter3);

  return FALSE;
}

static gboolean
_insert1row (DeeModel *model)
{
  g_return_val_if_fail (DEE_IS_MODEL (model), FALSE);

  dee_model_insert (model, 1, 27, "twentyseven");
  return FALSE;
}

static gboolean
_clear_model (DeeModel *model)
{
  g_return_val_if_fail (DEE_IS_MODEL (model), FALSE);

  dee_model_clear (model);
  
  return FALSE;
}

static gboolean
_add_and_clear6rows (DeeModel *model)
{
  g_return_val_if_fail (DEE_IS_MODEL (model), FALSE);

  _add3rows (model);
  _clear_model (model);

  return FALSE;
}

static gboolean
_clear_and_add5rows (DeeModel *model)
{
  g_return_val_if_fail (DEE_IS_MODEL (model), FALSE);

  _clear_model (model);
  _add5rows (model);

  return FALSE;
}

static gboolean
_txn_clear_and_add5rows (DeeModel *model)
{
  DeeTransaction *txn;
  GError         *error;

  g_return_val_if_fail (DEE_IS_MODEL (model), FALSE);
  
  
  txn = DEE_TRANSACTION (dee_transaction_new (model));
  
  _clear_model (DEE_MODEL (txn));
  _add5rows (DEE_MODEL (txn));
  
  error = NULL;
  dee_transaction_commit (txn, &error);
  
  if (error)
    {
      g_critical ("Failed to commit clear-add5 transaction: %s", error->message);
    }
  
  g_object_unref (txn);

  return FALSE;
}

static void
test_clone (Fixture *fix, gconstpointer data)
{ 
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never synchronized");

  _add3rows (fix->model);

  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (clone3rows, MODEL_NAME),
                            1000))
    g_critical ("Model helper timed out");

  gtx_assert_last_command_status (0);

  /* We test that we can do this two times */
  /*if (gtx_wait_for_command (TESTDIR, MODEL_HELPER (3rows, MODEL_NAME), 1000))
    g_critical ("Model helper timed out");

  gtx_assert_last_command_status (0);*/
}

static void
test_clone_meta (Fixture *fix, gconstpointer data)
{
  GVariant *row_buf[3];
  DeeModel *model;

  model = fix->model;

  if (gtx_wait_for_signal (G_OBJECT (model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never synchronized");

  dee_model_append_row (model,
      dee_model_build_named_row (model, row_buf,
        "count", 0, "title", "zero", NULL));
  dee_model_append_row (model,
      dee_model_build_named_row (model, row_buf,
        "count", 1, "title", "one", NULL));
  dee_model_append_row (model,
      dee_model_build_named_row (model, row_buf,
        "count", 2, "title", "two", NULL));

  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (clone3rows-meta, MODEL_NAME),
                            1000))
    g_critical ("Model helper timed out");

  gtx_assert_last_command_status (0);
}

static void
test_row_added (Fixture *fix, gconstpointer data)
{ 
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  g_timeout_add (500, (GSourceFunc)_add3rows, fix->model);

  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (add3rows, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");

  gtx_assert_last_command_status (0);
}

static void
test_row_changed (Fixture *fix, gconstpointer data)
{ 
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  _add3rows (fix->model);
  g_timeout_add (500, (GSourceFunc)_change3rows, fix->model);

  //const gchar *cmd[] = {"dbus-monitor", NULL};
  //const gchar *cmd[] = {"sleep", "1",NULL};
  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (change3rows, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");
  
  gtx_assert_last_command_status (0);  
}

static void
test_row_removed (Fixture *fix, gconstpointer data)
{
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  _add5rows (fix->model);
  g_timeout_add (500, (GSourceFunc)_remove3rows, fix->model);

  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (remove3rows, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");

  gtx_assert_last_command_status (0);
}

static void
test_model_clear (Fixture *fix, gconstpointer data)
{ 
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  _add3rows (fix->model);
  g_timeout_add (500, (GSourceFunc)_clear_model, fix->model);
  
  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (clear3rows, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");
  
  gtx_assert_last_command_status (0);  
}

static void
test_clear_add (Fixture *fix, gconstpointer data)
{
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  g_timeout_add (500, (GSourceFunc)_add3rows, fix->model);
  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (add3rows, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");
  
  gtx_assert_last_command_status (0);

  g_timeout_add (500, (GSourceFunc)_clear_and_add5rows, fix->model);
  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (clear3add5, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");
  
  gtx_assert_last_command_status (0);
}

static void
test_clear_add_txn (Fixture *fix, gconstpointer data)
{
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  g_timeout_add (500, (GSourceFunc)_add3rows, fix->model);
  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (add3rows, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");
  
  gtx_assert_last_command_status (0);

  g_timeout_add (500, (GSourceFunc)_txn_clear_and_add5rows, fix->model);
  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (clear3add5, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");
  
  /* Test that the local model looks as expected */
  DeeModelIter *iter = dee_model_get_first_iter (fix->model);
  g_assert_cmpint (dee_model_get_int32 (fix->model, iter, 0), ==, 0);
  g_assert_cmpstr (dee_model_get_string (fix->model, iter, 1), ==, "zero");
  
  iter = dee_model_next (fix->model, iter);
  g_assert_cmpint (dee_model_get_int32 (fix->model, iter, 0), ==, 1);
  g_assert_cmpstr (dee_model_get_string (fix->model, iter, 1), ==, "one");
  
  iter = dee_model_next (fix->model, iter);
  g_assert_cmpint (dee_model_get_int32 (fix->model, iter, 0), ==, 2);
  g_assert_cmpstr (dee_model_get_string (fix->model, iter, 1), ==, "two");
  
  iter = dee_model_next (fix->model, iter);
  g_assert_cmpint (dee_model_get_int32 (fix->model, iter, 0), ==, 3);
  g_assert_cmpstr (dee_model_get_string (fix->model, iter, 1), ==, "three");
  
  iter = dee_model_next (fix->model, iter);
  g_assert_cmpint (dee_model_get_int32 (fix->model, iter, 0), ==, 4);
  g_assert_cmpstr (dee_model_get_string (fix->model, iter, 1), ==, "four");
  
  gtx_assert_last_command_status (0);
}

static void
test_clear_then_add (Fixture *fix, gconstpointer data)
{ 
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  _clear_model (fix->model);
  g_timeout_add (500, (GSourceFunc)_add3rows, fix->model);
  
  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (add3rows, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");
  
  gtx_assert_last_command_status (0);
  
  g_timeout_add (500, (GSourceFunc)_clear_model, fix->model);
  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (clear3rows, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");
  
  gtx_assert_last_command_status (0);
}

static void
test_add_clear (Fixture *fix, gconstpointer data)
{
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  g_timeout_add (500, (GSourceFunc)_add3rows, fix->model);
  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (add3rows, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");
  
  gtx_assert_last_command_status (0);

  g_timeout_add (500, (GSourceFunc)_add_and_clear6rows, fix->model);
  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (clear6rows, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");
  
  gtx_assert_last_command_status (0);
}

static void
test_row_inserted (Fixture *fix, gconstpointer data)
{ 
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  _add3rows (fix->model);
  g_timeout_add (500, (GSourceFunc)_insert1row, fix->model);
  
  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (insert1row, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");

  gtx_assert_last_command_status (0);
}

static void
_ready (DeeModel *model, GParamSpec *pspec, GSList **rows_so_far)
{
  /* We must not have any rows when 'ready' is emitted */
  g_assert (*rows_so_far == NULL);
}

static void
_collect_row (DeeModel *model, DeeModelIter *iter, GSList **rows_so_far)
{
  /* Yes, I _know_ that append() is slow, but this is a test! */
  *rows_so_far = g_slist_append (*rows_so_far, iter);
}

/* This case must run without a Fixture  */
static void
test_schemaless_leader (Fixture *fix, gconstpointer data)
{
  DeeModel     *model;
  DeeModelIter *iter;
  GSList        *rows_added;
  
  g_assert (fix->model == NULL);

  /* Set up a clean model *without* a schema. We will pick up the schema
   * with the first transaction from the slave. Or at least, that's what we
   * want to assert ;-) */
  model = dee_shared_model_new (MODEL_NAME);
  // no set_schema() on purpose!

  /* Listen for changes */
  rows_added = NULL;
  g_signal_connect (model, "row-added", G_CALLBACK (_collect_row), &rows_added);
  g_signal_connect (model, "notify::synchronized", G_CALLBACK (_ready), &rows_added);
  
  /* Remote process defines column types and adds two rows */
  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (schemaless, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");

  /* Check that we got the right schema from the peer */
  g_assert_cmpint (dee_model_get_n_columns (model), ==, 2);
  g_assert_cmpstr (dee_model_get_column_schema (model, 0), ==, "i");
  g_assert_cmpstr (dee_model_get_column_schema (model, 1), ==, "s");

  /* Check that we got what we expected */
  g_assert_cmpint (g_slist_length (rows_added), == , 2);
  g_assert_cmpint (dee_model_get_n_rows (model), ==, 2);
  g_assert_cmpint (dee_model_get_n_columns (model), ==, 2);

  iter = (DeeModelIter*) g_slist_nth (rows_added, 0)->data;
  g_assert_cmpint (dee_model_get_position (model, iter), == , 0);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), == , 27);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), == , "skunkworks");

  iter = (DeeModelIter*) g_slist_nth (rows_added, 1)->data;
  g_assert_cmpint (dee_model_get_position (model, iter), == , 1);
  g_assert_cmpint (dee_model_get_int32 (model, iter, 0), == , 68);
  g_assert_cmpstr (dee_model_get_string (model, iter, 1), == , "wumbo");

  gtx_assert_last_unref (model);
  g_slist_free (rows_added);
}

static void
test_introspect (Fixture *fix, gconstpointer data)
{
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (introspect, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");

  gtx_assert_last_command_status (0);
}

static void
stealing_notify (GObject *object, GParamSpec *pspec, Fixture *fix)
{
  g_assert (DEE_IS_PEER (object));

  if (!dee_peer_is_swarm_leader (DEE_PEER (object)))
  {
    g_object_set_data (G_OBJECT (fix->model), "stealing-success",
                       GINT_TO_POINTER (1));
  }
}

static void
test_ownership_stealing (Fixture *fix, gconstpointer data)
{
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  /* We should be leader before starting the helper process */
  g_assert (dee_shared_model_is_leader (DEE_SHARED_MODEL (fix->model)));
  /* But loose it later on */
  g_signal_connect (dee_shared_model_get_peer (DEE_SHARED_MODEL (fix->model)),
                    "notify::swarm-leader", G_CALLBACK (stealing_notify), fix);

  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (replace, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");

  gtx_assert_last_command_status (0);
  gtx_flush_sources (FALSE);

  g_assert (g_object_get_data (G_OBJECT (fix->model), "stealing-success"));
  g_assert_cmpuint (dee_model_get_n_rows (fix->model), >, 0);
}

static void
changeset_signal (DeeModel *model, gboolean *value)
{
  *value = TRUE;
}

static void
test_remote_append (Fixture *fix, gconstpointer data)
{
  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  /* We should be leader before starting the helper process */
  g_assert (dee_shared_model_is_leader (DEE_SHARED_MODEL (fix->model)));
  g_assert_cmpuint (dee_model_get_n_rows (fix->model), ==, 0);

  gboolean got_changeset_start = FALSE;
  gboolean got_changeset_finish = FALSE;
  g_signal_connect (fix->model, "changeset-started",
                    G_CALLBACK (changeset_signal), &got_changeset_start);
  g_signal_connect (fix->model, "changeset-finished",
                    G_CALLBACK (changeset_signal), &got_changeset_finish);

  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (append1, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");

  gtx_assert_last_command_status (0);

  /* There should be a new row in the model */
  g_assert_cmpuint (dee_model_get_n_rows (fix->model), ==, 1);
  g_assert (got_changeset_start);
  g_assert (got_changeset_finish);
}

static void
test_disabled_writes (Fixture *fix, gconstpointer data)
{
  DeePeer *peer;
  DeeModelIter *iter;

  peer = dee_peer_new (MODEL_NAME);
  fix->model = DEE_MODEL (
      g_object_new (DEE_TYPE_SHARED_MODEL, 
                    "peer", peer,
                    "back-end", dee_sequence_model_new (),
                    "access-mode", DEE_SHARED_MODEL_ACCESS_MODE_LEADER_WRITABLE,
                    NULL));
  dee_model_set_schema (fix->model, "i", "s", NULL);

  g_object_unref (G_OBJECT (peer));

  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  /* We should be leader before starting the helper process */
  g_assert (dee_shared_model_is_leader (DEE_SHARED_MODEL (fix->model)));
  dee_model_prepend (fix->model, 81, "eightyone");

  if (gtx_wait_for_command (TESTDIR,
                            MODEL_HELPER (append1, MODEL_NAME),
                            2000))
    g_critical ("Model helper timed out");

  gtx_assert_last_command_status (0);

  /* The peer tried to append a row, but we should have ignored that */
  g_assert_cmpuint (dee_model_get_n_rows (fix->model), ==, 1);
  iter = dee_model_get_first_iter (fix->model);
  g_assert_cmpstr (dee_model_get_string (fix->model, iter, 1), ==, "eightyone");

  gtx_assert_last_unref (fix->model);
  fix->model = NULL;
}

static gboolean
quit_loop (GMainLoop *ml)
{
  g_main_loop_quit (ml);
  return FALSE;
}

static void
child_quit (GPid pid, gint status, gpointer user_data)
{
  GMainLoop *ml;
  gpointer *data;

  data = user_data;
  ml = (GMainLoop*) data[0];
  data[1] = GINT_TO_POINTER (status);

  g_main_loop_quit (ml);
}

static void
test_commit_before_clone (Fixture *fix, gconstpointer data)
{
  GPid pid;
  GMainLoop *ml;
  gpointer *wait_data;

  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  _add3rows (fix->model);

  /* need special handling of synchronization - commit must be emmitted after
   * the client is up (and possibly waiting for clone) */

  g_spawn_async (TESTDIR,
                 MODEL_HELPER (clone3rows, MODEL_NAME),
                 NULL,
                 G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
                 NULL,
                 NULL,
                 &pid,
                 NULL);

  ml = g_main_loop_new (NULL, FALSE);
  wait_data = g_new0 (gpointer, 2);
  wait_data[0] = ml;
  wait_data[1] = GINT_TO_POINTER (837);

  g_child_watch_add (pid, child_quit, wait_data);

  /* wait for the helper process to start up, then flush the commits */
  g_usleep (500000);

  /* don't wait indefinitely for the child */
  g_timeout_add (5000, (GSourceFunc) quit_loop, ml);

  g_main_loop_run (ml);

  if (GPOINTER_TO_INT (wait_data[1]) == 837)
  {
    g_critical ("Model helper timed out");
  }
  else if (GPOINTER_TO_INT (wait_data[1]) != 0)
  {
    g_critical ("Model helper returned error");
  }

  g_assert_cmpint (dee_model_get_n_rows (fix->model), ==, 3);
}

static void
test_force_resync (Fixture *fix, gconstpointer data)
{
  GPid pid;
  GMainLoop *ml;
  gpointer *wait_data;
  DeePeer *leader_peer;

  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  _add3rows (fix->model);

  /* need special handling of synchronization */

  g_spawn_async (TESTDIR,
                 MODEL_HELPER (resync3rows, MODEL_NAME),
                 NULL,
                 G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
                 NULL,
                 NULL,
                 &pid,
                 NULL);

  ml = g_main_loop_new (NULL, FALSE);
  wait_data = g_new0 (gpointer, 2);
  wait_data[0] = ml;
  wait_data[1] = GINT_TO_POINTER (837);

  g_child_watch_add (pid, child_quit, wait_data);

  /* We don't really have a better way atm. Peer discovery doesn't work
   * with latest changes done to DBus (no watching of method calls from other
   * processes) */
  gtx_yield_main_loop (500); /* 500ms yield */

  /* Peer should be synced now, let's throw away our model */
  gtx_assert_last_unref (fix->model);

  gtx_yield_main_loop (50); /* 50ms yield */

  /* And re-own it */
  leader_peer = DEE_PEER (g_object_new (DEE_TYPE_PEER,
                                        "swarm-name", MODEL_NAME,
                                        "swarm-owner", TRUE, NULL));
  fix->model = dee_shared_model_new_for_peer (leader_peer);
  dee_model_set_schema (fix->model, "i", "s", NULL);
  g_assert (DEE_IS_MODEL (fix->model));

  if (gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", NULL))
    g_critical ("Model never emitted 'ready' signal");

  _add3rows (fix->model);

  /* don't wait indefinitely for the child */
  g_timeout_add (5000, (GSourceFunc) quit_loop, ml);

  g_main_loop_run (ml);

  if (GPOINTER_TO_INT (wait_data[1]) == 837)
  {
    g_critical ("Model helper timed out");
  }
  else if (GPOINTER_TO_INT (wait_data[1]) != 0)
  {
    g_critical ("Model helper returned error");
  }

  g_assert_cmpint (dee_model_get_n_rows (fix->model), ==, 3);
}

static void
test_manual_flush (Fixture *fix, gconstpointer data)
{
  DeeSharedModel *sm;
  sm = DEE_SHARED_MODEL (fix->model);
  dee_shared_model_set_flush_mode (sm, DEE_SHARED_MODEL_FLUSH_MODE_MANUAL);
  _add5rows (fix->model);

  /* A timeout would normally cause the model to flush, but with manual
   * flushing it shouldn't */
  gtx_yield_main_loop (50); /* 50ms yield */

  g_assert_cmpuint (dee_shared_model_flush_revision_queue_sync (sm), >, 0);
}

