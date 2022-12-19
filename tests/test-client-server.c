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
 * Authored by Michal Hruby <michal.hruby@canonical.com>
 *
 */

#include <glib.h>
#include <glib-object.h>
#include <dee.h>
#include <gtx.h>

#define TIMEOUT 100
#define PEER_NAME "com.canonical.Dee.Peer.Tests.Interactions"
#define MODEL_NAME "com.canonical.Dee.Peer.Tests.Interactions"

/* A command line that launches the appropriate *-helper-* executable,
 * giving $name as first argument */
#define SERVER_HELPER(helper,name,count) \
  (gchar *[]) { "./server-helper-"#helper, name, #count, NULL }

#define MODEL_HELPER(helper,name) \
  (gchar *[]) { "./model-helper-"#helper, name, "DeeClient", NULL }

typedef struct
{
  DeeServer *server;

} ServerFixture;

typedef struct
{
  DeeModel *model;
} Fixture;

static void server_setup         (ServerFixture *fix, gconstpointer data);
static void server_teardown      (ServerFixture *fix, gconstpointer data);
static void model_setup          (Fixture *fix, gconstpointer data);
static void model_teardown       (Fixture *fix, gconstpointer data);
static void model_setup_null     (Fixture *fix, gconstpointer data);
static void model_teardown_null  (Fixture *fix, gconstpointer data);

static void test_allocation           (ServerFixture *fix, gconstpointer data);
static void test_become_leader        (ServerFixture *fix, gconstpointer data);
static void test_valid_client_address (ServerFixture *fix, gconstpointer data);
static void test_one_client           (ServerFixture *fix, gconstpointer data);
static void test_multiple_clients     (ServerFixture *fix, gconstpointer data);
static void test_shared_server        (ServerFixture *fix, gconstpointer data);

static void test_ready          (Fixture *fix, gconstpointer data);
static void test_clone          (Fixture *fix, gconstpointer data);
static void test_row_added      (Fixture *fix, gconstpointer data);
static void test_row_changed    (Fixture *fix, gconstpointer data);
static void test_row_removed    (Fixture *fix, gconstpointer data);
static void test_model_clear    (Fixture *fix, gconstpointer data);
static void test_clear_add      (Fixture *fix, gconstpointer data);
static void test_clear_then_add (Fixture *fix, gconstpointer data);
static void test_row_inserted   (Fixture *fix, gconstpointer data);
static void test_schemaless_leader (Fixture *fix, gconstpointer data);

static void test_client_commit    (Fixture *fix, gconstpointer data);
static void test_multiple_models  (Fixture *fix, gconstpointer data);
static void test_multiple_models2 (Fixture *fix, gconstpointer data);
static void test_remote_append    (Fixture *fix, gconstpointer data);
static void test_disabled_writes  (Fixture *fix, gconstpointer data);

void
test_client_server_interactions_create_suite (void)
{
#define DOMAIN "/ClientServer/Interactions"

  g_test_add (DOMAIN"/Allocation", ServerFixture, 0,
              server_setup, test_allocation, server_teardown);
  g_test_add (DOMAIN"/BecomeLeader", ServerFixture, 0,
              server_setup, test_become_leader, server_teardown);
  g_test_add (DOMAIN"/ValidClientAddress", ServerFixture, 0,
              server_setup, test_valid_client_address, server_teardown);
  g_test_add (DOMAIN"/OneClient", ServerFixture, 0,
              server_setup, test_one_client, server_teardown);
  g_test_add (DOMAIN"/MultipleClients", ServerFixture, 0,
              server_setup, test_multiple_clients, server_teardown);
  g_test_add (DOMAIN"/SharedServer", ServerFixture, 0,
              server_setup, test_shared_server, server_teardown);

#undef DOMAIN
#define DOMAIN "/ClientServer/Model/Interactions"

  g_test_add (DOMAIN"/Ready", Fixture, 0,
              model_setup, test_ready, model_teardown);
  g_test_add (DOMAIN"/Clone", Fixture, 0,
              model_setup, test_clone, model_teardown);
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
  g_test_add (DOMAIN"/ClearThenAdd", Fixture, 0,
              model_setup, test_clear_then_add, model_teardown);
  g_test_add (DOMAIN"/RowInserted", Fixture, 0,
              model_setup, test_row_inserted, model_teardown);
  g_test_add (DOMAIN"/SchemalessLeader", Fixture, 0,
              model_setup_null, test_schemaless_leader, model_teardown_null);
  g_test_add (DOMAIN"/ClientCommit", Fixture, 0,
              model_setup, test_client_commit, model_teardown);
  g_test_add (DOMAIN"/MultipleModels", Fixture, 0,
              model_setup_null, test_multiple_models, model_teardown_null);
  g_test_add (DOMAIN"/MultipleModels2", Fixture, 0,
              model_setup_null, test_multiple_models2, model_teardown_null);
  g_test_add (DOMAIN"/RemoteAppend", Fixture, 0,
              model_setup, test_remote_append, model_teardown);
  g_test_add (DOMAIN"/DisabledWrites", Fixture, 0,
              model_setup_null, test_disabled_writes, model_teardown_null);
}

static void
server_setup (ServerFixture *fix, gconstpointer data)
{
  fix->server = dee_server_new (PEER_NAME);

  g_assert_cmpint (0, ==, dee_peer_is_swarm_leader (DEE_PEER (fix->server)));

  g_assert (DEE_IS_SERVER (fix->server));
}

static void
server_teardown (ServerFixture *fix, gconstpointer data)
{
  gtx_assert_last_unref (fix->server);

  /* Spin the mainloop a bit to check if we have any post-test
   * async effect crashing us */
  gtx_yield_main_loop (200);
}

static void
model_setup (Fixture *fix, gconstpointer data)
{
  DeeServer *server = dee_server_new (MODEL_NAME);

  fix->model = (DeeModel*) dee_shared_model_new_for_peer (DEE_PEER (server));
  dee_model_set_schema (fix->model, "i", "s", NULL);

  g_assert (DEE_IS_MODEL (fix->model));
}

static void
model_teardown (Fixture *fix, gconstpointer data)
{
  gtx_assert_last_unref (fix->model);

  /* Spin the mainloop so the socket service gets into usable state again */
  gtx_yield_main_loop (200);
}

static void
model_setup_null (Fixture *fix, gconstpointer data)
{
}

static void
model_teardown_null (Fixture *fix, gconstpointer data)
{
}

/******************** The actual test cases ********************/

static void
test_allocation (ServerFixture *fix, gconstpointer data)
{
  /* Do nothing, this test basically just asserts that
   * the fix->server is cleaned up after immediate construction */
}

static void
test_become_leader (ServerFixture *fix, gconstpointer data)
{
  gtx_wait_for_signal (G_OBJECT (fix->server), TIMEOUT,
                       "notify::swarm-leader", NULL);

  /* Assert that we have become swarm leaders.
   * No other peers should be running */
  g_assert_cmpint (0, !=, dee_peer_is_swarm_leader (DEE_PEER (fix->server)));
}

static void
test_valid_client_address (ServerFixture *fix, gconstpointer data)
{
  gtx_wait_for_signal (G_OBJECT (fix->server), TIMEOUT,
                       "notify::swarm-leader", NULL);

  g_assert (dee_server_get_client_address (fix->server) != NULL);
}

static void
on_connection_acquired (DeeServer *server, GDBusConnection *connection, 
                        GSList **list)
{
  /* Yes, I _know_ that append() is slow, but this is a test! */
  *list = g_slist_append (*list, connection);
}

static void
test_one_client (ServerFixture *fix, gconstpointer data)
{
  GSList *list = NULL;

  g_signal_connect (fix->server, "connection-acquired",
                    G_CALLBACK (on_connection_acquired), &list);

  gtx_wait_for_signal (G_OBJECT (fix->server), TIMEOUT,
                       "notify::swarm-leader", NULL);

  /* We are leaders - launch the helper */
  if (gtx_wait_for_command (TESTDIR,
                            SERVER_HELPER (client, PEER_NAME, 1),
                            2000))
    g_critical ("Peer helper timed out");

  gtx_assert_last_command_status (0);

  g_assert_cmpuint (g_slist_length (list), ==, 1);

  g_slist_free (list);
}

static void
test_multiple_clients (ServerFixture *fix, gconstpointer data)
{
  GSList *list = NULL;

  g_signal_connect (fix->server, "connection-acquired",
                    G_CALLBACK (on_connection_acquired), &list);

  gtx_wait_for_signal (G_OBJECT (fix->server), TIMEOUT,
                       "notify::swarm-leader", NULL);

  /* We are leaders - launch the helper */
  if (gtx_wait_for_command (TESTDIR,
                            SERVER_HELPER (client, PEER_NAME, 4),
                            4000))
    g_critical ("Peer helper timed out");

  gtx_assert_last_command_status (0);

  g_assert_cmpuint (g_slist_length (list), ==, 4);

  g_slist_free (list);
}

static void
test_shared_server (ServerFixture *fix, gconstpointer data)
{
  DeeServer *another_server;

  gtx_wait_for_signal (G_OBJECT (fix->server), TIMEOUT,
                       "notify::swarm-leader", NULL);

  /* Make sure we're able to use the same bus_address on multiple servers */
  another_server = dee_server_new_for_address (
      PEER_NAME ".T1", dee_server_get_client_address (fix->server));

  gtx_wait_for_signal (G_OBJECT (another_server), TIMEOUT,
                       "notify::swarm-leader", NULL);

  g_assert_cmpstr (dee_peer_get_swarm_leader (DEE_PEER (fix->server)), ==,
                   dee_peer_get_swarm_leader (DEE_PEER (another_server)));
  g_assert_cmpstr (dee_server_get_client_address (fix->server), ==,
                   dee_server_get_client_address (another_server));

  g_object_unref (another_server);
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

  if (!gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT, "notify::synchronized", &pspec))
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
_clear_and_add5rows (DeeModel *model)
{
  g_return_val_if_fail (DEE_IS_MODEL (model), FALSE);

  _clear_model (model);
  _add5rows (model);

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
  model = dee_shared_model_new_for_peer (DEE_PEER (dee_server_new (MODEL_NAME)));
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

  /* Spin the mainloop so the socket service gets into usable state again */
  gtx_yield_main_loop (200);
}

static void
test_client_commit (Fixture *fix, gconstpointer data)
{
  gtx_wait_for_signal (G_OBJECT (fix->model), TIMEOUT,
                       "notify::synchronized", NULL);

  DeeModel *client_model1 = dee_shared_model_new_for_peer (
      DEE_PEER (dee_client_new (MODEL_NAME)));
  DeeModel *client_model2 = dee_shared_model_new_for_peer (
      DEE_PEER (dee_client_new (MODEL_NAME)));

  gtx_wait_for_signal (G_OBJECT (client_model1), TIMEOUT,
                       "notify::synchronized", NULL);

  g_assert (dee_shared_model_is_synchronized (DEE_SHARED_MODEL (client_model1)));

  if (!dee_shared_model_is_synchronized (DEE_SHARED_MODEL (client_model2)))
  {
    gtx_wait_for_signal (G_OBJECT (client_model2), TIMEOUT, 
                         "notify::synchronized", NULL);
  }

  dee_model_append (client_model1, 38, "client_change");

  gtx_yield_main_loop (500);

  g_assert_cmpuint (dee_model_get_n_rows (fix->model), >, 0);
  g_assert_cmpuint (dee_model_get_n_rows (client_model1), >, 0);
  g_assert_cmpuint (dee_model_get_n_rows (client_model2), >, 0);

  g_object_unref (client_model1);
  g_object_unref (client_model2);
}

static void
test_multiple_models (Fixture *fix, gconstpointer data)
{
  gchar *address = dee_server_bus_address_for_name (PEER_NAME, TRUE);

  DeeModel *model1 = dee_shared_model_new_for_peer (
      DEE_PEER (dee_server_new_for_address (PEER_NAME ".T1", address))
  );
  DeeModel *model2 = dee_shared_model_new_for_peer (
      DEE_PEER (dee_server_new_for_address (PEER_NAME ".T2", address))
  );
  dee_model_set_schema (model1, "i", "s", NULL);
  dee_model_set_schema (model2, "i", "s", NULL);
  
  gtx_wait_for_signal (G_OBJECT (model1), TIMEOUT, 
                       "notify::synchronized", NULL);

  if (!dee_shared_model_is_synchronized (DEE_SHARED_MODEL (model2)))
  {
    gtx_wait_for_signal (G_OBJECT (model2), TIMEOUT, 
                         "notify::synchronized", NULL);
  }

  g_assert (dee_shared_model_is_synchronized (DEE_SHARED_MODEL (model1)) &&
            dee_shared_model_is_synchronized (DEE_SHARED_MODEL (model2)));

  DeeModel *client_model1 = dee_shared_model_new_for_peer (
      DEE_PEER (dee_client_new_for_address (PEER_NAME ".T1", address)));
  DeeModel *client_model2 = dee_shared_model_new_for_peer (
      DEE_PEER (dee_client_new_for_address (PEER_NAME ".T2", address)));

  _add5rows (model1);
  _add3rows (model2);

  gtx_wait_for_signal (G_OBJECT (client_model1), TIMEOUT,
                       "notify::synchronized", NULL);

  g_assert_cmpuint (dee_model_get_n_rows (client_model1), ==, 5);
  g_assert_cmpuint (dee_model_get_n_rows (model1), ==, 5);

  if (!dee_shared_model_is_synchronized (DEE_SHARED_MODEL (client_model2)))
  {
    gtx_wait_for_signal (G_OBJECT (client_model2), TIMEOUT,
                         "notify::synchronized", NULL);
  }

  g_assert_cmpuint (dee_model_get_n_rows (client_model2), ==, 3);
  g_assert_cmpuint (dee_model_get_n_rows (model2), ==, 3);

  /* Make sure the first model still has 5 rows */
  g_assert_cmpuint (dee_model_get_n_rows (client_model1), ==, 5);
  g_assert_cmpuint (dee_model_get_n_rows (model1), ==, 5);

  g_object_unref (client_model1);
  g_object_unref (client_model2);
  g_object_unref (model1);
  g_object_unref (model2);

  g_free (address);

  /* Spin the mainloop so the socket service gets into usable state again */
  gtx_yield_main_loop (200);
}

static void
test_multiple_models2 (Fixture *fix, gconstpointer data)
{
  gchar *address = dee_server_bus_address_for_name (PEER_NAME, TRUE);

  DeeModel *model1 = dee_shared_model_new_for_peer (
      DEE_PEER (dee_server_new_for_address (PEER_NAME ".T1", address))
  );
  DeeModel *model2 = dee_shared_model_new_for_peer (
      DEE_PEER (dee_server_new_for_address (PEER_NAME ".T2", address))
  );
  dee_model_set_schema (model1, "i", "s", NULL);
  dee_model_set_schema (model2, "i", "s", NULL);
  
  gtx_wait_for_signal (G_OBJECT (model1), TIMEOUT, 
                       "notify::synchronized", NULL);

  if (!dee_shared_model_is_synchronized (DEE_SHARED_MODEL (model2)))
  {
    gtx_wait_for_signal (G_OBJECT (model2), TIMEOUT, 
                         "notify::synchronized", NULL);
  }

  g_assert (dee_shared_model_is_synchronized (DEE_SHARED_MODEL (model1)) &&
            dee_shared_model_is_synchronized (DEE_SHARED_MODEL (model2)));

  DeeModel *client_model1 = dee_shared_model_new_for_peer (
      DEE_PEER (dee_client_new_for_address (PEER_NAME ".T1", address)));
  DeeModel *client_model2 = dee_shared_model_new_for_peer (
      DEE_PEER (dee_client_new_for_address (PEER_NAME ".T2", address)));

  _add5rows (model1);
  _add3rows (model2);

  gtx_wait_for_signal (G_OBJECT (client_model1), TIMEOUT,
                       "notify::synchronized", NULL);

  g_assert_cmpuint (dee_model_get_n_rows (client_model1), ==, 5);
  g_assert_cmpuint (dee_model_get_n_rows (model1), ==, 5);

  if (!dee_shared_model_is_synchronized (DEE_SHARED_MODEL (client_model2)))
  {
    gtx_wait_for_signal (G_OBJECT (client_model2), TIMEOUT,
                         "notify::synchronized", NULL);
  }

  g_assert_cmpuint (dee_model_get_n_rows (client_model2), ==, 3);
  g_assert_cmpuint (dee_model_get_n_rows (model2), ==, 3);

  /* Make sure the first model still has 5 rows */
  g_assert_cmpuint (dee_model_get_n_rows (client_model1), ==, 5);
  g_assert_cmpuint (dee_model_get_n_rows (model1), ==, 5);

  /* get rid of the first client and server */
  g_object_unref (client_model1);
  g_object_unref (model1);

  /* and make sure the second ones still work fine */
  dee_model_clear (model2);

  gtx_yield_main_loop (500); /* sync */

  g_assert_cmpuint (dee_model_get_n_rows (client_model2), ==, 0);
  g_assert_cmpuint (dee_model_get_n_rows (model2), ==, 0);

  g_object_unref (client_model2);
  g_object_unref (model2);

  g_free (address);

  /* Spin the mainloop so the socket service gets into usable state again */
  gtx_yield_main_loop (200);
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

  peer = DEE_PEER (dee_server_new (MODEL_NAME));
  fix->model = DEE_MODEL (g_object_new (DEE_TYPE_SHARED_MODEL,
        "peer", peer,
        "back-end", dee_sequence_model_new (),
        "access-mode", DEE_SHARED_MODEL_ACCESS_MODE_LEADER_WRITABLE, NULL));
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
}

