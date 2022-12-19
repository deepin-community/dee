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
  DeeModel  *model;

} FilterFixture;

typedef struct
{
  gint first;
  gint second;

} TwoIntsTuple;

static void setup       (FilterFixture *fix, gconstpointer data);
static void setup_empty (FilterFixture *fix, gconstpointer data);
static void teardown    (FilterFixture *fix, gconstpointer data);

static void test_empty_orig                    (FilterFixture *fix,
                                                gconstpointer  data);
static void test_append_all                    (FilterFixture *fix,
                                                gconstpointer  data);
static void test_discard_all                   (FilterFixture *fix,
                                                gconstpointer  data);
static void test_discard_all_append_notify     (FilterFixture *fix,
                                                gconstpointer  data);
static void test_change_backend                (FilterFixture *fix,
                                                gconstpointer  data);
static void test_collator_asc                  (FilterFixture *fix,
                                                gconstpointer  data);
static void test_collator_desc                  (FilterFixture *fix,
                                                gconstpointer  data);
static void test_key                           (FilterFixture *fix,
                                                gconstpointer  data);

static void test_any                           (FilterFixture *fix,
                                                gconstpointer  data);

static void test_regex                         (FilterFixture *fix,
                                                gconstpointer  data);

static void test_changesets                    (FilterFixture *fix,
                                                gconstpointer  data);

void
test_filter_model_create_suite (void)
{
#define DOMAIN "/Model/Filter"
  g_test_add (DOMAIN"/EmptyOrig", FilterFixture, 0,
              setup, test_empty_orig, teardown);
  g_test_add (DOMAIN"/AppendAll", FilterFixture, 0,
              setup, test_append_all, teardown);
  g_test_add (DOMAIN"/DiscardAll", FilterFixture, 0,
              setup, test_discard_all, teardown);
  g_test_add (DOMAIN"/DiscardAllAppendNotify", FilterFixture, 0,
              setup, test_discard_all_append_notify, teardown);
  g_test_add (DOMAIN"/ChangeBackend", FilterFixture, 0,
              setup, test_change_backend, teardown);
  g_test_add (DOMAIN"/CollatorAscending", FilterFixture, 0,
              setup, test_collator_asc, teardown);
  g_test_add (DOMAIN"/CollatorDescending", FilterFixture, 0,
              setup, test_collator_desc, teardown);
  g_test_add (DOMAIN"/Key", FilterFixture, 0,
              setup, test_key, teardown);
  g_test_add (DOMAIN"/Any", FilterFixture, 0,
              setup, test_any, teardown);
  g_test_add (DOMAIN"/Regex", FilterFixture, 0,
              setup, test_regex, teardown);
  g_test_add (DOMAIN"/Changesets", FilterFixture, 0,
              setup_empty, test_changesets, teardown);
}

static void
setup_empty (FilterFixture *fix, gconstpointer data)
{
  fix->model = dee_sequence_model_new ();
  dee_model_set_schema (fix->model, "i", "s", NULL);
  
  g_assert (DEE_IS_SEQUENCE_MODEL (fix->model));
}

static void
add_3rows(DeeModel *model)
{
  dee_model_append (model, 0, "Zero");
  dee_model_append (model, 1, "One");
  dee_model_append (model, 2, "Two");
}

static void
setup (FilterFixture *fix, gconstpointer data)
{
  setup_empty (fix, data);
  add_3rows (fix->model);
}

static void
teardown (FilterFixture *fix, gconstpointer data)
{
  g_object_unref (fix->model);
  fix->model = NULL;
}

static void
discard_all_model_map (DeeModel       *orig_model,
                       DeeFilterModel *mapped_model,
                       gpointer  user_data)
{
  /* Don't do anything! */
}

static void
append_all_model_map (DeeModel       *orig_model,
                      DeeFilterModel *mapped_model,
                      gpointer  user_data)
{
  DeeModelIter *iter;
  
  iter = dee_model_get_first_iter (orig_model);
  
  while (!dee_model_is_last (orig_model, iter))
    {
      dee_filter_model_append_iter (mapped_model, iter);
      iter = dee_model_next (orig_model, iter);
    }
}

