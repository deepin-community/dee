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
  DeeTermList *terms;

} Fixture;

static void setup    (Fixture *fix, gconstpointer data);
static void teardown (Fixture *fix, gconstpointer data);

static void
setup (Fixture *fix, gconstpointer data)
{
  fix->terms = g_object_new (DEE_TYPE_TERM_LIST, NULL);

  g_assert (DEE_IS_TERM_LIST (fix->terms));
}

static void
teardown (Fixture *fix, gconstpointer data)
{
  g_object_unref (fix->terms);
  fix->terms = NULL;
}

static void
test_empty (Fixture *fix, gconstpointer data)
{
  g_assert (DEE_IS_TERM_LIST (fix->terms));
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 0);

  dee_term_list_clear (fix->terms);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 0);
}

static void
test_one (Fixture *fix, gconstpointer data)
{
  dee_term_list_add_term (fix->terms, "one");
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 1);
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 0), ==, "one");

  dee_term_list_clear (fix->terms);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 0);
}

static void
test_two (Fixture *fix, gconstpointer data)
{
  dee_term_list_add_term (fix->terms, "one");
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 1);
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 0), ==, "one");

  dee_term_list_add_term (fix->terms, "two");
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 2);
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 0), ==, "one");
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 1), ==, "two");

  dee_term_list_clear (fix->terms);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 0);
}

static void
test_clone (Fixture *fix, gconstpointer data)
{
  DeeTermList *clone;

  /* Cloning an empty list should work */
  clone = dee_term_list_clone (fix->terms);
  g_assert_cmpint (dee_term_list_num_terms (clone), ==, 0);
  g_object_unref (clone);

  /* Clone with 1 item */
  dee_term_list_add_term (fix->terms, "one");
  clone = dee_term_list_clone (fix->terms);
  g_assert_cmpint (dee_term_list_num_terms (clone), ==, 1);
  g_assert_cmpstr (dee_term_list_get_term(clone, 0), ==, "one");

  /* Terms from cloned lists should compare by pointers directly */
  g_assert (dee_term_list_get_term (fix->terms, 0) ==
                                          dee_term_list_get_term (clone, 0));

  /* Clearing clone should not affect original */
  dee_term_list_clear (clone);
  g_assert_cmpint (dee_term_list_num_terms (clone), ==, 0);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 1);

  g_object_unref (clone);

  /* Clone with 3 items */
  dee_term_list_add_term (fix->terms, "two");
  dee_term_list_add_term (fix->terms, "three");
  clone = dee_term_list_clone (fix->terms);
  g_assert_cmpint (dee_term_list_num_terms (clone), ==, 3);
  g_assert_cmpstr (dee_term_list_get_term(clone, 0), ==, "one");
  g_assert_cmpstr (dee_term_list_get_term(clone, 1), ==, "two");
  g_assert_cmpstr (dee_term_list_get_term(clone, 2), ==, "three");

  /* Terms from cloned lists should compare by pointers directly */
  g_assert (dee_term_list_get_term (fix->terms, 0) ==
                                          dee_term_list_get_term (clone, 0));
  g_assert (dee_term_list_get_term (fix->terms, 1) ==
                                          dee_term_list_get_term (clone, 1));
  g_assert (dee_term_list_get_term (fix->terms, 2) ==
                                          dee_term_list_get_term (clone, 2));

  /* Clearing original should not affect clone*/
  dee_term_list_clear (fix->terms);
  g_assert_cmpint (dee_term_list_num_terms (clone), ==, 3);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 0);

  g_object_unref (clone);
}

void
test_term_list_create_suite (void)
{
#define DOMAIN "/Index/TermList"

  g_test_add (DOMAIN"/Empty", Fixture, 0,
              setup, test_empty, teardown);
  g_test_add (DOMAIN"/One", Fixture, 0,
              setup, test_one, teardown);
  g_test_add (DOMAIN"/Two", Fixture, 0,
              setup, test_two, teardown);
  g_test_add (DOMAIN"/Clone", Fixture, 0,
                setup, test_clone, teardown);
}
