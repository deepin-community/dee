/*
 * Copyright (C) 2010-2011 Canonical Ltd
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

#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <dee.h>

typedef struct
{
  DeeModel *model;
  DeeIndex *index;

} Fixture;

static void setup_hash    (Fixture *fix, gconstpointer data);
static void setup_tree    (Fixture *fix, gconstpointer data);
static void teardown (Fixture *fix, gconstpointer data);

static void
setup_hash (Fixture *fix, gconstpointer data)
{
  DeeAnalyzer *analyzer = dee_analyzer_new ();
  DeeModelReader reader;

  dee_model_reader_new_for_string_column (0, &reader);

  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "s", "i", NULL);

  fix->index = DEE_INDEX (dee_hash_index_new(fix->model, analyzer, &reader));

  g_object_unref (analyzer);
}

static void
setup_text_hash (Fixture *fix, gconstpointer data)
{
  DeeAnalyzer *analyzer = DEE_ANALYZER (dee_text_analyzer_new ());
  DeeModelReader reader;

  dee_model_reader_new_for_string_column (0, &reader);

  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "s", "i", NULL);

  fix->index = DEE_INDEX (dee_hash_index_new(fix->model, analyzer, &reader));

  g_object_unref (analyzer);
}

static void
setup_tree (Fixture *fix, gconstpointer data)
{
  DeeAnalyzer *analyzer = dee_analyzer_new ();
  DeeModelReader reader;

  dee_model_reader_new_for_string_column (0, &reader);

  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "s", "i", NULL);

  fix->index = DEE_INDEX (dee_tree_index_new(fix->model, analyzer, &reader));

  g_object_unref (analyzer);
}

static void
setup_text_tree (Fixture *fix, gconstpointer data)
{
  DeeAnalyzer *analyzer = DEE_ANALYZER (dee_text_analyzer_new ());
  DeeModelReader reader;

  dee_model_reader_new_for_string_column (0, &reader);

  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "s", "i", NULL);

  fix->index = DEE_INDEX (dee_tree_index_new(fix->model, analyzer, &reader));

  g_object_unref (analyzer);
}

static void
teardown (Fixture *fix, gconstpointer data)
{
  g_object_unref (fix->index);
  g_object_unref (fix->model);
  fix->index = NULL;
  fix->model = NULL;
}

static void
test_empty (Fixture *fix, gconstpointer data)
{
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 0);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 0);

  dee_model_clear (fix->model);

  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 0);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 0);

  g_assert (NULL == dee_index_lookup_one (fix->index, "foobar"));
}

static void
test_one (Fixture *fix, gconstpointer data)
{
  DeeModelIter *orig_iter, *iter;
  DeeResultSet *results;

  orig_iter = dee_model_append (fix->model, "Hello world", 27);

  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 1);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "Hello world"), ==, 1);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 1);

  results = dee_index_lookup (fix->index, "Hello", DEE_TERM_MATCH_EXACT);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 0);
  g_object_unref (results);

  results = dee_index_lookup (fix->index, "Hello world", DEE_TERM_MATCH_EXACT);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 1);

  iter = dee_result_set_next (results);
  g_assert (iter == orig_iter);
  g_assert (NULL == dee_index_lookup_one (fix->index, "foobar"));
  g_assert (orig_iter == dee_index_lookup_one (fix->index, "Hello world"));

  dee_model_clear (fix->model);
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "Hello world"), ==, 0);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 0);

  g_object_unref (results);
}

static void
test_two (Fixture *fix, gconstpointer data)
{
  DeeModelIter *orig_iter, *iter;
  DeeResultSet *results;

  orig_iter = dee_model_append (fix->model, "Hello world", 27);
  dee_model_append (fix->model, "Dee world", 68);

  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 2);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "Hello world"), ==, 1);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "Dee world"), ==, 1);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 2);

  results = dee_index_lookup (fix->index, "Hello world", DEE_TERM_MATCH_EXACT);
  g_assert_cmpint (dee_result_set_get_n_rows(results), ==, 1);

  iter = dee_result_set_next (results);
  g_assert (iter == orig_iter);
  g_object_unref (results);

  results = dee_index_lookup (fix->index, "Dee world", DEE_TERM_MATCH_EXACT);
  g_assert_cmpint (dee_result_set_get_n_rows(results), ==, 1);
  g_object_unref (results);

  g_assert (NULL == dee_index_lookup_one (fix->index, "foobar"));
  g_assert (NULL == dee_index_lookup_one (fix->index, "Hello"));
  g_assert (NULL == dee_index_lookup_one (fix->index, "Dee"));

  dee_model_clear (fix->model);
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "Hello world"), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "Dee world"), ==, 0);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 0);
}

/* Test that we can create an index on a model that already has some rows in it */
static void
test_index_on_existing_rows (Fixture *fix, gconstpointer data)
{
  DeeAnalyzer    *analyzer;
  DeeModelReader  reader;
  DeeIndex       *extra_idx;
  DeeResultSet   *results;
  
  dee_model_append (fix->model, "Hello world", 27);
  dee_model_append (fix->model, "Hello Dee", 68);
  
  analyzer = DEE_ANALYZER (dee_text_analyzer_new ());
  dee_model_reader_new_for_string_column (0, &reader);
  
  /* Test DeeHashIndex */
  extra_idx = DEE_INDEX (dee_hash_index_new(fix->model, analyzer, &reader));
  results = dee_index_lookup (extra_idx, "hello", DEE_TERM_MATCH_EXACT);
  g_assert_cmpint (dee_result_set_get_n_rows(results), ==, 2);
  g_object_unref (results);
  g_object_unref (extra_idx);
  
  /* Test DeeTreeIndex */
  extra_idx = DEE_INDEX (dee_tree_index_new(fix->model, analyzer, &reader));
  results = dee_index_lookup (extra_idx, "hello", DEE_TERM_MATCH_EXACT);
  g_assert_cmpint (dee_result_set_get_n_rows(results), ==, 2);
  g_object_unref (results);
  g_object_unref (extra_idx);
  
  g_object_unref (analyzer);
  
}