static gboolean
discard_all_model_notify (DeeModel       *orig_model,
                          DeeModelIter   *orig_iter,
                          DeeFilterModel *mapped_model,
                          gpointer        user_data)
{
  /* Do nothing */

  return FALSE;
}

static gboolean
append_all_model_notify (DeeModel       *orig_model,
                         DeeModelIter   *orig_iter,
                         DeeFilterModel *mapped_model,
                         gpointer        user_data)
{
  /* Always say "Thank you, I am delighted",
   * and append the new row to @mapped_model */
  dee_filter_model_append_iter (mapped_model, orig_iter);

  return TRUE;
}

static void
signal_counter (DeeModel *model, DeeModelIter *iter, guint *count)
{
  *count = *count + 1;
}

/* Test behaviouor when orig_model is empty */
static void
test_empty_orig (FilterFixture *fix, gconstpointer data)
{
  DeeFilter filter;

  dee_filter_new (append_all_model_map,
                  append_all_model_notify,
                  fix,
                  NULL,
                  &filter);
  
  dee_model_clear (fix->model);
  g_assert_cmpint (0, ==, dee_model_get_n_rows (fix->model));
  
  DeeModel *m = dee_filter_model_new (fix->model, &filter);
  g_assert_cmpint (0, ==, dee_model_get_n_rows (fix->model));
  
  DeeModelIter *iter = dee_model_get_first_iter (m);
  g_assert (dee_model_is_first (m, iter));
  g_assert (dee_model_is_last (m, iter));

  g_assert_cmpuint (0, ==, dee_serializable_model_get_seqnum (m));
}

/* A filter model that is a complete copy of the orig */
static void
test_append_all (FilterFixture *fix, gconstpointer data)
{
  DeeFilter filter;

  dee_filter_new (append_all_model_map,
                  append_all_model_notify,
                  fix,
                  NULL,
                  &filter);
  
  DeeModel *m = dee_filter_model_new (fix->model, &filter);
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));
  
  DeeModelIter *iter = dee_model_get_first_iter (m);
  g_assert (dee_model_is_first (m, iter));
  g_assert (!dee_model_is_last (m, iter));
  g_assert_cmpint (0, ==, dee_model_get_int32 (m, iter, 0));
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, iter, 1));
  
  iter = dee_model_next (m, iter);
  g_assert (!dee_model_is_first (m, iter));
  g_assert (!dee_model_is_last (m, iter));
  g_assert_cmpint (1, ==, dee_model_get_int32 (m, iter, 0));
  g_assert_cmpstr ("One", ==, dee_model_get_string (m, iter, 1));
  g_assert (dee_model_get_first_iter (m) == dee_model_prev (m, iter));

  iter = dee_model_next (m, iter);
  g_assert (!dee_model_is_first (m, iter));
  g_assert (!dee_model_is_last (m, iter));
  g_assert_cmpint (2, ==, dee_model_get_int32 (m, iter, 0));
  g_assert_cmpstr ("Two", ==, dee_model_get_string (m, iter, 1));
  g_assert (iter == dee_model_prev (m, dee_model_get_last_iter (m)));

  iter = dee_model_next (m, iter);
  g_assert (!dee_model_is_first (m, iter));
  g_assert (dee_model_is_last (m, iter));
  
  g_assert (dee_model_is_last (m, iter));

  g_assert_cmpuint (dee_serializable_model_get_seqnum (fix->model), ==,
                    dee_serializable_model_get_seqnum (m));
}

