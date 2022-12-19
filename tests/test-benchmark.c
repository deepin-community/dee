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

#include "config.h"
#include <math.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib-object.h>

#include <dee.h>

typedef struct _Benchmark Benchmark;
typedef void (*BenchmarkFunc) (Benchmark *benchmark);
typedef void (*BenchmarkSetup) (Benchmark *benchmark);
typedef void (*BenchmarkTeardown) (Benchmark *benchmark);

typedef struct {
  gdouble elapsed;
} RunData;

struct _Benchmark {
  const gchar       *name;
  BenchmarkSetup     benchmark_setup;
  BenchmarkFunc      benchmark_func;
  BenchmarkTeardown  benchmark_teardown;
  guint              n_runs;
  RunData           *runs;
  gpointer           state;
};

static GList *benchmarks = NULL;

static void
add_benchmark (Benchmark *benchmark)
{
  benchmarks = g_list_append (benchmarks, benchmark);
}

static void
run_benchmark (Benchmark *bench)
{
  GTimer *timer;
  guint   i;
  gdouble total_runtime, avg_runtime, std_dev, coeff_of_var;
  const gchar *coeff_of_var_msg;
  
  bench->runs = g_new0 (RunData, bench->n_runs + 1);
  timer = g_timer_new ();
  total_runtime = 0;
  
  bench->benchmark_setup (bench);
  
  g_printf ("=== %s ===\n", bench->name);
      
  for (i = 0; i < bench->n_runs; i++)
  {
    g_timer_start (timer);
    bench->benchmark_func (bench);
    bench->runs[i].elapsed = g_timer_elapsed (timer, NULL);
    total_runtime += bench->runs[i].elapsed;
    
    /* Some benchmarks will reset their own state */
    if (bench->state == NULL)
      bench->benchmark_setup (bench);
  }
  
  /* Compute average runtime */
  avg_runtime = total_runtime / bench->n_runs;
  
  /* Compute standard deviation */
  std_dev = 0;
  for (i = 0; i < bench->n_runs; i++)
    {
      gdouble delta = bench->runs[i].elapsed - avg_runtime;
      std_dev += delta * delta;
    }
  std_dev = sqrt (std_dev / bench->n_runs);
  coeff_of_var = (std_dev /avg_runtime) * 100;
  
  /* This is not a very precise way of judging the quality,
  *  but better than rigorous hand waving */
  if (coeff_of_var < 1.0)
    coeff_of_var_msg = "super!";
  else if (coeff_of_var < 5)
    coeff_of_var_msg = "good";
  else if (coeff_of_var < 10)
    coeff_of_var_msg = "acceptable";
  else
    coeff_of_var_msg = "rejected!";
  
  /* Print report */
  g_printf ("Runs           : %u\n", bench->n_runs);
  g_printf ("Total runtime  : %fs\n", total_runtime);
  g_printf ("Avg. runtime   : %fs\n", avg_runtime);
  g_printf ("Std. deviation : %fs\n", std_dev);
  g_printf ("Accuracy       : %f%% [%s]\n", (100 - coeff_of_var), coeff_of_var_msg);
  g_printf ("\n");
  
  
  
  g_timer_destroy (timer);
  bench->benchmark_teardown (bench);
  // purposely leak bench->runs. Caller may want it
}

static void
run_benchmarks (gchar **prefixes)
{
  GList  *iter;
  gchar  **prefix;
  
  for (iter = benchmarks; iter; iter = iter->next)
    {
      Benchmark *bench = (Benchmark*) iter->data;
      
      if (prefixes == NULL)
        run_benchmark (bench);
      else
        {
          for (prefix = prefixes; *prefix; prefix++)
            {
              if (g_str_has_prefix (bench->name, *prefix))
                {
                  run_benchmark (bench);
                }
            }
        }
    }
}

static void
bench_seqmodel_setup (Benchmark *bench)
{
  DeeModel *model;
  
  model = dee_sequence_model_new ();
  dee_model_set_schema (model, "s", "s", "s", "u", "b", NULL);
  bench->state = model;
}

static void
bench_seqmodel_named_setup (Benchmark *bench)
{
  bench_seqmodel_setup (bench);

  dee_model_set_column_names (bench->state,
                              "string1", "string2", "string3",
                              "count", "bool", NULL);
}

