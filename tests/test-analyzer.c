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
 * Authored by
 *              Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */

#include <glib.h>
#include <glib-object.h>
#include <dee.h>

typedef struct
{
  DeeAnalyzer    *analyzer;
  DeeTermList    *terms;
} Fixture;

static void setup         (Fixture *fix, gconstpointer data);
static void text_setup    (Fixture *fix, gconstpointer data);
static void teardown      (Fixture *fix, gconstpointer data);

static void
setup (Fixture *fix, gconstpointer data)
{
  fix->analyzer = dee_analyzer_new ();
  fix->terms = g_object_new (DEE_TYPE_TERM_LIST, NULL);
}

static void
text_setup (Fixture *fix, gconstpointer data)
{
  fix->analyzer = DEE_ANALYZER (dee_text_analyzer_new ());
  fix->terms = g_object_new (DEE_TYPE_TERM_LIST, NULL);
}

static void
teardown (Fixture *fix, gconstpointer data)
{
  g_object_unref (fix->analyzer);
  g_object_unref (fix->terms);
  fix->analyzer = NULL;
  fix->terms = NULL;
}

static void
test_simple (Fixture *fix, gconstpointer data)
{
  dee_analyzer_tokenize (fix->analyzer, "tok", fix->terms);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 1);
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 0), ==, "tok");
  dee_term_list_clear (fix->terms);

  dee_analyzer_tokenize (fix->analyzer, "tok kot", fix->terms);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 1);
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 0), ==, "tok kot");
  dee_term_list_clear (fix->terms);

  dee_analyzer_analyze (fix->analyzer, "foobar", fix->terms, NULL);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 1);
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 0), ==, "foobar");
}

void
_casefold (DeeTermList *in, DeeTermList *out, gpointer data)
{
  int i;
  gchar *fold;

  for (i = 0; i < dee_term_list_num_terms (in); i++)
    {
      fold = g_utf8_casefold (dee_term_list_get_term (in, i), -1);
      dee_term_list_add_term (out, fold);
      g_free (fold);
    }
}

static void
test_term_filter1 (Fixture *fix, gconstpointer data)
{
  dee_analyzer_analyze (fix->analyzer, "foobar", fix->terms, NULL);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 1);
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 0), ==, "foobar");
  dee_term_list_clear (fix->terms);

  dee_analyzer_add_term_filter(fix->analyzer, _casefold, NULL, NULL);

  dee_analyzer_analyze (fix->analyzer, "FooBar", fix->terms, NULL);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 1);
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 0), ==, "foobar");
  dee_term_list_clear (fix->terms);
}

void
test_text_analyzer_simple (Fixture *fix, gconstpointer data)
{
  dee_analyzer_analyze (fix->analyzer, "foobar", fix->terms, NULL);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 1);
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 0), ==, "foobar");
  dee_term_list_clear (fix->terms);

  dee_analyzer_analyze (fix->analyzer, "FooBar ", fix->terms, NULL);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 1);
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 0), ==, "foobar");
  dee_term_list_clear (fix->terms);

  dee_analyzer_analyze (fix->analyzer, "foo baR", fix->terms, NULL);
  g_assert_cmpint (dee_term_list_num_terms (fix->terms), ==, 2);
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 0), ==, "foo");
  g_assert_cmpstr (dee_term_list_get_term (fix->terms, 1), ==, "bar");
  dee_term_list_clear (fix->terms);
}

void
test_analyzer_create_suite (void)
{
  g_test_add ("/Index/Analyzer/Simple", Fixture, 0,
              setup, test_simple, teardown);

  g_test_add ("/Index/Analyzer/TermFilter1", Fixture, 0,
              setup, test_term_filter1, teardown);

  g_test_add ("/Index/TextAnalyzer/Simple", Fixture, 0,
              text_setup, test_text_analyzer_simple, teardown);
}