/* Test a filter that blocks everything */
static void
test_discard_all (FilterFixture *fix, gconstpointer data)
{
  DeeFilter filter;

  dee_filter_new (discard_all_model_map,
                  discard_all_model_notify,
                  fix,
                  NULL,
                  &filter);
  
  DeeModel *m = dee_filter_model_new (fix->model, &filter);
  
  /* Check expected sizes */
  g_return_if_fail (DEE_IS_FILTER_MODEL (m));
  g_assert_cmpint (0, ==, dee_model_get_n_rows (m));
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));
  
  /* Check that the begin iter is indeed also the end iter */
  DeeModelIter *iter = dee_model_get_first_iter (m);
  g_assert (dee_model_is_first (m, iter));
  g_assert (dee_model_is_last (m, iter));

  /* Check that seqnum is still zero */
  g_assert_cmpuint (0, ==, dee_serializable_model_get_seqnum (m));

  guint filter_add_count = 0;
  guint orig_add_count = 0;
  g_signal_connect (m, "row-added", G_CALLBACK (signal_counter), &filter_add_count);
  g_signal_connect (fix->model, "row-added", G_CALLBACK (signal_counter), &orig_add_count);
  
  dee_model_append (fix->model, 3, "Three");
  dee_model_append (fix->model, 4, "Four");
  
  /* Updates to the orig model should be ignored by this filter */
  g_assert_cmpint (0, ==, filter_add_count);
  g_assert_cmpint (2, ==, orig_add_count);
  g_assert_cmpint (0, ==, dee_model_get_n_rows (m));
  g_assert_cmpint (5, ==, dee_model_get_n_rows (fix->model));
  g_assert_cmpuint (0, ==, dee_serializable_model_get_seqnum (m));
  
  /* Now add stuff to the filtered model directly. This should work,
   * and should be written through to the orig model as well  */
  dee_model_append (m, 27, "TwentySeven");
  g_assert_cmpint (1, ==, filter_add_count);
  g_assert_cmpint (3, ==, orig_add_count);
  g_assert_cmpint (1, ==, dee_model_get_n_rows (m));
  g_assert_cmpint (6, ==, dee_model_get_n_rows (fix->model));
  g_assert_cmpuint (1, ==, dee_serializable_model_get_seqnum (m));
  
  /* The first (and only) row of 'm' should be at offset 5 in fix->model */
  iter = dee_model_get_first_iter (m);
  g_assert (iter == dee_model_get_iter_at_row (fix->model, 5));
  g_assert_cmpint (27, ==, dee_model_get_int32 (m, iter, 0));
  g_assert_cmpint (27, ==, dee_model_get_int32 (fix->model, iter, 0));
  g_assert_cmpstr ("TwentySeven", ==, dee_model_get_string (m, iter, 1));
  g_assert_cmpstr ("TwentySeven", ==, dee_model_get_string (fix->model, iter, 1));

  /* And append two more rows to the filtered model, to ensure the order */  
  dee_model_prepend (m, -1, "MinusOne");
  dee_model_append (m, 39, "ThirtyNine");
  g_assert_cmpint (3, ==, filter_add_count);
  g_assert_cmpint (5, ==, orig_add_count);
  g_assert_cmpint (3, ==, dee_model_get_n_rows (m));
  g_assert_cmpint (8, ==, dee_model_get_n_rows (fix->model));
  g_assert_cmpuint (3, ==, dee_serializable_model_get_seqnum (m));
  iter = dee_model_prev (m, dee_model_get_last_iter (m));
  g_assert_cmpint (39, ==, dee_model_get_int32 (m, iter, 0));
  g_assert_cmpstr ("ThirtyNine", ==, dee_model_get_string (m, iter, 1));
  iter = dee_model_prev (m, iter);
  g_assert_cmpint (27, ==, dee_model_get_int32 (m, iter, 0));
  g_assert_cmpstr ("TwentySeven", ==, dee_model_get_string (m, iter, 1));
  iter = dee_model_prev (m, iter);
  g_assert_cmpint (-1, ==, dee_model_get_int32 (m, iter, 0));
  g_assert_cmpstr ("MinusOne", ==, dee_model_get_string (m, iter, 1));
  iter = dee_model_prev (fix->model, dee_model_get_last_iter (fix->model));
  g_assert_cmpint (39, ==, dee_model_get_int32 (fix->model, iter, 0));
  g_assert_cmpstr ("ThirtyNine", ==, dee_model_get_string (fix->model, iter, 1));

  g_object_unref (m);
}