static void
bench_filter_model_collator_setup (Benchmark *bench)
{
  DeeModel *fmodel, *base_model;
  DeeFilter filter;
  
  base_model = dee_sequence_model_new ();
  dee_model_set_schema (base_model, "s", "s", "s", "u", "b", NULL);
  
  dee_filter_new_collator (0, &filter);
  fmodel = dee_filter_model_new (base_model, &filter);
  
  g_object_unref (base_model);
    
  bench->state = fmodel;
}

static void
bench_filter_model_collator_desc_setup (Benchmark *bench)
{
  DeeModel *fmodel, *base_model;
  DeeFilter filter;
  
  base_model = dee_sequence_model_new ();
  dee_model_set_schema (base_model, "s", "s", "s", "u", "b", NULL);
  
  dee_filter_new_collator_desc (0, &filter);
  fmodel = dee_filter_model_new (base_model, &filter);
  
  g_object_unref (base_model);
    
  bench->state = fmodel;
}

static gint
_cmp_uint (GVariant **row1, GVariant **row2, gpointer user_data)
{
  return g_variant_get_uint32 (row1[3]) - g_variant_get_uint32 (row2[3]);
}

static void
bench_filter_model_sort_uint_setup (Benchmark *bench)
{
  DeeModel *fmodel, *base_model;
  DeeFilter filter;

  base_model = dee_sequence_model_new ();
  dee_model_set_schema (base_model, "s", "s", "s", "u", "b", NULL);

  dee_filter_new_sort(_cmp_uint, NULL, NULL, &filter);
  fmodel = dee_filter_model_new (base_model, &filter);

  g_object_unref (base_model);

  bench->state = fmodel;
}

static void
bench_seqmodel_read_string_setup (Benchmark *bench)
{
  DeeModel *model;
  guint     limit = 25000, i;
  
  model = dee_sequence_model_new ();
  dee_model_set_schema (model, "s", "s", "s", "u", "b", NULL);

  for (i = 0; i < limit; i++)
    {
      gchar *random_string = g_strdup_printf ("%"G_GINT32_FORMAT,
                                              g_test_rand_int());
      dee_model_prepend (model, random_string, "Hello world", "!", 42, TRUE);
      g_free (random_string);
    }

  bench->state = model;
}

static void
bench_index_setup (Benchmark *bench)
{
  DeeModel *model;
  DeeIndex *index;
  DeeModelReader reader;
  guint     limit = 25000, i;
  
  model = dee_sequence_model_new ();
  dee_model_reader_new_for_string_column (0, &reader);
  dee_model_set_schema (model, "s", "s", "s", "u", "b", NULL);
  index = DEE_INDEX (dee_tree_index_new (model, dee_analyzer_new (), &reader));

  for (i = 0; i < limit; i++)
    {
      gchar *random_string = g_strdup_printf ("%"G_GINT32_FORMAT,
                                              g_test_rand_int());
      dee_model_prepend (model, random_string, "Hello world", "!", 42, TRUE);
      g_free (random_string);
    }

  bench->state = index;
}

static void
bench_model_append_run (Benchmark *bench)
{
  DeeModel *model;
  guint     limit = 2500, i;
  
  g_assert (DEE_IS_MODEL (bench->state));
  
  model = DEE_MODEL (bench->state);
  
  for (i = 0; i < limit; i++)
    {
      gchar *random_string = g_strdup_printf ("%"G_GINT32_FORMAT,
                                              g_test_rand_int ());
      dee_model_append (model, random_string, "Hello world", "!",
                        (guint32) g_test_rand_int (), TRUE);
      g_free (random_string);
    }
}

static void
bench_model_named_append_run (Benchmark *bench)
{
  DeeModel  *model;
  GVariant **row_buf;
  guint      n_cols, i;
  guint      limit = 2500;

  g_assert (DEE_IS_MODEL (bench->state));

  model = DEE_MODEL (bench->state);
  n_cols = dee_model_get_n_columns (model);
  row_buf = g_new0 (GVariant*, n_cols + 1);
  
  for (i = 0; i < limit; i++)
    {
      gchar *random_string = g_strdup_printf ("%"G_GINT32_FORMAT,
                                              g_test_rand_int ());
      dee_model_build_named_row (model, row_buf,
                                 "count", (guint32) g_test_rand_int (),
                                 "string1", random_string,
                                 "string2", "Hello world",
                                 "string3", "!",
                                 "bool", TRUE,
                                 NULL);
      dee_model_append_row (model, row_buf);
      g_free (random_string);
    }
}

