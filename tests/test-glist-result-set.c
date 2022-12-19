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

#include <glib.h>
#include <glib-object.h>
#include <dee.h>

DeeResultSet* dee_glist_result_set_new (GList *rows,
                                         DeeModel *model,
                                         GObject *row_owner);

typedef struct
{
  DeeResultSet *results;
  DeeModel     *model;
  GList        *list;
} Fixture;

static void setup    (Fixture *fix, gconstpointer data);
static void teardown (Fixture *fix, gconstpointer data);

static void
setup (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter;

  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "s", "i", NULL);

  fix->list = NULL;

  iter = dee_model_append (fix->model, "Hello world", 27);
  fix->list = g_list_append (fix->list, iter);

  iter = dee_model_append (fix->model, "how are you", 28);
  fix->list = g_list_append (fix->list, iter);

  iter = dee_model_append (fix->model, "today", 28);
  fix->list = g_list_append (fix->list, iter);

  fix->results = dee_glist_result_set_new (fix->list, fix->model, NULL);
}

static void
teardown (Fixture *fix, gconstpointer data)
{
  g_object_unref (fix->results);
  g_object_unref (fix->model);
  g_list_free (fix->list);
  fix->results = NULL;
  fix->model = NULL;
  fix->list = NULL;
}

static void
test_three (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter;

  g_assert (DEE_IS_RESULT_SET (fix->results));

  g_assert_cmpint (dee_result_set_get_n_rows (fix->results), ==, 3);
  g_assert_cmpint (dee_result_set_tell(fix->results), ==, 0);
  g_assert (dee_result_set_has_next (fix->results));

  iter = dee_result_set_next (fix->results);
  g_assert (iter != NULL);
  g_assert_cmpstr (dee_model_get_string (fix->model, iter, 0), ==, "Hello world");
  g_assert_cmpint (dee_result_set_get_n_rows (fix->results), ==, 3);
  g_assert_cmpint (dee_result_set_tell(fix->results), ==, 1);
  g_assert (dee_result_set_has_next (fix->results));

  iter = dee_result_set_next (fix->results);
  g_assert (iter != NULL);
  g_assert_cmpstr (dee_model_get_string (fix->model, iter, 0), ==, "how are you");
  g_assert_cmpint (dee_result_set_get_n_rows (fix->results), ==, 3);
  g_assert_cmpint (dee_result_set_tell(fix->results), ==, 2);
  g_assert (dee_result_set_has_next (fix->results));

  iter = dee_result_set_next (fix->results);
  g_assert (iter != NULL);
  g_assert_cmpstr (dee_model_get_string (fix->model, iter, 0), ==, "today");
  g_assert_cmpint (dee_result_set_get_n_rows (fix->results), ==, 3);
  g_assert_cmpint (dee_result_set_tell(fix->results), ==, 3);
  g_assert (!dee_result_set_has_next (fix->results));

  g_assert (dee_result_set_get_model (fix->results) == fix->model);
}

static void
test_empty (Fixture *fix, gconstpointer data)
{
  DeeResultSet *results;

  results = dee_glist_result_set_new (NULL, fix->model, NULL);
  g_assert (DEE_IS_RESULT_SET (results));

  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 0);
  g_assert_cmpint (dee_result_set_tell(results), ==, 0);
  g_assert (!dee_result_set_has_next (results));

  g_object_unref (results);
}

void
test_glist_result_set_create_suite (void)
{
#define DOMAIN "/Index/ResultSet/GList"

  g_test_add (DOMAIN"/Test3", Fixture, 0,
              setup, test_three, teardown);
  g_test_add (DOMAIN"/Test0", Fixture, 0,
                setup, test_empty, teardown);
}