/* Test a filter that blocks everything */
static void
test_discard_all_append_notify (FilterFixture *fix, gconstpointer data)
{
  DeeFilter filter;
  
  dee_filter_new (discard_all_model_map,
                  append_all_model_notify,
                  fix,
                  NULL,
                  &filter);

  DeeModel *m = dee_filter_model_new (fix->model, &filter);

  /* Nothing was added to the filter model, seqnum should be zero */
  g_assert_cmpuint (0, ==, dee_serializable_model_get_seqnum (m));
  
  guint filter_add_count = 0;
  guint orig_add_count = 0;
  g_signal_connect (m, "row-added", G_CALLBACK (signal_counter), &filter_add_count);
  g_signal_connect (fix->model, "row-added", G_CALLBACK (signal_counter), &orig_add_count);
  
  dee_model_append (fix->model, 3, "Three");
  dee_model_append (fix->model, 4, "Four");
  
  /* Updates to the orig model should be detected by this filter */
  g_assert_cmpint (2, ==, filter_add_count);
  g_assert_cmpint (2, ==, orig_add_count);
  g_assert_cmpint (2, ==, dee_model_get_n_rows (m));
  g_assert_cmpint (5, ==, dee_model_get_n_rows (fix->model));
  g_assert_cmpuint (2, ==, dee_serializable_model_get_seqnum (m));
  
  /* The first row of 'm' should be at offset 3 in fix->model */
  DeeModelIter *iter = dee_model_get_first_iter (m);
  g_assert (iter == dee_model_get_iter_at_row (fix->model, 3));
  g_assert_cmpint (3, ==, dee_model_get_int32 (m, iter, 0));
  g_assert_cmpint (3, ==, dee_model_get_int32 (fix->model, iter, 0));
  g_assert_cmpstr ("Three", ==, dee_model_get_string (m, iter, 1));
  g_assert_cmpstr ("Three", ==, dee_model_get_string (fix->model, iter, 1));
  
  /* The second row of 'm' should be at offset 4 in fix->model */
  iter = dee_model_next (m, iter);
  g_assert (iter == dee_model_get_iter_at_row (fix->model, 4));
  g_assert_cmpint (4, ==, dee_model_get_int32 (m, iter, 0));
  g_assert_cmpint (4, ==, dee_model_get_int32 (fix->model, iter, 0));
  g_assert_cmpstr ("Four", ==, dee_model_get_string (m, iter, 1));
  g_assert_cmpstr ("Four", ==, dee_model_get_string (fix->model, iter, 1));
  
  /* Assert that the next iter in 'm is the end iter */
  iter = dee_model_next (m, iter);
  g_assert (dee_model_is_last (m, iter));
  
  g_object_unref (m);
}

