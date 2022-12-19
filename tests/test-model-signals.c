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
 *
 */

#include <glib.h>
#include <glib-object.h>
#include <dee.h>

typedef struct
{
  DeeModel *model;

} SignalsFixture;

static void rows_setup          (SignalsFixture *fix, gconstpointer data);
static void rows_teardown       (SignalsFixture *fix, gconstpointer data);
static void proxy_rows_setup    (SignalsFixture *fix, gconstpointer data);
static void proxy_rows_teardown (SignalsFixture *fix, gconstpointer data);

static void test_signal_add     (SignalsFixture *fix, gconstpointer data);
static void test_signal_remove  (SignalsFixture *fix, gconstpointer data);
static void test_signal_changed (SignalsFixture *fix, gconstpointer data);

void
test_model_signals_create_suite (void)
{
#define SEQ_DOMAIN "/Model/Sequence/Signals"
#define PROXY_DOMAIN "/Model/Proxy/Signals"

  g_test_add (SEQ_DOMAIN"/Add", SignalsFixture, 0,
              rows_setup, test_signal_add, rows_teardown);
  g_test_add (PROXY_DOMAIN"/Add", SignalsFixture, 0,
              proxy_rows_setup, test_signal_add, proxy_rows_teardown);
  
  g_test_add (SEQ_DOMAIN"/Remove", SignalsFixture, 0,
              rows_setup, test_signal_remove, rows_teardown);
  g_test_add (PROXY_DOMAIN"/Remove", SignalsFixture, 0,
              proxy_rows_setup, test_signal_remove, proxy_rows_teardown);
              
  g_test_add (SEQ_DOMAIN"/Changed", SignalsFixture, 0,
              rows_setup, test_signal_changed, rows_teardown);
  g_test_add (PROXY_DOMAIN"/Changed", SignalsFixture, 0,
              proxy_rows_setup, test_signal_changed, proxy_rows_teardown);
}

static void
rows_setup (SignalsFixture *fix, gconstpointer data)
{
  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "i", "s", NULL);

  g_assert (DEE_IS_SEQUENCE_MODEL (fix->model));
}

static void
rows_teardown (SignalsFixture *fix, gconstpointer data)
{
  g_object_unref (fix->model);
}

static void
proxy_rows_setup (SignalsFixture *fix, gconstpointer data)
{
  rows_setup (fix, data);
  fix->model = g_object_new (DEE_TYPE_PROXY_MODEL,
                             "back-end", fix->model,
                             NULL);

  g_assert (DEE_IS_PROXY_MODEL (fix->model));
}

static void
proxy_rows_teardown (SignalsFixture *fix, gconstpointer data)
{
  g_object_unref (fix->model);
}

static guint n_add_signals = 0;

static void
test_signal_add_callback (DeeModel *model, DeeModelIter *iter)
{
  n_add_signals++;
}

static void
test_signal_add (SignalsFixture *fix, gconstpointer data)
{
  gint i;

  g_signal_connect (fix->model, "row-added",
                    G_CALLBACK (test_signal_add_callback), NULL);

  n_add_signals = 0;
  for (i = 0; i < 10000; i++)
    {
      dee_model_append (fix->model, i, "Test");
    }
  g_assert_cmpint (n_add_signals, ==, 10000);
}

static guint n_remove_signals = 0;

static void
test_signal_remove_callback (DeeModel *model, DeeModelIter *iter)
{
  n_remove_signals++;
}

static void
test_signal_remove (SignalsFixture *fix, gconstpointer data)
{
  gint           i;

  g_signal_connect (fix->model, "row-removed",
                    G_CALLBACK (test_signal_remove_callback), NULL);

  for (i = 0; i < 10000; i++)
    {
      dee_model_append (fix->model, i, "Test");
    }

  n_remove_signals = i = 0;
  dee_model_clear (fix->model);
  g_assert_cmpint (n_remove_signals, ==, 10000);
}

static guint n_changed_signals = 0;

static void
test_signal_changed_callback (DeeModel *model, DeeModelIter *iter)
{
  n_changed_signals++;
}

static void
test_signal_changed (SignalsFixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  gint          i;

  g_signal_connect (fix->model, "row-changed",
                    G_CALLBACK (test_signal_changed_callback), NULL);

  for (i = 0; i < 10000; i++)
    {
      dee_model_append (fix->model, i, "Test");
    }

  n_changed_signals = 0;
  iter = dee_model_get_first_iter (fix->model);
  while (iter != NULL && !dee_model_is_last (fix->model, iter))
    {
      gint32   j = 0;
      gchar   *k = "ing";

      dee_model_set (fix->model, iter, j, k);
      iter = dee_model_next (fix->model, iter);
    }
  g_assert_cmpint (n_changed_signals, ==, 10000);

  n_changed_signals = 0;
  iter = dee_model_get_first_iter (fix->model);
  while (iter != NULL && !dee_model_is_last (fix->model, iter))
    {
      dee_model_set_value (fix->model, iter, 0, g_variant_new_int32 (10));
      iter = dee_model_next (fix->model, iter);
    }
  g_assert_cmpint (n_changed_signals, ==, 10000);
}