static void
bench_model_prepend_run (Benchmark *bench)
{
  DeeModel   *model;
  guint32     limit = 2500, i;
  
  g_assert (DEE_IS_MODEL (bench->state));
  
  model = DEE_MODEL (bench->state);
  
  for (i = 0; i < limit; i++)
    {
      gchar *random_string = g_strdup_printf ("%"G_GINT32_FORMAT,
                                              g_test_rand_int ());
      dee_model_prepend (model, random_string, "Hello world", "!",
                         (guint32)g_test_rand_int (), TRUE);
      g_free (random_string);
    }
}

static void
bench_model_sorted_run (Benchmark *bench)
{
  DeeModel   *model;
  guint32     limit = 2500, i;

  g_assert (DEE_IS_MODEL (bench->state));

  model = DEE_MODEL (bench->state);

  for (i = 0; i < limit; i++)
    {
      gchar *random_string = g_strdup_printf ("%"G_GINT32_FORMAT,
                                              g_test_rand_int ());
      dee_model_insert_sorted (model, _cmp_uint, NULL, random_string,
                               "Hello world", "!", (guint32)g_test_rand_int (),
                               TRUE);
      g_free (random_string);
    }
}

static void
bench_model_read_string_run (Benchmark *bench)
{
  DeeModel     *model;
  DeeModelIter *iter, *end;

  g_assert (DEE_IS_MODEL (bench->state));

  model = DEE_MODEL (bench->state);


  for (iter = dee_model_get_first_iter (model),
       end = dee_model_get_last_iter (model);
       iter != end; iter = dee_model_next (model, iter))
    {
      dee_model_get_string (model, iter, 0);
    }
}

static void
bench_model_read_row_run (Benchmark *bench)
{
  DeeModel     *model;
  DeeModelIter *iter, *end;
  GVariant    **row_buf;
  guint         n_cols, i;

  g_assert (DEE_IS_MODEL (bench->state));

  model = DEE_MODEL (bench->state);
  n_cols = dee_model_get_n_columns (model);
  row_buf = g_new0 (GVariant*, n_cols + 1);

  for (iter = dee_model_get_first_iter (model),
       end = dee_model_get_last_iter (model);
       iter != end; iter = dee_model_next (model, iter))
    {
      dee_model_get_row (model, iter, row_buf);
      for (i = 0; i < n_cols; i++) g_variant_unref (row_buf[i]);
    }
  
  g_free (row_buf);
}

static void
bench_model_clear_run (Benchmark *bench)
{
  DeeModel     *model;

  g_assert (DEE_IS_MODEL (bench->state));

  model = DEE_MODEL (bench->state);
  dee_model_clear (model);
  
  g_assert (dee_model_get_n_rows (model) == 0);
  
  /* Force a re-run of the setup func */
  bench->benchmark_teardown (bench);
}

static void
bench_model_walk_next_run (Benchmark *bench)
{
  DeeModel     *model;
  DeeModelIter *iter, *end;

  g_assert (DEE_IS_MODEL (bench->state));

  model = DEE_MODEL (bench->state);


  for (iter = dee_model_get_first_iter (model),
       end = dee_model_get_last_iter (model);
       iter != end; iter = dee_model_next (model, iter))
    {
    }
}

static void
bench_model_walk_pos_run (Benchmark *bench)
{
  DeeModel     *model;
  guint         n_rows, i;

  g_assert (DEE_IS_MODEL (bench->state));

  model = DEE_MODEL (bench->state);

  n_rows = dee_model_get_n_rows (model);
  for (i = 0; i < n_rows; i++)
    {
      dee_model_get_iter_at_row (model, i);
    }
}

static void
bench_index_prefix_search (Benchmark *bench)
{
  DeeIndex     *index;
  guint         i;
  gchar        *prefixes[] =
  {
   "-9", "-8", "-7", "-6", "-5", "-4", "-3", "-2", "-1",
    "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",
  };

  g_assert (DEE_IS_INDEX (bench->state));

  index = DEE_INDEX (bench->state);

  for (i = 0; i < G_N_ELEMENTS (prefixes); i++)
    {
      DeeResultSet *rs = dee_index_lookup (index, prefixes[i],
                                           DEE_TERM_MATCH_PREFIX);

      g_assert_cmpuint (dee_result_set_get_n_rows (rs), >, 0);

      g_object_unref (G_OBJECT (rs));
    }
}