static void
test_change_backend (FilterFixture *fix, gconstpointer data)
{
  DeeFilter filter;

  dee_filter_new (append_all_model_map,
                  append_all_model_notify,
                  fix,
                  NULL,
                  &filter);
  
  DeeModel *m = dee_filter_model_new (fix->model, &filter);
  g_assert_cmpint (3, ==, dee_model_get_n_rows (fix->model));
  
  DeeModelIter *iter = dee_model_get_first_iter (m);
  g_assert (dee_model_is_first (m, iter));
  g_assert (!dee_model_is_last (m, iter));
  g_assert_cmpint (0, ==, dee_model_get_int32 (m, iter, 0));
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, iter, 1));
  g_assert_cmpuint (0, ==, dee_model_get_position (m, iter));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_remove (fix->model, iter);

  g_assert_cmpint (2, ==, dee_model_get_n_rows (fix->model));
  g_assert_cmpint (2, ==, dee_model_get_n_rows (m));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_remove (fix->model, iter);

  g_assert_cmpint (1, ==, dee_model_get_n_rows (fix->model));
  g_assert_cmpint (1, ==, dee_model_get_n_rows (m));

  iter = dee_model_get_first_iter (m);
  g_assert (dee_model_is_first (m, iter));
  g_assert (!dee_model_is_last (m, iter));
  g_assert_cmpint (2, ==, dee_model_get_int32 (m, iter, 0));
  g_assert_cmpstr ("Two", ==, dee_model_get_string (m, iter, 1));

  iter = dee_model_get_first_iter (fix->model);
  dee_model_set (fix->model, iter, -1, "Minus one");

  iter = dee_model_get_first_iter (m);
  g_assert (dee_model_is_first (m, iter));
  g_assert (!dee_model_is_last (m, iter));
  g_assert_cmpint (-1, ==, dee_model_get_int32 (m, iter, 0));
  g_assert_cmpstr ("Minus one", ==, dee_model_get_string (m, iter, 1));
  g_assert_cmpuint (0, ==, dee_model_get_position (m, iter));

  /* and finally remove the last row via the filter model */
  dee_model_remove (m, iter);

  g_assert_cmpint (0, ==, dee_model_get_n_rows (m));
  g_assert_cmpint (0, ==, dee_model_get_n_rows (fix->model));
}

/* Test dee_filter_new_collator() ascending */
static void
test_collator_asc (FilterFixture *fix, gconstpointer data)
{
  DeeModelIter *r0, *r1, *r2, *r3, *r4, *r5;
  DeeFilter    collator;
  DeeModel     *m;

  dee_filter_new_collator (1, &collator);
  m  = dee_filter_model_new (fix->model, &collator);

  /* Test alphabetic sorting after initial construction */
  r0 = dee_model_get_iter_at_row (m, 0);
  r1 = dee_model_get_iter_at_row (m, 1);
  r2 = dee_model_get_iter_at_row (m, 2);
  g_assert_cmpstr ("One", ==, dee_model_get_string (m, r0, 1));
  g_assert_cmpstr ("Two", ==, dee_model_get_string (m, r1, 1));
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, r2, 1));

  /* Test alphabetic sorting after updates */
  dee_model_append (fix->model, 3, "Three");
  dee_model_append (fix->model, 4, "Four");
  r0 = dee_model_get_iter_at_row (m, 0);
  r1 = dee_model_get_iter_at_row (m, 1);
  r2 = dee_model_get_iter_at_row (m, 2);
  r3 = dee_model_get_iter_at_row (m, 3);
  r4 = dee_model_get_iter_at_row (m, 4);
  g_assert_cmpstr ("Four", ==, dee_model_get_string (m, r0, 1));
  g_assert_cmpstr ("One", ==, dee_model_get_string (m, r1, 1));
  g_assert_cmpstr ("Three", ==, dee_model_get_string (m, r2, 1));
  g_assert_cmpstr ("Two", ==, dee_model_get_string (m, r3, 1));
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, r4, 1));

  /* Appending to the end of the sorted model is a special case in the code,
   * so double check on that... */
  dee_model_append (fix->model, 5, "Zzzz");
  r0 = dee_model_get_iter_at_row (m, 0);
  r5 = dee_model_get_iter_at_row (m, 5);
  g_assert_cmpstr ("Four", ==, dee_model_get_string (m, r0, 1));
  g_assert_cmpstr ("Zzzz", ==, dee_model_get_string (m, r5, 1));

  g_object_unref (m);
}