static void
test_change (Fixture *fix, gconstpointer data)
{
  DeeModelIter *orig_iter, *iter;
  DeeResultSet *results;

  dee_model_append (fix->model, "Hello world", 27);
  dee_model_append (fix->model, "Hello world", 27);
  dee_model_append (fix->model, "Hello world", 27);
  orig_iter = dee_model_append (fix->model, "xyz", 27);
  dee_model_append (fix->model, "Hello world", 27);

  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 5);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "Hello world"), ==, 4);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "xyz"), ==, 1);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 2);

  results = dee_index_lookup (fix->index, "xyz", DEE_TERM_MATCH_EXACT);
  g_assert_cmpint (dee_result_set_get_n_rows(results), ==, 1);

  iter = dee_result_set_next(results);
  g_assert (iter == orig_iter);

  /* Change the model and assert that the changes propagate to the index */
  dee_model_set (fix->model, orig_iter, "Hello Yoda", 27);

  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 5);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "Hello world"), ==, 4);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "Hello Yoda"), ==, 1);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "xyz"), ==, 0);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 2);

  dee_model_clear (fix->model);
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "Hello world"), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "Hello Yoda"), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "xyz"), ==, 0);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 0);

  g_object_unref (results);
}

static void
test_text (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter1, *iter2;

  /* 1 row, 2 terms */
  iter1 = dee_model_append (fix->model, "Hello world", 27);
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 1);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "hello"), ==, 1);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "world"), ==, 1);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "xyz"), ==, 0);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 2);
  g_assert (dee_index_lookup_one (fix->index, "hello") == iter1);
  g_assert (dee_index_lookup_one (fix->index, "world") == iter1);

  /* Verify that model.clear() works */
  dee_model_clear (fix->model);
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "hello"), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "world"), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "xyz"), ==, 0);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 0);
  g_assert (dee_index_lookup_one (fix->index, "hello") == NULL);
  g_assert (dee_index_lookup_one (fix->index, "world") == NULL);

  /* 2 rows, 3 terms */
  iter1 = dee_model_append (fix->model, "Hello world", 27);
  iter2 = dee_model_append (fix->model, "Hello... dee?", 68);
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 2);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "hello"), ==, 2);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "world"), ==, 1);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "dee"), ==, 1);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "xyz"), ==, 0);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 3);
  g_assert (dee_index_lookup_one (fix->index, "world") == iter1);
  g_assert (dee_index_lookup_one (fix->index, "dee") == iter2);
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR))
    {
      /* lookup_one() should warn on more than 1 result */
      g_message("This should not print!!! %p",
                dee_index_lookup_one (fix->index, "hello"));
      exit (0);
    }
  g_test_trap_assert_failed();

  /* Verify that model.clear() works with 2 rows and 3 terms */
  dee_model_clear (fix->model);
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "hello"), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "world"), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "xyz"), ==, 0);
  g_assert_cmpint (dee_index_get_n_terms(fix->index), ==, 0);
  g_assert (dee_index_lookup_one (fix->index, "hello") == NULL);
  g_assert (dee_index_lookup_one (fix->index, "world") == NULL);
}

static void
test_text_with_dupe_terms_per_row (Fixture *fix, gconstpointer data)
{
  dee_model_append (fix->model, "Hello hello there!", 27);
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 1);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "hello"), ==, 1);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "there"), ==, 1);
  
  dee_model_clear (fix->model);
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "hello"), ==, 0);
  g_assert_cmpint (dee_index_get_n_rows_for_term(fix->index, "there"), ==, 0);
}

