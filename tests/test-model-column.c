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
#include <stdlib.h>

typedef struct 
{
  DeeModel *model;

} ColumnFixture;

static void column_setup          (ColumnFixture *fix, gconstpointer data);
static void column_teardown       (ColumnFixture *fix, gconstpointer data);
static void proxy_column_setup    (ColumnFixture *fix, gconstpointer data);
static void proxy_column_teardown (ColumnFixture *fix, gconstpointer data);

static void test_column_allocation   (ColumnFixture *fix, gconstpointer data);
static void test_unmodified_and_get_value        (ColumnFixture *fix, gconstpointer data);
static void test_modification_and_get_row (ColumnFixture *fix, gconstpointer data);
static void test_get_schema          (ColumnFixture *fix, gconstpointer data);
static void test_no_schema           (ColumnFixture *fix, gconstpointer data);
static void test_bad_schemas         (void);
static void test_null_string         (ColumnFixture *fix, gconstpointer data);

void
test_model_column_create_suite (void)
{
#define SEQ_DOMAIN "/Model/Sequence/Column"
#define PROXY_DOMAIN "/Model/Proxy/Column"

  g_test_add (SEQ_DOMAIN"/Allocation", ColumnFixture, 0,
              column_setup, test_column_allocation, column_teardown);
  g_test_add (PROXY_DOMAIN"/Allocation", ColumnFixture, 0,
              proxy_column_setup, test_column_allocation, proxy_column_teardown);
  
  g_test_add (SEQ_DOMAIN"/UnmodifiedAndGetValue", ColumnFixture, 0,
              column_setup, test_unmodified_and_get_value, column_teardown);
  g_test_add (PROXY_DOMAIN"/UnmodifiedAndGetValue", ColumnFixture, 0,
              proxy_column_setup, test_unmodified_and_get_value, proxy_column_teardown);
  
  g_test_add (SEQ_DOMAIN"/ModificationAndGetRow", ColumnFixture, 0,
              column_setup, test_modification_and_get_row, column_teardown);
  g_test_add (PROXY_DOMAIN"/ModificationAndGetRow", ColumnFixture, 0,
              proxy_column_setup, test_modification_and_get_row, proxy_column_teardown);
  
  g_test_add (SEQ_DOMAIN"/Schemas", ColumnFixture, 0,
              column_setup, test_get_schema, column_teardown);
  g_test_add (PROXY_DOMAIN"/Schemas", ColumnFixture, 0,
              proxy_column_setup, test_get_schema, proxy_column_teardown);
  
  g_test_add (SEQ_DOMAIN"/NoSchemas", ColumnFixture, 0,
              column_setup, test_no_schema, column_teardown);
  g_test_add (PROXY_DOMAIN"/NoSchemas", ColumnFixture, 0,
              proxy_column_setup, test_no_schema, proxy_column_teardown);

  g_test_add_func ("/Model/Column/BadSchemas", test_bad_schemas);

  g_test_add (SEQ_DOMAIN"/NullString", ColumnFixture, 0,
              column_setup, test_null_string, column_teardown);
  g_test_add (PROXY_DOMAIN"/NullString", ColumnFixture, 0,
              proxy_column_setup, test_null_string, proxy_column_teardown);
}

