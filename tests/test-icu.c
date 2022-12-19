/*
 * Copyright (C) 2012 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authored by Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 */

#include <unicode/uvernum.h>
#include <dee.h>
#include <dee-icu.h>

typedef struct
{
  DeeICUTermFilter *filter;
} Fixture;

static void setup    (Fixture *fix, gconstpointer data);
static void teardown (Fixture *fix, gconstpointer data);

static void
setup (Fixture *fix, gconstpointer data)
{
  fix->filter = NULL;
}

static void
teardown (Fixture *fix, gconstpointer data)
{
  if (fix->filter)
    dee_icu_term_filter_destroy (fix->filter);
}

static void
test_ascii_folder (Fixture *fix, gconstpointer data)
{
  gchar *result;

  fix->filter = dee_icu_term_filter_new_ascii_folder();

  result = dee_icu_term_filter_apply (fix->filter, "Hello world");
  g_assert_cmpstr (result, ==, "Hello world");
  g_free (result);

  result = dee_icu_term_filter_apply (fix->filter, "øöô");
  g_assert_cmpstr (result, ==, "ooo");
  g_free (result);

  result = dee_icu_term_filter_apply (fix->filter, "Θεοδωράτου, Ελένη");
  g_assert_cmpstr (result, ==, "Theodoratou, Elene");
  g_free (result);

  result = dee_icu_term_filter_apply (fix->filter, "Догилева, Татьяна");
#if U_ICU_VERSION_MAJOR_NUM > 55
  g_assert_cmpstr (result, ==, "Dogileva, Tat'ana");
#else
  g_assert_cmpstr (result, ==, "Dogileva, Tatʹana");
#endif
  g_free (result);

  result = dee_icu_term_filter_apply (fix->filter, "김, 국삼");
  g_assert_cmpstr (result, ==,  "gim, gugsam");
  g_free (result);

  result = dee_icu_term_filter_apply (fix->filter, "たけだ, まさゆき");
  g_assert_cmpstr (result, ==, "takeda, masayuki");
  g_free (result);

  /* One last time to honor the French ;-) */
  result = dee_icu_term_filter_apply (fix->filter, "Est-ce que tes enfants sont de bons élèves? Ça va pour eux?");
  g_assert_cmpstr (result, ==, "Est-ce que tes enfants sont de bons eleves? Ca va pour eux?");
  g_free (result);
}

static void
test_bad_id (Fixture *fix, gconstpointer data)
{
  GError *error = NULL;

  fix->filter = dee_icu_term_filter_new ("*-sad ???", NULL, &error);

  g_assert (fix->filter == NULL);
  g_assert (error != NULL);
  g_assert_cmpint (error->domain, ==, DEE_ICU_ERROR);
  g_assert_cmpint (error->code, ==, DEE_ICU_ERROR_BAD_ID);

  g_error_free (error);
}

static void
test_bad_rule (Fixture *fix, gconstpointer data)
{
  GError *error = NULL;

  fix->filter = dee_icu_term_filter_new (NULL, "*-sad ???", &error);

  g_assert (fix->filter == NULL);
  g_assert (error != NULL);
  g_assert_cmpint (error->domain, ==, DEE_ICU_ERROR);
  g_assert_cmpint (error->code, ==, DEE_ICU_ERROR_BAD_RULE);

  g_error_free (error);
}

void
test_icu_create_suite (void)
{
#define DOMAIN "/ICU/TermFilter"

  g_test_add (DOMAIN"/Default/AsciiFolder", Fixture, 0,
              setup, test_ascii_folder, teardown);
  g_test_add (DOMAIN"/Default/BadId", Fixture, 0,
              setup, test_bad_id, teardown);
  g_test_add (DOMAIN"/Default/BadRule", Fixture, 0,
              setup, test_bad_rule, teardown);
}
