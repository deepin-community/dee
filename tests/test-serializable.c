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
#include <glib/gstdio.h>
#include <glib-object.h>
#include <dee.h>

typedef struct
{
  DeeModel *orig;
  DeeModel *copy;
} Fixture;

static void sequence_model_setup    (Fixture *fix, gconstpointer data);
static void sequence_model_teardown (Fixture *fix, gconstpointer data);

static void shared_model_setup    (Fixture *fix, gconstpointer data);
static void shared_model_teardown (Fixture *fix, gconstpointer data);

static void test_model_zero_rows (Fixture *fix, gconstpointer data);
static void test_model_one_rows (Fixture *fix, gconstpointer data);
static void test_model_two_rows (Fixture *fix, gconstpointer data);
static void test_model_non_zero_seqnum_offset (Fixture *fix, gconstpointer data);

void
test_serializable_create_suite (void)
{
#define SEQUENCE_MODEL_DOMAIN "/Serializable/SequenceModel"
#define SHARED_MODEL_DOMAIN "/Serializable/SharedModel"

  g_test_add (SEQUENCE_MODEL_DOMAIN"/ZeroRows", Fixture, 0,
              sequence_model_setup, test_model_zero_rows, sequence_model_teardown);
  
  g_test_add (SHARED_MODEL_DOMAIN"/ZeroRows", Fixture, 0,
                shared_model_setup, test_model_zero_rows, shared_model_teardown);

  g_test_add (SEQUENCE_MODEL_DOMAIN"/OneRows", Fixture, 0,
                sequence_model_setup, test_model_one_rows, sequence_model_teardown);
  
  g_test_add (SHARED_MODEL_DOMAIN"/OneRows", Fixture, 0,
                  sequence_model_setup, test_model_one_rows, shared_model_teardown);

  g_test_add (SEQUENCE_MODEL_DOMAIN"/TwoRows", Fixture, 0,
                sequence_model_setup, test_model_two_rows, sequence_model_teardown);

  g_test_add (SHARED_MODEL_DOMAIN"/TwoRows", Fixture, 0,
                  shared_model_setup, test_model_two_rows, shared_model_teardown);

  g_test_add (SEQUENCE_MODEL_DOMAIN"/NonZeroSeqnumOffset", Fixture, 0,
              sequence_model_setup, test_model_non_zero_seqnum_offset, sequence_model_teardown);

    g_test_add (SHARED_MODEL_DOMAIN"/NonZeroSeqnumOffset", Fixture, 0,
                    shared_model_setup, test_model_non_zero_seqnum_offset, shared_model_teardown);
}

static void
sequence_model_setup (Fixture *fix, gconstpointer data)
{
  fix->orig = dee_sequence_model_new ();
  dee_model_set_schema (fix->orig, "i", "s", NULL);

  fix->copy = NULL;

  g_assert (DEE_IS_SEQUENCE_MODEL (fix->orig));
}

static void
sequence_model_teardown (Fixture *fix, gconstpointer data)
{
  g_object_unref (fix->orig);
  fix->orig = NULL;

  if (fix->copy)
    g_object_unref (fix->copy);
  fix->copy = NULL;
}

static void
shared_model_setup (Fixture *fix, gconstpointer data)
{
  fix->orig = dee_shared_model_new ("org.example.ThisIsNotATest");
  dee_model_set_schema (fix->orig, "i", "s", NULL);

  fix->copy = NULL;

  g_assert (DEE_IS_SHARED_MODEL (fix->orig));
}

static void
shared_model_teardown (Fixture *fix, gconstpointer data)
{
  g_object_unref (fix->orig);
  fix->orig = NULL;

  if (fix->copy)
    g_object_unref (fix->copy);
  fix->copy = NULL;
}

