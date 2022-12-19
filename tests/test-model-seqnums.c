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
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include <glib.h>
#include <glib-object.h>
#include <dee.h>

typedef struct 
{
  DeeModel *model;

} SeqnumFixture;

static void setup          (SeqnumFixture *fix, gconstpointer data);
static void teardown       (SeqnumFixture *fix, gconstpointer data);
static void proxy_setup    (SeqnumFixture *fix, gconstpointer data);
static void proxy_teardown (SeqnumFixture *fix, gconstpointer data);
static void txn_setup      (SeqnumFixture *fix, gconstpointer data);
static void txn_teardown   (SeqnumFixture *fix, gconstpointer data);

static void test_getset_last   (SeqnumFixture *fix, gconstpointer data);
static void test_auto_inc      (SeqnumFixture *fix, gconstpointer data);

void
test_model_seqnums_create_suite (void)
{
#define SEQ_DOMAIN "/Model/Sequence/Seqnums"
#define PROXY_DOMAIN "/Model/Proxy/Seqnums"
#define TXN_DOMAIN "/Model/Transaction/Seqnums"

  g_test_add (SEQ_DOMAIN"/GetSet", SeqnumFixture, 0,
              setup, test_getset_last, teardown);
  g_test_add (PROXY_DOMAIN"/GetSet", SeqnumFixture, 0,
              proxy_setup, test_getset_last, proxy_teardown);
  g_test_add (TXN_DOMAIN"/GetSet", SeqnumFixture, 0,
              txn_setup, test_getset_last, txn_teardown);

  g_test_add (SEQ_DOMAIN"/AutoInc", SeqnumFixture, 0,
              setup, test_auto_inc, teardown);
  g_test_add (PROXY_DOMAIN"/AutoInc", SeqnumFixture, 0,
              proxy_setup, test_auto_inc, proxy_teardown);
  g_test_add (TXN_DOMAIN"/AutoInc", SeqnumFixture, 0,
              txn_setup, test_auto_inc, txn_teardown);
}

static void
setup (SeqnumFixture *fix, gconstpointer data)
{
  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "i", NULL);

  g_assert (DEE_IS_SEQUENCE_MODEL (fix->model));
  g_assert_cmpint (1, ==, dee_model_get_n_columns (fix->model));
  g_assert_cmpstr ("i", ==, dee_model_get_column_schema (fix->model, 0));
  g_assert_cmpint (0, ==, dee_model_get_n_rows (fix->model));
}

static void
teardown (SeqnumFixture *fix, gconstpointer data)
{
  g_object_unref (fix->model);
}

static void
proxy_setup (SeqnumFixture *fix, gconstpointer data)
{
  setup (fix, data);
  fix->model = g_object_new (DEE_TYPE_PROXY_MODEL,
                             "back-end", fix->model,
                             NULL);
  
  g_assert (DEE_IS_PROXY_MODEL (fix->model));
}

static void
proxy_teardown (SeqnumFixture *fix, gconstpointer data)
{
  g_assert (DEE_IS_PROXY_MODEL (fix->model));
  g_object_unref (fix->model);
}

static void
txn_setup (SeqnumFixture *fix, gconstpointer data)
{
  DeeModel *dum;

  setup (fix, data);
  dum = fix->model;
  fix->model = dee_transaction_new (dum);
  g_object_unref (dum);

  g_assert (DEE_IS_TRANSACTION (fix->model));
}

static void
txn_teardown (SeqnumFixture *fix, gconstpointer data)
{
  g_assert (DEE_IS_TRANSACTION (fix->model));
  g_object_unref (fix->model);
}

static void
test_getset_last (SeqnumFixture *fix, gconstpointer data)
{
  DeeModel     *model = fix->model;
  
  g_assert_cmpint (0, ==, dee_serializable_model_get_seqnum (model));
  
  dee_serializable_model_set_seqnum (model, 68);
  g_assert_cmpint (68, ==, dee_serializable_model_get_seqnum (model));
}

static void
test_auto_inc (SeqnumFixture *fix, gconstpointer data)
{
  DeeModel     *model = fix->model;

  g_assert_cmpint (0, ==, dee_serializable_model_get_seqnum (model));

  dee_model_append (fix->model, 9);
  g_assert_cmpint (1, ==, dee_serializable_model_get_seqnum (model));

  dee_model_set (fix->model, dee_model_get_first_iter (fix->model), 10);
  g_assert_cmpint (2, ==, dee_serializable_model_get_seqnum (model));

  dee_model_clear (fix->model);
  g_assert_cmpint (3, ==, dee_serializable_model_get_seqnum (model));
  
  dee_model_append (fix->model, 11);
  dee_model_append (fix->model, 12);
  dee_model_append (fix->model, 13);
  g_assert_cmpint (6, ==, dee_serializable_model_get_seqnum (model));
  
  dee_model_clear (fix->model);
  g_assert_cmpint (9, ==, dee_serializable_model_get_seqnum (model));
}