static void
test_prefix_1_row (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter, *result_iter;
  DeeResultSet *results;

  g_assert (dee_index_get_supported_term_match_flags (fix->index) &
            DEE_TERM_MATCH_PREFIX);

  iter = dee_model_append (fix->model, "Hello world", 27);

  results = dee_index_lookup (fix->index, "hello", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 1);
  result_iter = dee_result_set_next (results);
  g_assert (iter == result_iter);
  g_object_unref (results);

  results = dee_index_lookup (fix->index, "hel", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 1);
  result_iter = dee_result_set_next (results);
  g_assert (iter == result_iter);
  g_object_unref (results);

  results = dee_index_lookup (fix->index, "h", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 1);
  result_iter = dee_result_set_next (results);
  g_assert (iter == result_iter);
  g_object_unref (results);

  results = dee_index_lookup (fix->index, "hellop", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 0);
  g_object_unref (results);
}

static void
test_prefix_1_row_multiterm (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter, *result_iter;
  DeeResultSet *results;

  g_assert (dee_index_get_supported_term_match_flags (fix->index) &
            DEE_TERM_MATCH_PREFIX);

  iter = dee_model_append (fix->model, "To test this tremendous trouble", 27);

  results = dee_index_lookup (fix->index, "to", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 1);
  result_iter = dee_result_set_next (results);
  g_assert (iter == result_iter);
  g_object_unref (results);

  results = dee_index_lookup (fix->index, "t", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 1);
  result_iter = dee_result_set_next (results);
  g_assert (iter == result_iter);
  g_object_unref (results);
}

static void
test_prefix_2_rows (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter, *i0, *i1;
  DeeResultSet *results;

  g_assert (dee_index_get_supported_term_match_flags (fix->index) &
            DEE_TERM_MATCH_PREFIX);

  i0 = dee_model_append (fix->model, "Caravan scrap yard", 0);
  i1 = dee_model_append (fix->model, "Scraper foo", 1);

  /* NOTE: We can NOT infer anything about the ordering of the matching
   *       iters inside a single term */
  results = dee_index_lookup (fix->index, "scrap", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 2);
  iter = dee_result_set_next (results);
  g_assert (iter == i0 || iter == i1);
  iter = dee_result_set_next (results);
  g_assert (iter == i0 || iter == i1);
  g_object_unref (results);

  results = dee_index_lookup (fix->index, "scra", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 2);
  iter = dee_result_set_next (results);
  g_assert (iter == i0 || iter == i1);
  iter = dee_result_set_next (results);
  g_assert (iter == i0 || iter == i1);
  g_object_unref (results);

  results = dee_index_lookup (fix->index, "scraper", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 1);
  iter = dee_result_set_next (results);
  g_assert (iter == i1);
  g_object_unref (results);

  results = dee_index_lookup (fix->index, "scraperer", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 0);
  g_object_unref (results);
}

static void
test_prefix_3_rows (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter, *i0, *i1;
  DeeResultSet *results;

  g_assert (dee_index_get_supported_term_match_flags (fix->index) &
            DEE_TERM_MATCH_PREFIX);

  i0 = dee_model_append (fix->model, "Hello world", 0);
  i1 = dee_model_append (fix->model, "Hello dee", 1);
  dee_model_append (fix->model, "Whopping heroes", 2);

  /* NOTE: We can NOT infer anything about the ordering of the matching
   *       iters inside a single term */
  results = dee_index_lookup (fix->index, "hello", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 2);
  iter = dee_result_set_next (results);
  g_assert (iter == i0 || iter == i1);
  iter = dee_result_set_next (results);
  g_assert (iter == i0 || iter == i1);
  g_object_unref (results);

  results = dee_index_lookup (fix->index, "hel", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 2);
  iter = dee_result_set_next (results);
  g_assert (iter == i0 || iter == i1);
  iter = dee_result_set_next (results);
  g_assert (iter == i0 || iter == i1);
  g_object_unref (results);

  results = dee_index_lookup (fix->index, "h", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 3);
  g_object_unref (results);

  /* NOTE: We CAN infer the ordering below because the test model is crafted
   *       so that we have 2 hits each from distinct terms */
  results = dee_index_lookup (fix->index, "w", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 2);
  iter = dee_result_set_next (results);
  g_assert_cmpstr (dee_model_get_string (fix->model, iter, 0), ==, "Whopping heroes");
  iter = dee_result_set_next (results);
  g_assert_cmpstr (dee_model_get_string (fix->model, iter, 0), ==, "Hello world");
  g_object_unref (results);

  results = dee_index_lookup (fix->index, "hellop", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpint (dee_result_set_get_n_rows (results), ==, 0);
  g_object_unref (results);
}

static void
test_prefix_4_rows_and_clear (Fixture *fix, gconstpointer data)
{
  DeeModelIter *i0;
  DeeResultSet *rs;

  dee_model_append (fix->model, "file:///local_uri", 44);
  dee_model_append (fix->model, "http://foo.com", 23);
  dee_model_append (fix->model, "ftp://bar.org", 11);
  i0 = dee_model_append (fix->model, "file:///home/username/file@home", 07);
  
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 4);
  
  rs = dee_index_lookup (fix->index, "user", DEE_TERM_MATCH_PREFIX);
  g_assert (i0 == dee_result_set_next (rs));
  g_assert (!dee_result_set_has_next (rs));
  g_object_unref (rs);
  
  dee_model_clear (fix->model);
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 0);
  
}