static void
dee_assert_cmpmodel (DeeModel *m1, DeeModel *m2)
{
  guint         i, j, n_cols, n_rows;
  DeeModelIter *row1, *row2;
  GVariant     *v1, *v2;

  g_assert (m1 != NULL);
  g_assert (m2 != NULL);

  g_assert (DEE_IS_MODEL (m1));
  g_assert (DEE_IS_MODEL (m2));

  g_assert_cmpint (dee_model_get_n_columns (m1), ==, dee_model_get_n_columns (m2));
  g_assert_cmpint (dee_model_get_n_rows (m1), ==, dee_model_get_n_rows (m2));

  n_cols = dee_model_get_n_columns (m1);
  for (i = 0; i < n_cols; i++)
    {
      g_assert_cmpstr (dee_model_get_column_schema (m1, i), ==, dee_model_get_column_schema (m1, i));
    }

  n_rows = dee_model_get_n_rows (m1);
  for (i = 0; i < n_rows; i++)
    {
      row1 = dee_model_get_iter_at_row (m1, i);
      row2 = dee_model_get_iter_at_row (m2, i);
      for (j = 0; j < n_cols; j++)
        {
          v1 = dee_model_get_value (m1, row1, j);
          v2 = dee_model_get_value (m2, row2, j);
          g_assert (g_variant_equal (v1, v2));
          g_variant_unref (v1);
          g_variant_unref (v2);
        }
    }

  if (DEE_IS_SERIALIZABLE_MODEL (m1) && DEE_IS_SERIALIZABLE_MODEL (m2))
      {
        g_assert_cmpuint(dee_serializable_model_get_seqnum (m1), ==, dee_serializable_model_get_seqnum (m2));
      }

  if (DEE_IS_SHARED_MODEL (m1) && DEE_IS_SHARED_MODEL (m2))
    {
      g_assert_cmpstr (dee_shared_model_get_swarm_name (DEE_SHARED_MODEL (m1)), ==, dee_shared_model_get_swarm_name (DEE_SHARED_MODEL (m2)));
    }
}

static void
test_model_zero_rows (Fixture *fix, gconstpointer data)
{
  GVariant *vdata;

  vdata = dee_serializable_externalize (DEE_SERIALIZABLE (fix->orig));
  g_assert (vdata != NULL);

  fix->copy = DEE_MODEL (dee_serializable_parse_external (vdata));

  dee_assert_cmpmodel (fix->orig, fix->copy);
}

static void
test_model_one_rows (Fixture *fix, gconstpointer data)
{
  GVariant *vdata;

  dee_model_append (fix->orig, 27, "Hello world");

  vdata = dee_serializable_externalize (DEE_SERIALIZABLE (fix->orig));
  g_assert (vdata != NULL);

  fix->copy = DEE_MODEL (dee_serializable_parse_external (vdata));

  dee_assert_cmpmodel (fix->orig, fix->copy);
}

static void
test_model_two_rows (Fixture *fix, gconstpointer data)
{
  GVariant *vdata;

  dee_model_append (fix->orig, 27, "Hello world");
  dee_model_append (fix->orig, 68, "Hola Mars");

  vdata = dee_serializable_externalize (DEE_SERIALIZABLE (fix->orig));
  g_assert (vdata != NULL);

  fix->copy = DEE_MODEL (dee_serializable_parse_external (vdata));

  dee_assert_cmpmodel (fix->orig, fix->copy);
}

static void
test_model_non_zero_seqnum_offset (Fixture *fix, gconstpointer data)
{
  GVariant *vdata;
  DeeModelIter *first_row;

  first_row = dee_model_append (fix->orig, 27, "Hello world");
  dee_model_append (fix->orig, 68, "Hola Mars");
  dee_model_remove (fix->orig, first_row);


  vdata = dee_serializable_externalize (DEE_SERIALIZABLE (fix->orig));
  g_assert (vdata != NULL);

  fix->copy = DEE_MODEL (dee_serializable_parse_external (vdata));

  g_assert_cmpuint (1, ==, dee_model_get_n_rows (fix->copy));
  g_assert_cmpuint (3, ==, dee_serializable_model_get_seqnum (fix->copy));

  dee_assert_cmpmodel (fix->orig, fix->copy);
}