static void
column_setup (ColumnFixture *fix, gconstpointer data)
{
  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model,
                        "b", "y", "i", "u", "x", "t", "d", "s", NULL);

  g_assert (DEE_IS_SEQUENCE_MODEL (fix->model));
  g_assert_cmpint (8, ==, dee_model_get_n_columns (fix->model));
  g_assert_cmpstr ("b", ==, dee_model_get_column_schema (fix->model, 0));
  g_assert_cmpstr ("y", ==, dee_model_get_column_schema(fix->model, 1));
  g_assert_cmpstr ("i", ==, dee_model_get_column_schema (fix->model, 2));
  g_assert_cmpstr ("u", ==, dee_model_get_column_schema (fix->model, 3));
  g_assert_cmpstr ("x", ==, dee_model_get_column_schema (fix->model, 4));
  g_assert_cmpstr ("t", ==, dee_model_get_column_schema (fix->model, 5));
  g_assert_cmpstr ("d", ==, dee_model_get_column_schema (fix->model, 6));
  g_assert_cmpstr ("s", ==, dee_model_get_column_schema (fix->model, 7));
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
test_unmodified_and_get_value (ColumnFixture *fix, gconstpointer data)
{
  DeeModel     *model = fix->model;
  DeeModelIter *iter;
  GVariant     *value;

  dee_model_append (fix->model,
                    TRUE,
                    '1',
                    G_MININT32,
                    G_MAXUINT32,
                    G_MAXINT64,
                    G_MAXUINT64,
                    G_MAXDOUBLE,
                    "Hello World");

  g_assert (dee_model_get_n_rows (model) == 1);

  iter = dee_model_get_first_iter (model);

  value = dee_model_get_value (model, iter, 0);
  g_assert_cmpint (TRUE, ==, g_variant_get_boolean (value));
  g_assert_cmpint (TRUE, ==, dee_model_get_bool (model, iter, 0));
  g_variant_unref (value);

  value = dee_model_get_value (model, iter, 1);
  g_assert_cmpint ('1', ==, g_variant_get_byte (value));
  g_assert_cmpint ('1', ==, dee_model_get_uchar (model, iter, 1));
  g_variant_unref (value);

  value = dee_model_get_value (model, iter, 2);
  g_assert_cmpint (G_MININT32, ==, g_variant_get_int32 (value));
  g_assert_cmpint (G_MININT32, ==, dee_model_get_int32 (model, iter, 2));
  g_variant_unref (value);

  value = dee_model_get_value (model, iter, 3);
  g_assert (g_variant_get_uint32 (value) == G_MAXUINT32);
  g_assert_cmpuint (G_MAXUINT32, ==, dee_model_get_uint32 (model, iter, 3));
  g_variant_unref (value);

  value = dee_model_get_value (model, iter, 4);
  g_assert (g_variant_get_int64 (value) == G_MAXINT64);
  g_assert_cmpint (G_MAXINT64, ==, dee_model_get_int64 (model, iter, 4));
  g_variant_unref (value);

  value = dee_model_get_value (model, iter, 5);
  g_assert (g_variant_get_uint64 (value) == G_MAXUINT64);
  g_assert_cmpuint (G_MAXUINT64, ==, dee_model_get_uint64 (model, iter, 5));
  g_variant_unref (value);

  value = dee_model_get_value (model, iter, 6);
  g_assert (g_variant_get_double (value) == G_MAXDOUBLE);
  g_assert_cmpfloat (G_MAXDOUBLE, ==, dee_model_get_double (model, iter, 6));
  g_variant_unref (value);

  value = dee_model_get_value (model, iter, 7);
  g_assert_cmpstr (g_variant_get_string (value, NULL), ==, "Hello World");
  g_assert_cmpstr ("Hello World", ==, dee_model_get_string (model, iter, 7));
  g_variant_unref (value);

  /* Assert that we don't mess up the string copying.
   * Ie that dee_model_get_string() returns a const string */
  const gchar *cp1 = dee_model_get_string (model, iter, 7);
  const gchar *cp2 = dee_model_get_string (model, iter, 7);
  g_assert_cmpstr ("Hello World", ==, cp1);
  g_assert_cmpstr ("Hello World", ==, cp2);
  g_assert (cp1 == cp2);
}

static void
test_modification_and_get_row (ColumnFixture *fix, gconstpointer data)
{
  DeeModel      *model = fix->model;
  DeeModelIter  *iter;
  GVariant     **heap, **stack, **_stack;
  gint           i;

  dee_model_append (fix->model,
                    TRUE,
                    '1',
                    G_MININT,
                    G_MAXUINT,
                    G_MAXINT64,
                    G_MAXUINT64,
                    G_MAXDOUBLE,
                    "Hello World");
  
  g_assert (dee_model_get_n_rows (model) == 1);

  iter = dee_model_get_first_iter (model);

  dee_model_set (model, iter, 
                 FALSE,
                 '2',
                 G_MININT+5,
                 G_MAXUINT-5,
                 G_MAXINT64-5,
                 G_MAXUINT64-5,
                 G_MAXDOUBLE-5.0,
                 "World Hello");

  /* Try filling a stack allocated array and assert
   * that we get the same pointer back*/
  stack = g_alloca (sizeof(gpointer) * dee_model_get_n_columns (model));
  _stack = dee_model_get_row (model, iter, stack);
  g_assert (stack == _stack);

  /* Also try allocating a new array on the heap by passing in NULL for
   * the destination */
  heap = dee_model_get_row (model, iter, NULL);
  g_assert (heap != NULL);

  /* Now assert that both the stack- and heap allocated row
   * contains the right data */
  g_assert (!g_variant_get_boolean (stack[0]));
  g_assert (!g_variant_get_boolean (heap[0]));

  g_assert_cmpint (g_variant_get_byte (stack[1]), ==, '2');
  g_assert_cmpint (g_variant_get_byte (heap[1]), ==, '2');

  g_assert_cmpint (g_variant_get_int32 (stack[2]), ==, G_MININT+5);
  g_assert_cmpint (g_variant_get_int32 (heap[2]), ==, G_MININT+5);

  g_assert_cmpint (g_variant_get_uint32 (stack[3]), ==, G_MAXUINT-5);
  g_assert_cmpint (g_variant_get_uint32 (heap[3]), ==, G_MAXUINT-5);

  g_assert (g_variant_get_int64 (stack[4]) == G_MAXINT64-5);
  g_assert (g_variant_get_int64 (heap[4]) == G_MAXINT64-5);

  g_assert (g_variant_get_uint64 (stack[5]) == G_MAXUINT64-5);
  g_assert (g_variant_get_uint64 (heap[5]) == G_MAXUINT64-5);

  g_assert (g_variant_get_double (stack[6]) == G_MAXDOUBLE-5.0);
  g_assert (g_variant_get_double (heap[6]) == G_MAXDOUBLE-5.0);

  g_assert_cmpstr (g_variant_get_string (stack[7], NULL), ==, "World Hello");
  g_assert_cmpstr (g_variant_get_string (heap[7], NULL), ==, "World Hello");

  /* Unref the variants and free the heap allocated array */
  for (i = 0; i < dee_model_get_n_columns (model); i++)
    {
      g_variant_unref (stack[i]);
      g_variant_unref (heap[i]);
    }
  g_free (heap);
}