/* Test dee_filter_new_collator_desc() descending*/
static void
test_collator_desc (FilterFixture *fix, gconstpointer data)
{
  DeeModelIter *r0, *r1, *r2, *r3, *r4, *r5;
  DeeFilter    collator;
  DeeModel     *m;

  dee_filter_new_collator_desc (1, &collator);
  m  = dee_filter_model_new (fix->model, &collator);

  /* Test alphabetic sorting after initial construction */
  r0 = dee_model_get_iter_at_row (m, 0);
  r1 = dee_model_get_iter_at_row (m, 1);
  r2 = dee_model_get_iter_at_row (m, 2);
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, r0, 1));
  g_assert_cmpstr ("Two", ==, dee_model_get_string (m, r1, 1));
  g_assert_cmpstr ("One", ==, dee_model_get_string (m, r2, 1));

  /* Test alphabetic sorting after updates */
  dee_model_append (fix->model, 3, "Three");
  dee_model_append (fix->model, 4, "Four");
  r0 = dee_model_get_iter_at_row (m, 0);
  r1 = dee_model_get_iter_at_row (m, 1);
  r2 = dee_model_get_iter_at_row (m, 2);
  r3 = dee_model_get_iter_at_row (m, 3);
  r4 = dee_model_get_iter_at_row (m, 4);
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, r0, 1));
  g_assert_cmpstr ("Two", ==, dee_model_get_string (m, r1, 1));
  g_assert_cmpstr ("Three", ==, dee_model_get_string (m, r2, 1));
  g_assert_cmpstr ("One", ==, dee_model_get_string (m, r3, 1));
  g_assert_cmpstr ("Four", ==, dee_model_get_string (m, r4, 1));

  /* Appending to the end of the sorted model is a special case in the code,
   * so double check on that... */
  dee_model_append (fix->model, 5, "Zzzz");
  r0 = dee_model_get_iter_at_row (m, 0);
  r5 = dee_model_get_iter_at_row (m, 5);
  g_assert_cmpstr ("Four", ==, dee_model_get_string (m, r5, 1));
  g_assert_cmpstr ("Zzzz", ==, dee_model_get_string (m, r0, 1));

  g_object_unref (m);
}

