/*
 * Copyright (C) 2009 Canonical Ltd
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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include <locale.h>

void test_model_column_create_suite (void);
void test_model_complex_column_create_suite (void);
void test_model_rows_create_suite (void);
void test_model_signals_create_suite (void);
void test_model_seqnums_create_suite (void);
void test_model_tags_create_suite (void);
void test_filter_model_create_suite (void);
void test_term_list_create_suite (void);
void test_hash_index_create_suite (void);
void test_analyzer_create_suite (void);
void test_model_readers_create_suite (void);
void test_glist_result_set_create_suite (void);
void test_serializable_create_suite (void);
void test_resource_manager_create_suite (void);
void test_transaction_create_suite (void);

#ifdef HAVE_GTX
void test_model_interactions_create_suite(void);
void test_peer_interactions_create_suite(void);
void test_client_server_interactions_create_suite(void);
#endif  /* HAVE_GTX */

#ifdef HAVE_ICU
void test_icu_create_suite(void);
#endif  /* HAVE_ICU */

gint
main (gint argc, gchar *argv[])
{
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init (); 
#endif

  g_test_init (&argc, &argv, NULL);
  setlocale (LC_ALL, "");

  test_model_column_create_suite ();
  test_model_complex_column_create_suite ();
  test_model_rows_create_suite ();
  test_model_signals_create_suite ();
  test_model_seqnums_create_suite ();
  test_model_tags_create_suite ();
  test_filter_model_create_suite ();
  test_term_list_create_suite ();
  test_hash_index_create_suite ();
  test_analyzer_create_suite ();
  test_model_readers_create_suite ();
  test_glist_result_set_create_suite ();
  test_serializable_create_suite ();
  test_resource_manager_create_suite ();
  test_transaction_create_suite ();

#ifdef HAVE_GTX
  test_model_interactions_create_suite();
  test_peer_interactions_create_suite();
  test_client_server_interactions_create_suite();
#else
  g_message ("Interactions' test suite disabled. GTX not found. You can download GTX from https://launchpad.net/gtx");
#endif  /* HAVE_GTX */
  
#ifdef HAVE_ICU
  test_icu_create_suite ();
#endif

  return g_test_run ();
}