static void
test_get_schema (ColumnFixture *fix, gconstpointer data)
{
  const gchar* const *schema;
  guint               n_cols;

  /* First check we don't crash when passing NULL for num_columns */
  schema = dee_model_get_schema (fix->model,
                                 NULL);

  schema = dee_model_get_schema (fix->model,
                                 &n_cols);
  
  g_assert_cmpint (8, ==, n_cols);

  g_assert_cmpstr ("b", ==, schema[0]);
  g_assert_cmpstr ("b", ==, dee_model_get_column_schema (fix->model, 0));
  g_assert_cmpstr ("y", ==, schema[1]);
  g_assert_cmpstr ("y", ==, dee_model_get_column_schema (fix->model, 1));
  g_assert_cmpstr ("i", ==, schema[2]);
  g_assert_cmpstr ("i", ==, dee_model_get_column_schema (fix->model, 2));
  g_assert_cmpstr ("u", ==, schema[3]);
  g_assert_cmpstr ("u", ==, dee_model_get_column_schema (fix->model, 3));
  g_assert_cmpstr ("x", ==, schema[4]);
  g_assert_cmpstr ("x", ==, dee_model_get_column_schema (fix->model, 4));
  g_assert_cmpstr ("t", ==, schema[5]);
  g_assert_cmpstr ("t", ==, dee_model_get_column_schema (fix->model, 5));
  g_assert_cmpstr ("d", ==, schema[6]);
  g_assert_cmpstr ("d", ==, dee_model_get_column_schema (fix->model, 6));
  g_assert_cmpstr ("s", ==, schema[7]);
  g_assert_cmpstr ("s", ==, dee_model_get_column_schema (fix->model, 7));

}

static void
test_no_schema (ColumnFixture *fix, gconstpointer data)
{
  DeeModel *model;

  /* Create a model without a schema and try to manipulate it */
  model = dee_sequence_model_new ();

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR))
    {
      dee_model_append (model, "ignore this...");
      exit (0); /* successful test run (which we shouldn't have) */
    }
  g_test_trap_assert_failed();
  g_test_trap_assert_stderr ("*doesn't have a schema*");
}

static void
test_bad_schemas (void)
{
  DeeModel *model = NULL;

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT
                        | G_TEST_TRAP_SILENCE_STDERR))
    {
      model = dee_sequence_model_new ();
      dee_model_set_schema (model, "", NULL); // empty signature
    }
  g_test_trap_assert_failed ();

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT
                        | G_TEST_TRAP_SILENCE_STDERR))
    {
      model = dee_sequence_model_new ();
      dee_model_set_schema (model, "as", "u", "(tt", NULL); // unclosed tuple
    }
  g_test_trap_assert_failed ();

  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT
                        | G_TEST_TRAP_SILENCE_STDERR))
    {
      model = dee_sequence_model_new ();
      dee_model_set_schema (model, "///", NULL); // illegal characters
    }
  g_test_trap_assert_failed ();
}

/* Inserting a NULL string should work */
static void
test_null_string (ColumnFixture *fix, gconstpointer data)
{
  DeeModel      *model = fix->model;
  DeeModelIter  *iter;

  /* Test that we can add a NULL string and that it'll be treated as "" */
  dee_model_append (fix->model,
                    TRUE,
                    '1',
                    (gint)0,
                    (guint)0,
                    G_GINT64_CONSTANT(0),
                    G_GUINT64_CONSTANT (0),
                    (gdouble)0.0,
                    NULL);

  iter = dee_model_get_first_iter (model);
  g_assert_cmpstr ("", ==, dee_model_get_string (model, iter, 7));

  dee_model_set_value (model, iter, 7, g_variant_new_string ("foo"));
  g_assert_cmpstr ("foo", ==, dee_model_get_string (model, iter, 7));

  /* Test that we can modify a row in place and set a NULL string which
   * is treated as a "" */
  dee_model_set (model, iter,
                 TRUE,
                 '1',
                 (gint)0,
                 (guint)0,
                 G_GINT64_CONSTANT(0),
                 G_GUINT64_CONSTANT (0),
                 (gdouble)0.0,
                 NULL);
  g_assert_cmpstr ("", ==, dee_model_get_string (model, iter, 7));
}