static void
_test_orig_ordering (FilterFixture *fix,
                     DeeFilter     *filter)
{
  DeeModelIter *r0, *r1, *r2, *r3, *r4;//, *r5;
  DeeModel     *m = dee_filter_model_new (fix->model, filter);

  /* Assert that the initial filtering is good:
   * { [  0, "Zero" ] }        { [  0, "Zero" ],
   *                             [  1, "One"  ],
   *                             [  2, "Two"  ]}
   * */
  g_assert_cmpint (1, ==, dee_model_get_n_rows (m));
  r0 = dee_model_get_first_iter (m);
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, r0, 1));
  g_assert_cmpint (0, ==, dee_model_get_int32 (m, r0, 0));
  g_assert (dee_model_next (m, r0) == dee_model_get_last_iter (m));

  /* Assert that we can append rows:
   * { [  0, "Zero" ],        { [  0, "Zero" ],
   *   [ 12, "Zero" ],          [  1, "One"  ],
   *   [ 13, "Zero" ] }         [  2, "Two"  ],
   *                            [  1, "One"  ],
   *                            [ 12, "Zero" ],
   *                            [ 13, "Zero" ] }
   * */
  dee_model_append (fix->model, 11, "One");  // Discard
  dee_model_append (fix->model, 12, "Zero"); // Include
  dee_model_append (fix->model, 13, "Zero"); // Include
  g_assert_cmpint (3, ==, dee_model_get_n_rows (m));
  r0 = dee_model_get_iter_at_row (m, 0);
  r1 = dee_model_get_iter_at_row (m, 1);
  r2 = dee_model_get_iter_at_row (m, 2);
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, r0, 1));
  g_assert_cmpint (0, ==, dee_model_get_int32 (m, r0, 0));
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, r1, 1));
  g_assert_cmpint (12, ==, dee_model_get_int32 (m, r1, 0));
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, r2, 1));
  g_assert_cmpint (13, ==, dee_model_get_int32 (m, r2, 0));

  /* Assert sorting is correct for prepends:
   * { [ -1, "Zero" ],      { [ -1, "Zero" ],
   *   [  0, "Zero" ],        [  0, "Zero" ],
   *   [ 12, "Zero" ],        [  1, "One"  ],
   *   [ 13, "Zero" ] }       [  2, "Two"  ],
   *                          [  1, "One"  ],
   *                          [ 12, "Zero" ],
   *                          [ 13, "Zero" ] }
   *   */
  dee_model_prepend (fix->model, -1, "Zero");
  g_assert_cmpint (4, ==, dee_model_get_n_rows (m));
  r0 = dee_model_get_first_iter (m);
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, r0, 1));
  g_assert_cmpint (-1, ==, dee_model_get_int32 (m, r0, 0));

  /* Assert sorting is correct for inserts before a row that is already
   * in the filtered model:
   * { [ -1, "Zero" ],      { [ -1, "Zero" ],
   *   [ -2, "Zero" ],        [ -2, "Zero" ],
   *   [  0, "Zero" ],        [  0, "Zero" ],
   *   [ 12, "Zero" ],        [  1, "One"  ],
   *   [ 13, "Zero" ] }       [  2, "Two"  ],
   *                          [  1, "One"  ],
   *                          [ 12, "Zero" ],
   *                          [ 13, "Zero" ] }
   *   */
  r1 = dee_model_get_iter_at_row (m, 1); // The (0, "Zero") row
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, r1, 1));
  g_assert_cmpint (0, ==, dee_model_get_int32 (m, r1, 0));
  dee_model_insert_before (fix->model, r1, -2, "Zero");
  r1 = dee_model_get_iter_at_row (m, 1);
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, r1, 1));
  g_assert_cmpint (-2, ==, dee_model_get_int32 (m, r1, 0));

  /* Assert sorting is correct for inserts before a row that is *not* included
   * in the filtered model:
   * { [ -1, "Zero" ],      { [ -1, "Zero" ],
   *   [ -2, "Zero" ],        [ -2, "Zero" ],
   *   [  0, "Zero" ],        [  0, "Zero" ],
   *   [ -3, "Zero" ],        [  1, "One"  ],
   *   [ 12, "Zero" ],        [ -3, "Zero" ]
   *                          [  2, "Two"  ],
   *   [ 13, "Zero" ] }       [  1, "One"  ],
   *                          [ 12, "Zero" ],
   *                          [ 13, "Zero" ] }
   *   */
  r4 = dee_model_get_iter_at_row (fix->model, 4);
  g_assert_cmpstr ("Two", ==, dee_model_get_string (fix->model, r4, 1));
  g_assert (!dee_filter_model_contains (DEE_FILTER_MODEL (m), r4));
  dee_model_insert_before (fix->model, r4, -3, "Zero");
  r3 = dee_model_get_iter_at_row (m, 3);
  g_assert_cmpstr ("Zero", ==, dee_model_get_string (m, r3, 1));
  g_assert_cmpint (-3, ==, dee_model_get_int32 (m, r3, 0));

  g_object_unref (m);
}

/* Test dee_filter_new_for_key_column() */
static void
test_key (FilterFixture *fix, gconstpointer data)
{
  DeeFilter    filter;

  dee_filter_new_for_key_column (1, "Zero", &filter);
  _test_orig_ordering (fix, &filter);
}

/* Test dee_filter_new_for_any_column() */
static void
test_any (FilterFixture *fix, gconstpointer data)
{
  DeeFilter    filter;

  dee_filter_new_for_any_column (1, g_variant_new_string ("Zero"), &filter);

  _test_orig_ordering (fix, &filter);
}

/* Test dee_filter_new_regex() */
static void
test_regex (FilterFixture *fix, gconstpointer data)
{
  DeeFilter    filter;
  GRegex       *regex;

  regex = g_regex_new (".ero", 0, 0, NULL);
  dee_filter_new_regex (1, regex, &filter);

  _test_orig_ordering (fix, &filter);
  g_regex_unref (regex);
}

