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
#include <stdlib.h>

typedef struct
{
  DeeModel *m;
} Fixture;

static void sequence_model_setup    (Fixture *fix, gconstpointer data);
static void sequence_model_teardown (Fixture *fix, gconstpointer data);

static void shared_model_setup    (Fixture *fix, gconstpointer data);
static void shared_model_teardown (Fixture *fix, gconstpointer data);

static void test_no_tags (Fixture *fix, gconstpointer data);
static void test_one_tag (Fixture *fix, gconstpointer data);
static void test_two_tags (Fixture *fix, gconstpointer data);
static void test_late_tag (Fixture *fix, gconstpointer data);
static void test_destroy_tag (Fixture *fix, gconstpointer data);
static void test_tag_access_in_row_removed_handler (Fixture *fix, gconstpointer data);

void
test_model_tags_create_suite (void)
{
#define SEQUENCE_MODEL_DOMAIN "/ModelTags/SequenceModel"
#define SHARED_MODEL_DOMAIN "/ModelTags/SharedModel"

  g_test_add (SEQUENCE_MODEL_DOMAIN"/NoTags", Fixture, 0,
              sequence_model_setup, test_no_tags, sequence_model_teardown);

  g_test_add (SHARED_MODEL_DOMAIN"/NoTags", Fixture, 0,
              shared_model_setup, test_no_tags, shared_model_teardown);

  g_test_add (SEQUENCE_MODEL_DOMAIN"/OneTag", Fixture, 0,
              sequence_model_setup, test_one_tag, sequence_model_teardown);

  g_test_add (SHARED_MODEL_DOMAIN"/OneTag", Fixture, 0,
              shared_model_setup, test_one_tag, shared_model_teardown);

  g_test_add (SEQUENCE_MODEL_DOMAIN"/TwoTags", Fixture, 0,
              sequence_model_setup, test_two_tags, sequence_model_teardown);

  g_test_add (SHARED_MODEL_DOMAIN"/TwoTags", Fixture, 0,
              shared_model_setup, test_two_tags, shared_model_teardown);

  g_test_add (SEQUENCE_MODEL_DOMAIN"/LateTag", Fixture, 0,
              sequence_model_setup, test_late_tag, sequence_model_teardown);

  g_test_add (SHARED_MODEL_DOMAIN"/LateTag", Fixture, 0,
              shared_model_setup, test_late_tag, shared_model_teardown);

  g_test_add (SEQUENCE_MODEL_DOMAIN"/DestroyFunc", Fixture, 0,
              sequence_model_setup, test_destroy_tag, sequence_model_teardown);

  g_test_add (SHARED_MODEL_DOMAIN"/DestroyFunc", Fixture, 0,
              shared_model_setup, test_destroy_tag, shared_model_teardown);

  g_test_add (SEQUENCE_MODEL_DOMAIN"/TagAccessInRowRemovedHandler", Fixture, 0,
                sequence_model_setup, test_tag_access_in_row_removed_handler, sequence_model_teardown);

  g_test_add (SHARED_MODEL_DOMAIN"/TagAccessInRowRemovedHandler", Fixture, 0,
                  shared_model_setup, test_tag_access_in_row_removed_handler, shared_model_teardown);
}

static void
sequence_model_setup (Fixture *fix, gconstpointer data)
{
  fix->m = dee_sequence_model_new ();
  dee_model_set_schema (fix->m, "i", "s", NULL);
}

static void
sequence_model_teardown (Fixture *fix, gconstpointer data)
{
  g_object_unref (fix->m);
  fix->m = NULL;
}

static void
shared_model_setup (Fixture *fix, gconstpointer data)
{
  fix->m = dee_shared_model_new ("org.example.ThisIsNotATest");
  dee_model_set_schema (fix->m, "i", "s", NULL);
}

static void
shared_model_teardown (Fixture *fix, gconstpointer data)
{
  g_object_unref (fix->m);
  fix->m = NULL;
}