static void
bench_gobject_teardown (Benchmark *bench)
{
  GObject *obj;
  
  if (bench->state)
    {
      obj = G_OBJECT (bench->state);
      g_object_unref (obj);
      bench->state = NULL;
    }
}

Benchmark seqmodel_append = { "SequenceModel.append",
                              bench_seqmodel_setup,
                              bench_model_append_run,
                              bench_gobject_teardown,
                              100,
                              NULL };

Benchmark seqmodel_named_append = { "SequenceModel.append.named",
                                    bench_seqmodel_named_setup,
                                    bench_model_named_append_run,
                                    bench_gobject_teardown,
                                    100,
                                    NULL };

Benchmark seqmodel_prepend = { "SequenceModel.prepend",
                                bench_seqmodel_setup,
                                bench_model_prepend_run,
                                bench_gobject_teardown,
                                100,
                                NULL };

Benchmark seqmodel_sorted = { "SequenceModel.sorted",
                                bench_seqmodel_setup,
                                bench_model_sorted_run,
                                bench_gobject_teardown,
                                100,
                                NULL };

Benchmark seqmodel_read_string = { "SequenceModel.read_string",
                                   bench_seqmodel_read_string_setup,
                                   bench_model_read_string_run,
                                   bench_gobject_teardown,
                                   100,
                                   NULL };

Benchmark seqmodel_read_row = { "SequenceModel.read_row",
                                 bench_seqmodel_read_string_setup,
                                 bench_model_read_row_run,
                                 bench_gobject_teardown,
                                 100,
                                 NULL };

Benchmark seqmodel_clear = { "SequenceModel.clear",
                             bench_seqmodel_read_string_setup,
                             bench_model_clear_run,
                             bench_gobject_teardown,
                             20,
                             NULL };

Benchmark seqmodel_walk_next = { "SequenceModel.walk_next",
                                 bench_seqmodel_read_string_setup,
                                 bench_model_walk_next_run,
                                 bench_gobject_teardown,
                                 1000,
                                 NULL };

Benchmark seqmodel_walk_pos = { "SequenceModel.walk_pos",
                                bench_seqmodel_read_string_setup,
                                bench_model_walk_pos_run,
                                bench_gobject_teardown,
                                1000,
                                NULL };

Benchmark filtermodel_collate = { "FilterModel.collate",
                                  bench_filter_model_collator_setup,
                                  bench_model_prepend_run,
                                  bench_gobject_teardown,
                                  100,
                                  NULL };

Benchmark filtermodel_collate_desc = { "FilterModel.collate_desc",
                                        bench_filter_model_collator_desc_setup,
                                        bench_model_prepend_run,
                                        bench_gobject_teardown,
                                        100,
                                        NULL };

Benchmark filtermodel_sort_uint = { "FilterModel.sort_uint",
                                     bench_filter_model_sort_uint_setup,
                                     bench_model_prepend_run,
                                     bench_gobject_teardown,
                                     100,
                                     NULL };

Benchmark tree_index_prefix_search = { "TreeIndex.prefix_search",
                                       bench_index_setup,
                                       bench_index_prefix_search,
                                       bench_gobject_teardown,
                                       25,
                                       NULL };

/* Arguments are interpreted as prefixes that benchmark names must match
 * in order to be run */
gint
main (gint argc, gchar *argv[])
{
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init ();
#endif
  g_test_init (&argc, &argv, NULL);
  
  /* Extract NULL terminated array of prefixes from arguments */
  int i;
  gchar **prefixes = g_new0 (gchar*, argc);
  for (i = 1; i < argc; i++)
    {
      prefixes[i-1] = argv[i];
    }

  add_benchmark (&seqmodel_append);
  add_benchmark (&seqmodel_named_append);
  add_benchmark (&seqmodel_prepend);
  add_benchmark (&seqmodel_sorted);
  add_benchmark (&seqmodel_read_string);
  add_benchmark (&seqmodel_read_row);
  add_benchmark (&seqmodel_clear);
  add_benchmark (&seqmodel_walk_next);
  add_benchmark (&seqmodel_walk_pos);
  add_benchmark (&filtermodel_collate);
  add_benchmark (&filtermodel_collate_desc);
  add_benchmark (&filtermodel_sort_uint);
  add_benchmark (&tree_index_prefix_search);

  run_benchmarks (argc > 1 ? prefixes : NULL);
  
  g_free (prefixes);
  
  return 0;
}
