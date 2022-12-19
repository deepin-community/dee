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

typedef struct
{
  DeeModel    *model;
} Fixture;

static void setup    (Fixture *fix, gconstpointer data);
static void teardown (Fixture *fix, gconstpointer data);

static void
setup (Fixture *fix, gconstpointer data)
{
  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "s", "i", "s", "u", NULL);

  dee_model_append (fix->model,
                    "Hello world",
                    27,
                    "Three Danish characters... æøå?",
                    68);
}

static void
teardown (Fixture *fix, gconstpointer data)
{
  g_object_unref (fix->model);
  fix->model = NULL;
}

static void
test_string0 (Fixture *fix, gconstpointer data)
{
  DeeModelReader  reader;
  gchar          *val;

  dee_model_reader_new_for_string_column (0, &reader);
  val = dee_model_reader_read (&reader, fix->model,
                               dee_model_get_first_iter (fix->model));

  g_assert_cmpstr ("Hello world", ==, val);
  g_free (val);
}

static void
test_string2 (Fixture *fix, gconstpointer data)
{
  DeeModelReader  reader;
  gchar          *val;

  dee_model_reader_new_for_string_column (2, &reader);
  val = dee_model_reader_read (&reader, fix->model,
                                 dee_model_get_first_iter (fix->model));

  g_assert_cmpstr ("Three Danish characters... æøå?", ==, val);
  g_free (val);
}

static void
test_int (Fixture *fix, gconstpointer data)
{
  DeeModelReader  reader;
  gchar          *val;

  dee_model_reader_new_for_int32_column (1, &reader);
  val = dee_model_reader_read (&reader, fix->model,
                               dee_model_get_first_iter (fix->model));

  g_assert_cmpstr ("27", ==, val);
  g_free (val);
}

static void
test_uint (Fixture *fix, gconstpointer data)
{
  DeeModelReader  reader;
  gchar          *val;

  dee_model_reader_new_for_uint32_column (3, &reader);
  val = dee_model_reader_read (&reader, fix->model,
                               dee_model_get_first_iter (fix->model));

    g_assert_cmpstr ("68", ==, val);
    g_free (val);
}

void
test_model_readers_create_suite (void)
{
#define DOMAIN "/Index/ModelReaders"

  g_test_add (DOMAIN"/String0", Fixture, 0,
              setup, test_string0, teardown);
  g_test_add (DOMAIN"/String2", Fixture, 0,
                setup, test_string2, teardown);
  g_test_add (DOMAIN"/Int", Fixture, 0,
                    setup, test_int, teardown);
  g_test_add (DOMAIN"/UInt", Fixture, 0,
                    setup, test_uint, teardown);
}