static void
test_no_tags (Fixture *fix, gconstpointer data)
{
  DeeModelIter *iter;
  gpointer       tag;

  iter = dee_model_append (fix->m, 27, "Hello");

  /* Check that getting an undefined tag fails gracefully */
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR))
    {
      tag = NULL;
      tag = dee_model_get_tag (fix->m, iter, GUINT_TO_POINTER (123));
      g_assert (tag == NULL);
      exit (0); /* successful test run */
    }
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*Unable to look up tag. No tags registered on DeeSequenceModel*");

  /* Ditto for setting undefined tags */
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR))
    {
      dee_model_set_tag (fix->m, iter, GUINT_TO_POINTER (123), GUINT_TO_POINTER (321));
      exit (0); /* successful test run */
    }
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*Unable to look up tag. No tags registered on DeeSequenceModel*");
}

static void
test_one_tag (Fixture *fix, gconstpointer data)
{
  DeeModelTag *tag;
  DeeModelIter *iter1, *iter2;

  tag = dee_model_register_tag (fix->m, NULL);
  iter1 = dee_model_append (fix->m, 27, "Hello");
  iter2 = dee_model_append (fix->m, 68, "world");

  /* Set+Get */
  dee_model_set_tag (fix->m, iter1, tag, "tag1");
  dee_model_set_tag (fix->m, iter2, tag, "tag2");
  g_assert_cmpstr ("tag1", ==, dee_model_get_tag (fix->m, iter1, tag));
  g_assert_cmpstr ("tag2", ==, dee_model_get_tag (fix->m, iter2, tag));

  /* Clear a tag */
  dee_model_clear_tag (fix->m, iter1, tag);
  g_assert (dee_model_get_tag (fix->m, iter1, tag) == NULL);
  g_assert_cmpstr ("tag2", ==, dee_model_get_tag (fix->m, iter2, tag));

  /* Override exiting */
  dee_model_set_tag (fix->m, iter2, tag, "tag3");
  g_assert (dee_model_get_tag (fix->m, iter1, tag) == NULL);
  g_assert_cmpstr ("tag3", ==, dee_model_get_tag (fix->m, iter2, tag));

  /* Check that getting an undefined tag fails gracefully */
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR))
    {
      tag = NULL;
      tag = dee_model_get_tag (fix->m, iter1, GUINT_TO_POINTER (123));
      g_assert (tag == NULL);
      exit (0); /* successful test run */
    }
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*Unable to find tag 123*");

  /* Ditto for setting undefined tags */
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR))
    {
      dee_model_set_tag (fix->m, iter1, GUINT_TO_POINTER (123), GUINT_TO_POINTER (321));
      exit (0); /* successful test run */
    }
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*Unable to find tag 123*");
}

static void
test_two_tags (Fixture *fix, gconstpointer data)
{
  DeeModelTag *tag1, *tag2;
  DeeModelIter *iter1, *iter2;

  tag1 = dee_model_register_tag (fix->m, NULL);
  tag2 = dee_model_register_tag (fix->m, (GDestroyNotify) g_free);

  iter1 = dee_model_append (fix->m, 27, "Hello");
  iter2 = dee_model_append (fix->m, 68, "world");

  dee_model_set_tag (fix->m, iter1, tag1, "tag1-value1");
  dee_model_set_tag (fix->m, iter2, tag1, "tag1-value2");
  dee_model_set_tag (fix->m, iter1, tag2, g_strdup ("tag2-value1"));
  dee_model_set_tag (fix->m, iter2, tag2, g_strdup ("tag2-value2"));

  g_assert_cmpstr ("tag1-value1", ==, dee_model_get_tag (fix->m, iter1, tag1));
  g_assert_cmpstr ("tag1-value2", ==, dee_model_get_tag (fix->m, iter2, tag1));
  g_assert_cmpstr ("tag2-value1", ==, dee_model_get_tag (fix->m, iter1, tag2));
  g_assert_cmpstr ("tag2-value2", ==, dee_model_get_tag (fix->m, iter2, tag2));

  /* Clear one tag. The rest should be unchanged */
  dee_model_clear_tag (fix->m, iter1, tag1);
  g_assert (dee_model_get_tag (fix->m, iter1, tag1) == NULL);
  g_assert_cmpstr ("tag1-value2", ==, dee_model_get_tag (fix->m, iter2, tag1));
  g_assert_cmpstr ("tag2-value1", ==, dee_model_get_tag (fix->m, iter1, tag2));
  g_assert_cmpstr ("tag2-value2", ==, dee_model_get_tag (fix->m, iter2, tag2));
}