static void
increment_first (TwoIntsTuple *tuple)
{
  tuple->first++;
  // first always has to be incremented before second
  g_assert (tuple->first > tuple->second);
}

static void
increment_second (TwoIntsTuple *tuple)
{
  tuple->second++;
  // second needs to be incremented after first
  g_assert (tuple->second == tuple->first);
}

static void
test_changesets (FilterFixture *fix, gconstpointer data)
{
  GRegex       *regex;
  DeeFilter     filter;
  DeeModel     *filter_m1;
  DeeModel     *filter_m2;
  DeeModel     *filter_m3;

  regex = g_regex_new ("^..[eo]", 0, 0, NULL);
  dee_filter_new_regex (1, regex, &filter);
  filter_m1 = dee_filter_model_new (fix->model, &filter);
  g_regex_unref (regex);

  regex = g_regex_new ("^Z", 0, 0, NULL);
  dee_filter_new_regex (1, regex, &filter);
  filter_m2 = dee_filter_model_new (fix->model, &filter);
  g_regex_unref (regex);

  regex = g_regex_new ("^X", 0, 0, NULL);
  dee_filter_new_regex (1, regex, &filter);
  filter_m3 = dee_filter_model_new (fix->model, &filter);
  g_regex_unref (regex);

  g_assert_cmpuint (0, ==, dee_model_get_n_rows (fix->model));
  g_assert_cmpuint (0, ==, dee_model_get_n_rows (filter_m1));
  g_assert_cmpuint (0, ==, dee_model_get_n_rows (filter_m2));
  g_assert_cmpuint (0, ==, dee_model_get_n_rows (filter_m3));

  TwoIntsTuple tuple_m0 = { 0, 0 };
  TwoIntsTuple tuple_m1 = { 0, 0 };
  TwoIntsTuple tuple_m2 = { 0, 0 };
  TwoIntsTuple tuple_m3 = { 0, 0 };

  g_signal_connect_swapped (fix->model, "changeset-started",
                            G_CALLBACK (increment_first), &tuple_m0);
  g_signal_connect_swapped (fix->model, "changeset-finished",
                            G_CALLBACK (increment_second), &tuple_m0);
  g_signal_connect_swapped (filter_m1, "changeset-started",
                            G_CALLBACK (increment_first), &tuple_m1);
  g_signal_connect_swapped (filter_m1, "changeset-finished",
                            G_CALLBACK (increment_second), &tuple_m1);
  g_signal_connect_swapped (filter_m2, "changeset-started",
                            G_CALLBACK (increment_first), &tuple_m2);
  g_signal_connect_swapped (filter_m2, "changeset-finished",
                            G_CALLBACK (increment_second), &tuple_m2);
  g_signal_connect_swapped (filter_m3, "changeset-started",
                            G_CALLBACK (increment_first), &tuple_m3);
  g_signal_connect_swapped (filter_m3, "changeset-finished",
                            G_CALLBACK (increment_second), &tuple_m3);

  dee_model_begin_changeset (fix->model);
  add_3rows (fix->model);
  dee_model_end_changeset (fix->model);

  g_assert_cmpuint (3, ==, dee_model_get_n_rows (fix->model));
  g_assert_cmpuint (2, ==, dee_model_get_n_rows (filter_m1));
  g_assert_cmpuint (1, ==, dee_model_get_n_rows (filter_m2));
  g_assert_cmpuint (0, ==, dee_model_get_n_rows (filter_m3));
  g_assert_cmpint (tuple_m0.first, ==, 1);
  g_assert_cmpint (tuple_m0.second, ==, 1);
  g_assert_cmpint (tuple_m1.first, ==, 1);
  g_assert_cmpint (tuple_m1.second, ==, 1);
  g_assert_cmpint (tuple_m2.first, ==, 1);
  g_assert_cmpint (tuple_m2.second, ==, 1);
  g_assert_cmpint (tuple_m3.first, ==, 1);
  g_assert_cmpint (tuple_m3.second, ==, 1);
}
