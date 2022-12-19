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

} ColumnFixture;

static void column_setup          (ColumnFixture *fix, gconstpointer data);
static void column_teardown       (ColumnFixture *fix, gconstpointer data);
static void proxy_column_setup    (ColumnFixture *fix, gconstpointer data);
static void proxy_column_teardown (ColumnFixture *fix, gconstpointer data);

static void test_column_allocation   (ColumnFixture *fix, gconstpointer data);
static void test_set_get             (ColumnFixture *fix, gconstpointer data);

void
test_model_complex_column_create_suite (void)
{
#define SEQ_DOMAIN "/Model/Sequence/ComplexColumn"
#define PROXY_DOMAIN "/Model/Proxy/ComplexColumn"

  g_test_add (SEQ_DOMAIN"/Allocation", ColumnFixture, 0,
              column_setup, test_column_allocation, column_teardown);
  g_test_add (PROXY_DOMAIN"/Allocation", ColumnFixture, 0,
              proxy_column_setup, test_column_allocation, proxy_column_teardown);
  
  g_test_add (SEQ_DOMAIN"/SetGet", ColumnFixture, 0,
              column_setup, test_set_get, column_teardown);
  g_test_add (PROXY_DOMAIN"/SetGet", ColumnFixture, 0,
              proxy_column_setup, test_set_get, proxy_column_teardown);
}

static void
column_setup (ColumnFixture *fix, gconstpointer data)
{
  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "as", "(ii)", NULL);

  g_assert (DEE_IS_SEQUENCE_MODEL (fix->model));
  g_assert_cmpint (2, ==, dee_model_get_n_columns (fix->model));
  g_assert_cmpstr ("as", ==, dee_model_get_column_schema (fix->model, 0));
  g_assert_cmpstr ("(ii)", ==, dee_model_get_column_schema(fix->model, 1));
  g_assert_cmpint (0, ==, dee_model_get_n_rows (fix->model));
}

static void
column_teardown (ColumnFixture *fix, gconstpointer data)
{
  g_object_unref (fix->model);
}

static void
proxy_column_setup (ColumnFixture *fix, gconstpointer data)
{
  column_setup (fix, data);
  fix->model = g_object_new (DEE_TYPE_PROXY_MODEL,
                             "back-end", fix->model,
                             NULL);
  
  g_assert (DEE_IS_PROXY_MODEL (fix->model));
}

static void
proxy_column_teardown (ColumnFixture *fix, gconstpointer data)
{
  g_assert (DEE_IS_PROXY_MODEL (fix->model));
  g_object_unref (fix->model);
}

static void
test_column_allocation (ColumnFixture *fix, gconstpointer data)
{
  g_assert (DEE_IS_MODEL (fix->model));
}

static void
test_set_get (ColumnFixture *fix, gconstpointer data)
{
  DeeModel        *model = fix->model;
  DeeModelIter    *iter;
  GVariantBuilder  as;
  GVariant        *value, *tmp, *col1, *col2;
  gint32           i1, i2;

  g_variant_builder_init (&as, G_VARIANT_TYPE ("as"));
  g_variant_builder_add (&as, "s", "Hello");
  g_variant_builder_add (&as, "s", "World");

  dee_model_append (fix->model,
                    g_variant_builder_end (&as),
                    g_variant_new ("(ii)", 27, 68));

  g_assert (dee_model_get_n_rows (model) == 1);

  iter = dee_model_get_first_iter (model);

  dee_model_get (model, iter, &col1, &col2);

  value = dee_model_get_value (model, iter, 0);
  g_assert (value == col1);
  g_assert_cmpstr ("as", ==, g_variant_get_type_string (value));
  g_assert_cmpint (2, ==, g_variant_n_children(value));
  tmp = g_variant_get_child_value (value, 0);
  g_assert_cmpstr ("Hello", ==, g_variant_get_string (tmp, NULL));
  g_variant_unref (tmp);
  tmp = g_variant_get_child_value (value, 1);
  g_assert_cmpstr ("World", ==, g_variant_get_string (tmp, NULL));
  g_variant_unref (tmp);
  g_variant_unref (value);

  value = dee_model_get_value (model, iter, 1);
  g_assert (value == col2);
  g_variant_get (value, "(ii)", &i1, &i2);
  g_assert_cmpint (27, ==, i1);
  g_assert_cmpint (68, ==, i2);
  g_variant_unref (value);

  g_variant_unref (col1);
  g_variant_unref (col2);

}