static void
test_late_tag (Fixture *fix, gconstpointer data)
{
  /* The point here is to try and register a tag *after* we begun populating
   * the model */

  DeeModelTag *tag1, *tag2;
  DeeModelIter *iter1, *iter2;

  tag1 = dee_model_register_tag (fix->m, NULL);

  iter1 = dee_model_append (fix->m, 27, "Hello");
  iter2 = dee_model_append (fix->m, 68, "world");

  /* Start working randomly with the model */
  dee_model_set_tag (fix->m, iter1, tag1, "tag1-value1");
  g_assert_cmpstr ("tag1-value1", ==, dee_model_get_tag (fix->m, iter1, tag1));
  g_assert (dee_model_get_tag (fix->m, iter2, tag1) == NULL);

  /* With some rows and tags in the model, now register another tag.
   * Assert that all rows have this tag in unset state  */
  tag2 = dee_model_register_tag (fix->m, (GDestroyNotify) g_free);
  g_assert (dee_model_get_tag (fix->m, iter1, tag2) == NULL);
  g_assert (dee_model_get_tag (fix->m, iter2, tag2) == NULL);

  /* Set the new tag and verify the complete state */
  dee_model_set_tag (fix->m, iter1, tag2, g_strdup ("tag2-value1"));
  dee_model_set_tag (fix->m, iter2, tag2, g_strdup ("tag2-value2"));
  g_assert_cmpstr ("tag1-value1", ==, dee_model_get_tag (fix->m, iter1, tag1));
  g_assert (dee_model_get_tag (fix->m, iter2, tag1) == NULL);
  g_assert_cmpstr ("tag2-value1", ==, dee_model_get_tag (fix->m, iter1, tag2));
  g_assert_cmpstr ("tag2-value2", ==, dee_model_get_tag (fix->m, iter2, tag2));
}

static void
destroy_tag (gpointer tag_value)
{
  /* A GDestroyNotify that twiddles with a string */
  ((gchar *) tag_value)[0] = '*';
}

static void
test_destroy_tag (Fixture *fix, gconstpointer data)
{
  /* Assert that the destroy function is indeed called */

  gchar *tag_value = g_strdup ("#");
  DeeModelTag *tag;
  DeeModelIter *iter1;

  tag = dee_model_register_tag (fix->m, destroy_tag);
  iter1 = dee_model_append (fix->m, 27, "Hello");

  /* Set+Get */
  dee_model_set_tag (fix->m, iter1, tag, tag_value);
  g_assert_cmpstr ("#", ==, dee_model_get_tag (fix->m, iter1, tag));

  /* Assert destroy func triggered. Changing "#" to "*" */
  dee_model_clear_tag (fix->m, iter1, tag);
  g_assert (dee_model_get_tag (fix->m, iter1, tag) == NULL);
  g_assert_cmpstr ("*", ==, tag_value);

  /* Change the annotation back to "#" and make sure that destroy is also
   * called when we remove the row */
  tag_value[0] = '#';
  dee_model_set_tag (fix->m, iter1, tag, tag_value);
  g_assert_cmpstr ("#", ==, dee_model_get_tag (fix->m, iter1, tag));
  dee_model_remove (fix->m, iter1);
  g_assert_cmpstr ("*", ==, tag_value);

  g_free (tag_value);
}

static int row_removed_handler_called = 0;
static void
row_removed_handler (DeeModel *model, DeeModelIter *iter, DeeModelTag *tag)
{
  /* Assert that we can read the tag before it's freed. We use a dummy
   * free that sets the tag to "*" to detect this */
  g_assert_cmpstr (dee_model_get_tag (model, iter, tag), ==, "#");
  row_removed_handler_called = 1;
}

static void
test_tag_access_in_row_removed_handler (Fixture *fix, gconstpointer data)
{
  /* Check that we can access a tag in the row-removed handler,
   * before the tag is destroyed */

  gchar *tag_value = g_strdup ("#");
  DeeModelTag *tag;
  DeeModelIter *iter1;

  tag = dee_model_register_tag (fix->m, destroy_tag);
  iter1 = dee_model_append (fix->m, 27, "Hello");
  dee_model_set_tag (fix->m, iter1, tag, tag_value);

  g_signal_connect (fix->m, "row-removed",
                    G_CALLBACK (row_removed_handler), tag);
  dee_model_clear (fix->m);

  g_assert_cmpint (row_removed_handler_called, ==, 1);
  g_assert_cmpstr ("*", ==, tag_value);

  g_free (tag_value);
}