static void
test_prefix_search_empty (Fixture *fix, gconstpointer data)
{
  DeeResultSet *rs;

  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 0);
  
  rs = dee_index_lookup (fix->index, "a", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpuint (0, ==, dee_result_set_get_n_rows (rs));
  g_object_unref (rs);
}

static void
test_prefix_search_near_beginning (Fixture *fix, gconstpointer data)
{
  DeeResultSet *rs;

  dee_model_append (fix->model, "vl", 44);
  
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 1);
  
  rs = dee_index_lookup (fix->index, "a", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpuint (0, ==, dee_result_set_get_n_rows (rs));
  g_object_unref (rs);
}

static void
test_prefix_search_near_end (Fixture *fix, gconstpointer data)
{
  DeeResultSet *rs;

  dee_model_append (fix->model, "a", 44);
  
  g_assert_cmpint (dee_index_get_n_rows (fix->index), ==, 1);
  
  rs = dee_index_lookup (fix->index, "vl", DEE_TERM_MATCH_PREFIX);
  g_assert_cmpuint (0, ==, dee_result_set_get_n_rows (rs));
  g_object_unref (rs);
}

void
test_hash_index_create_suite (void)
{
  g_test_add ("/Index/Hash/Empty", Fixture, 0,
              setup_hash, test_empty, teardown);
  g_test_add ("/Index/Tree/Empty", Fixture, 0,
              setup_tree, test_empty, teardown);
  g_test_add ("/Index/Hash/One", Fixture, 0,
              setup_hash, test_one, teardown);
  g_test_add ("/Index/Tree/One", Fixture, 0,
              setup_tree, test_one, teardown);
  g_test_add ("/Index/Hash/Two", Fixture, 0,
              setup_hash, test_two, teardown);
  g_test_add ("/Index/Tree/Two", Fixture, 0,
              setup_tree, test_two, teardown);
  g_test_add ("/Index/WithExistingRows", Fixture, 0,
              setup_tree, test_index_on_existing_rows, teardown);
  g_test_add ("/Index/Hash/Change", Fixture, 0,
              setup_hash, test_change, teardown);
  g_test_add ("/Index/Tree/Change", Fixture, 0,
              setup_tree, test_change, teardown);
  g_test_add ("/Index/Hash/Text", Fixture, 0,
              setup_text_hash, test_text, teardown);
  g_test_add ("/Index/Tree/Text", Fixture, 0,
              setup_text_tree, test_text, teardown);
  g_test_add ("/Index/Hash/TextWithDupeTermsPerRow", Fixture, 0,
              setup_text_hash, test_text_with_dupe_terms_per_row, teardown);
  g_test_add ("/Index/Tree/TextWithDupeTermsPerRow", Fixture, 0,
              setup_text_tree, test_text_with_dupe_terms_per_row, teardown);
  g_test_add ("/Index/Tree/Prefix1Row", Fixture, 0,
              setup_text_tree, test_prefix_1_row, teardown);
  g_test_add ("/Index/Tree/Prefix1RowMultiterm", Fixture, 0,
              setup_text_tree, test_prefix_1_row_multiterm, teardown);
  g_test_add ("/Index/Tree/Prefix2Rows", Fixture, 0,
              setup_text_tree, test_prefix_2_rows, teardown);
  g_test_add ("/Index/Tree/Prefix3Rows", Fixture, 0,
              setup_text_tree, test_prefix_3_rows, teardown);
  g_test_add ("/Index/Tree/Prefix4RowsAndClear", Fixture, 0,
              setup_text_tree, test_prefix_4_rows_and_clear, teardown);
  g_test_add ("/Index/Tree/PrefixSearchEmpty", Fixture, 0,
              setup_text_tree, test_prefix_search_empty, teardown);
  g_test_add ("/Index/Tree/PrefixSearchNearBeginning", Fixture, 0,
              setup_text_tree, test_prefix_search_near_beginning, teardown);
  g_test_add ("/Index/Tree/PrefixSearchNearEnd", Fixture, 0,
              setup_text_tree, test_prefix_search_near_end, teardown);
}
