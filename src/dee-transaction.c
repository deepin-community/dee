/*
 * Copyright (C) 2011 Canonical, Ltd.
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
 * Authored by:
 *               Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 */

/**
 * SECTION:dee-transaction
 * @short_description: A self contained change set for a #DeeModel
 * @include: dee.h
 *
 * #DeeTransaction is a self contained change set related to some particular
 * #DeeModel called the <emphasis>target model</emphasis>.
 *
 * The transaction instance itself implements the #DeeModel interface in a way
 * that overlays the target model. In database terms the target model has
 * isolation level READ_COMMITTED. Meaning that the target model is not modified
 * until you call dee_transaction_commit().
 *
 * To flush the changes to the target model call dee_transaction_commit().
 * After committing the transaction will become invalid and must be freed with
 * g_object_unref(). It is a programming error to try and access a transaction
 * that has been committed with the sole exception of calling
 * dee_transaction_is_committed().
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <memory.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "dee-model.h"
#include "dee-transaction.h"
#include "dee-serializable-model.h"
#include "dee-marshal.h"
#include "trace-log.h"

/*
 * IMPLEMENTATION NOTES
 *
 * The idea is to keep the transaction as light as possible by reusing the
 * iters from the target model, letting them fall through to the target when
 * unchanged in the transaction.
 *
 * This gives some overhead when reading stuff from a transaction model, but
 * that is considered an acceptable tradeoff as the transaction is optimized
 * for writing and not reading.
 *
 * The transaction keeps a "journal" of the changes it needs to apply and
 * commit. The relevant data structure is the JournalIter. Journal iters
 * are characterized in 2 ways. JournalIters are called 'jiters' for short.
 *
 * Firstly there is the ChangeType of a jiter.
 *
 * CHANGE_TYPE_REMOVE: The jiter is an 'override' for an iter in the
 *                     target model that has been deleted. The jiter.overlay
 *                     member points to the original
 *
 * CHANGE_TYPE_CHANGE: The jiter is an 'override' for an iter in the
 *                     target model that has been changed. The jiter.overlay
 *                     points to the original. The jiter.row_data member will
 *                     contain all the values for the changed row
 *
 * CHANGE_TYPE_ADD: The jiter does not correspond to an iter in the target, and
 *                  the jiter.override member will be unset. Additions are
 *                  grouped together in "segments" and these segments points
 *                  to an iter in the target they attach *before*
 *
 * To ease internal book keeping we also have the IterType enumeration which
 * describes if a given pointer is a jiter or an iter.
 *
 * Segments
 * Jiters that are additions are grouped into "segments" and each segment
 * points to a row in the target model.
 *
 */

typedef struct _JournalSegment JournalSegment;
typedef struct _JournalIter JournalIter;

typedef enum {
  CHANGE_TYPE_REMOVE,
  CHANGE_TYPE_CHANGE,
  CHANGE_TYPE_ADD
} ChangeType;

typedef enum {
  ITER_TYPE_TARGET,
  ITER_TYPE_JOURNAL
} IterType;

struct _JournalSegment {
  /* End points of the journal iters in the segment */
  JournalIter    *first_iter;
  JournalIter    *last_iter;

  /* The row in the target model that this segment is attached before.
   * Note that the target row may be changed or deleted */
  DeeModelIter   *target_iter;

  /* The transaction owning the segment. It is mainly here as an optimization
   * to allow fast access to the number of column sin the model for copying
   * and freeing the row_data */
  DeeTransaction *txn;

  /* Used on commit() because we commit whole segments at a time,
   * disregarding playback order */
  gboolean        is_committed;
};

/* Implements a two dimensional doubly linked list. One dimension is the
 * playback queue and the other is the order of the iters (inside a segment).
 * INVARIANT: Set if and only if change_type == CHANGE_TYPE_ADD */
struct _JournalIter {
  /* Added rows all belong to a specific segment
   * attached before a row in the target */
  JournalSegment  *segment;

  /* Linked list for the playback queue */
  JournalIter     *next_playback;
  JournalIter     *prev_playback;

  /* Linked list for the itersinside a segment */
  JournalIter     *next_iter;
  JournalIter     *prev_iter;

  /* Points to a row in the targetmodel
   * INVARIANT: Set if and only if change_type == CHANGE_TYPE_{CHANGE,REMOVE} */
  DeeModelIter    *override_iter;

  /* FIXME: Not implemented. I am not even sure it's theoretically possible */
  GSList          *tags;

  ChangeType       change_type;
  GVariant       **row_data;
};

/**
 * DeeTransactionPrivate:
 *
 * Ignore this structure.
 */
struct _DeeTransactionPrivate
{
  /* The model the transaction applies against */
  DeeModel  *target;

  /* The journal maps DeeModelIters from the target model to JournalIters.
   * Also maps JournalIters to them selves.
   * INAVARIANT: If an iter is not in the journal it
   *             it is an untouched iter from the target model
   * NOTE: Different keys may point to the same jiter
   */
  GHashTable *journal;

  /* The segments map DeeModelIters from the target model,
   * to JournalSegments that lie immediately before them.
   * INVARIANT: A JournalIter that has a segment has change_type CHANGE_TYPE_ADD
   * NOTE: different keys may correspond to the same segment
   */
  GHashTable *segments;

  /* The head and the tail of the queue of JournalIters constituting
   * the changes we must play back on the target model.
   * NOTE: jiters that become irrelevant must be unlinked from the
   *       playback queue and freed. We use the playback queue on finalize
   *       to walk and free the jiters and segments */
  JournalIter *first_playback;
  JournalIter *last_playback;

  /* Signals handlers to check for the concurrent modification of the back end */
  gulong     target_row_added_handler;
  gulong     target_row_removed_handler;
  gulong     target_row_changed_handler;

  /* canary to saniy check that no one sneaks changes into the target while
   * the txn is open */
  guint64    begin_seqnum;

  /* DeeTransactionError code. If != 0 we have an error state set */
  guint      error_code;

  /* Number of columns in target model (just a cache) */
  guint n_cols;
};

static JournalIter*
journal_iter_new (ChangeType ct)
{
  JournalIter *jent;

  jent = g_slice_new0 (JournalIter);
  jent->change_type = ct;

  return jent;
}

static void
journal_iter_free (JournalIter *jiter)
{
  GVariant **v;

  if (jiter->row_data)
    {
      for (v = jiter->row_data; *v != NULL; v++)
        {
          g_variant_unref (*v);
          *v = NULL;
        }
      g_free (jiter->row_data);
      jiter->row_data = NULL;
    }

  // FIXME: free tags, when/if we implement tags

  g_slice_free (JournalIter, jiter);
}

#define journal_iter_is_removed(jiter) (jiter->change_type == CHANGE_TYPE_REMOVE)

static JournalSegment*
journal_segment_new_before (DeeModelIter *iter, DeeTransaction *txn)
{
  JournalSegment *jseg = g_slice_new0 (JournalSegment);
  jseg->target_iter = iter;
  jseg->txn = txn;
  jseg->is_committed = FALSE;
  return jseg;
}

static void
journal_segment_free (JournalSegment *segment)
{
  g_slice_free (JournalSegment, segment);
}

static GVariant**
copy_row_data (GVariant **row_data, guint n_cols)
{
  GVariant **iter, **copy;
  guint      i;

  for (iter = row_data, i = 0; i < n_cols; iter++, i++)
    {
      g_variant_ref_sink (*iter);
    }

  copy = g_new (GVariant*, n_cols + 1);
  memcpy (copy, row_data, n_cols * sizeof (GVariant*));
  copy[n_cols] = NULL;
  return copy;
}

static JournalIter*
journal_segment_append (JournalSegment *jseg, GVariant **row_data)
{
  JournalIter *new_jiter;

  g_assert ((jseg->last_iter == NULL && jseg->first_iter == NULL) ||
            jseg->last_iter->next_iter == NULL);

  new_jiter = journal_iter_new (CHANGE_TYPE_ADD);
  new_jiter->segment = jseg;
  new_jiter->row_data = copy_row_data (row_data, jseg->txn->priv->n_cols);

  if (jseg->last_iter == NULL)
    {
      jseg->last_iter = new_jiter;
      jseg->first_iter = new_jiter;
    }
  else
    {
      new_jiter->prev_iter = jseg->last_iter;
      jseg->last_iter->next_iter = new_jiter;
      jseg->last_iter = new_jiter;
    }

  return new_jiter;
}

static JournalIter*
journal_segment_prepend (JournalSegment *jseg, GVariant **row_data)
{
  JournalIter *new_jiter;

  g_assert ((jseg->last_iter == NULL && jseg->first_iter == NULL) ||
              jseg->first_iter->prev_iter == NULL);

  new_jiter = journal_iter_new (CHANGE_TYPE_ADD);
  new_jiter->segment = jseg;
  new_jiter->row_data = copy_row_data (row_data, jseg->txn->priv->n_cols);

  if (jseg->first_iter == NULL)
    {
      jseg->last_iter = new_jiter;
      jseg->first_iter = new_jiter;
    }
  else
    {
      new_jiter->next_iter = jseg->first_iter;
      jseg->first_iter->prev_iter = new_jiter;
      jseg->first_iter = new_jiter;
    }

  return new_jiter;
}

static JournalIter*
journal_segment_insert_before (JournalSegment  *jseg,
                               JournalIter     *jiter,
                               GVariant       **row_data)
{
  JournalIter *new_jiter;

  g_assert ((jseg->first_iter == NULL && jseg->last_iter == NULL) ||
            (jseg->first_iter != NULL && jseg->last_iter != NULL));

  if (jiter == NULL)
    {
      return journal_segment_append (jseg, row_data);
    }
  else if (jiter == jseg->first_iter)
    {
      return journal_segment_prepend (jseg, row_data);
    }

  /* It's not a pre- or append(), but a genuine insertion */
  new_jiter = journal_iter_new (CHANGE_TYPE_ADD);
  new_jiter->segment = jseg;
  new_jiter->row_data = copy_row_data (row_data, jseg->txn->priv->n_cols);

  if (jseg->first_iter == NULL)
    {
      jseg->first_iter = new_jiter;
      jseg->last_iter = new_jiter;
    }
  else
    {
      jiter->prev_iter->next_iter = new_jiter;
      new_jiter->prev_iter = jiter->prev_iter;
      new_jiter->next_iter = jiter;
      jiter->prev_iter = new_jiter;
    }

  return new_jiter;
}

#define AS_TXN(ptr) ((DeeTransaction*)ptr)

#define get_journal_segment_before(iter) \
  g_hash_table_lookup (priv->segments, iter)

#define set_journal_segment_before(iter,jseg) \
  g_hash_table_insert (priv->segments, iter, jseg)

#define register_journal_iter(jiter) \
  g_hash_table_insert (priv->journal, jiter, jiter); \
  if (jiter->override_iter) g_hash_table_insert (priv->journal, jiter->override_iter, jiter);

#define unregister_journal_iter(jiter) \
    g_hash_table_remove (priv->journal, jiter);

#define check_journal_iter(iter,jiter_p) \
    g_hash_table_lookup_extended (priv->journal, iter, NULL, (void**)jiter_p)

#define JOURNAL_ITER(iter) \
  ((JournalIter*)iter)

#define MODEL_ITER(jiter) \
  ((DeeModelIter*)jiter)

#define append_to_playback(jiter) \
    if (priv->first_playback == NULL) \
      priv->first_playback = jiter; \
    \
    if (priv->last_playback) \
      { \
        priv->last_playback->next_playback = jiter; \
        jiter->prev_playback = priv->last_playback; \
      } \
    \
    priv->last_playback = jiter;

static void dee_transaction_model_iface_init (DeeModelIface *iface);

G_DEFINE_TYPE_WITH_CODE (DeeTransaction,
                         dee_transaction,
                         DEE_TYPE_SERIALIZABLE_MODEL,
                         G_IMPLEMENT_INTERFACE (DEE_TYPE_MODEL,
                                                dee_transaction_model_iface_init));

#define DEE_TRANSACTION_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE(obj, DEE_TYPE_TRANSACTION, DeeTransactionPrivate))

enum
{
  PROP_0,
  PROP_TARGET
};

/* INVARIANT: The end iter of the transaction is the same as the end iter
 *            of the target model */

#define DEE_TRANSACTION_TARGET(txn) (DEE_TRANSACTION(txn)->priv->target)

/*
 * DeeModel forward declarations
 */

static DeeModelIter*  dee_transaction_insert_row_before (DeeModel      *self,
                                                         DeeModelIter  *iter,
                                                         GVariant     **row_members);

static void           dee_transaction_remove         (DeeModel     *self,
                                                      DeeModelIter *iter);

static void           dee_transaction_set_value      (DeeModel       *self,
                                                      DeeModelIter   *iter,
                                                      guint           column,
                                                      GVariant       *value);

static void           dee_transaction_set_row     (DeeModel       *self,
                                                   DeeModelIter   *iter,
                                                   GVariant      **row_members);

static GVariant*      dee_transaction_get_value      (DeeModel     *self,
                                                      DeeModelIter *iter,
                                                      guint          column);

static DeeModelIter* dee_transaction_get_first_iter  (DeeModel     *self);

static DeeModelIter* dee_transaction_get_last_iter   (DeeModel     *self);

static DeeModelIter* dee_transaction_next            (DeeModel     *self,
                                                      DeeModelIter *iter);

static DeeModelIter* dee_transaction_prev            (DeeModel     *self,
                                                      DeeModelIter *iter);

static DeeModelTag*   dee_transaction_register_tag   (DeeModel       *self,
                                                      GDestroyNotify  tag_destroy);

static gpointer       dee_transaction_get_tag        (DeeModel       *self,
                                                      DeeModelIter   *iter,
                                                      DeeModelTag    *tag);

static void           dee_transaction_set_tag        (DeeModel       *self,
                                                      DeeModelIter   *iter,
                                                      DeeModelTag    *tag,
                                                      gpointer        value);

static void           dee_transaction_set_tag        (DeeModel       *self,
                                                      DeeModelIter   *iter,
                                                      DeeModelTag    *tag,
                                                      gpointer        value);

static const gchar*   dee_transaction_get_field_schema (DeeModel    *self,
                                                        const gchar *field_name,
                                                        guint       *out_column);

static void           on_target_modified             (DeeTransaction *self,
                                                      DeeModelIter  *iter);

/* GObject Init */
static void
dee_transaction_finalize (GObject *object)
{
  DeeTransactionPrivate *priv = DEE_TRANSACTION (object)->priv;
  
  if (priv->target)
    {
      g_signal_handler_disconnect (priv->target, priv->target_row_added_handler);
      g_signal_handler_disconnect (priv->target, priv->target_row_removed_handler);
      g_signal_handler_disconnect (priv->target, priv->target_row_changed_handler);

      g_object_unref (priv->target);
    }
  
  if (priv->journal)
    {
      g_hash_table_unref (priv->journal);
      priv->journal = NULL;
    }

  if (priv->segments)
    {
      g_hash_table_unref (priv->segments);
      priv->segments = NULL;
    }

  if (priv->first_playback)
    {
      JournalIter *jiter, *free_jiter;
      GHashTable  *freed_segments = g_hash_table_new (g_direct_hash,
                                                      g_direct_equal);
      for (jiter = priv->first_playback; jiter != NULL; )
        {
          if (jiter->segment &&
              g_hash_table_lookup (freed_segments, jiter->segment))
            {
              g_hash_table_insert (freed_segments,jiter->segment, jiter->segment);
              journal_segment_free (jiter->segment);
            }

          free_jiter = jiter;
          jiter = jiter->next_playback;
          journal_iter_free (free_jiter);
        }
      priv->first_playback = NULL;
      priv->last_playback = NULL;
      g_hash_table_destroy (freed_segments);
    }

  G_OBJECT_CLASS (dee_transaction_parent_class)->finalize (object);
}

/* GObject Post-Init. Properties have been set */
static void
dee_transaction_constructed (GObject *object)
{
  const gchar* const  *schema;
  const gchar        **column_names;
  guint                n_columns;
  DeeTransactionPrivate *priv = DEE_TRANSACTION (object)->priv;
  
  /* Chain up before we do too much, to make sure the parent class is ready */
  if (G_OBJECT_CLASS (dee_transaction_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (dee_transaction_parent_class)->constructed (object);
  
  if (priv->target == NULL)
  {
    g_critical ("You must set the 'target' property of "
                "the DeeTransaction upon creation.");
    return;
  }

  /* Adopt schema of target model */
  schema = dee_model_get_schema (priv->target, &n_columns);
  dee_model_set_schema_full (DEE_MODEL (object), schema, n_columns);
  priv->n_cols = n_columns;

  /* Also adopt column names of target model */
  column_names = dee_model_get_column_names (priv->target, &n_columns);
  if (column_names)
    {
      dee_model_set_column_names_full (DEE_MODEL (object),
                                       column_names, n_columns);
    }

  /* Adopt seqnums of target model, if it has any */
  if (DEE_IS_SERIALIZABLE_MODEL (priv->target))
    {
      priv->begin_seqnum = dee_serializable_model_get_seqnum (priv->target);
    }
  else
    {
      priv->begin_seqnum = 0;
    }
  dee_serializable_model_set_seqnum (DEE_MODEL (object), priv->begin_seqnum);

  priv->target_row_added_handler =
      g_signal_connect_swapped (priv->target, "row-added",
                                G_CALLBACK (on_target_modified), object);
  priv->target_row_removed_handler =
      g_signal_connect_swapped (priv->target, "row-removed",
                                G_CALLBACK (on_target_modified), object);
  priv->target_row_changed_handler =
      g_signal_connect_swapped (priv->target, "row-changed",
                                G_CALLBACK (on_target_modified), object);
}

static void
dee_transaction_set_property (GObject       *object,
                              guint          id,
                              const GValue  *value,
                              GParamSpec    *pspec)
{
  DeeTransactionPrivate *priv = DEE_TRANSACTION (object)->priv;

  switch (id)
    {
    case PROP_TARGET:
      priv->target = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
      break;
    }
}

static void
dee_transaction_get_property (GObject     *object,
                              guint        id,
                              GValue      *value,
                              GParamSpec  *pspec)
{
  switch (id)
    {
    case PROP_TARGET:
      g_value_set_object (value, DEE_TRANSACTION (object)->priv->target);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
      break;
    }
}

static void
dee_transaction_class_init (DeeTransactionClass *klass)
{
  GObjectClass           *obj_class = G_OBJECT_CLASS (klass);
  GParamSpec             *pspec;

  obj_class->finalize     = dee_transaction_finalize;
  obj_class->constructed  = dee_transaction_constructed;
  obj_class->set_property = dee_transaction_set_property;
  obj_class->get_property = dee_transaction_get_property;

  /**
   * DeeTransaction:back-end:
   *
   * The backend model used by this proxy model.
   **/
  pspec = g_param_spec_object ("target", "Target",
                               "Target model",
                               DEE_TYPE_MODEL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
                               | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (obj_class, PROP_TARGET, pspec);
  
  /* Add private data */
  g_type_class_add_private (obj_class, sizeof (DeeTransactionPrivate));
}

static void
dee_transaction_model_iface_init (DeeModelIface *iface)
{
  DeeModelIface *piface = g_type_interface_peek_parent (iface);

  iface->set_schema_full      = piface->set_schema_full;
  iface->get_schema           = piface->get_schema;
  iface->get_field_schema     = dee_transaction_get_field_schema;
  iface->get_column_schema    = piface->get_column_schema;
  iface->get_n_columns        = piface->get_n_columns;
  iface->get_n_rows           = piface->get_n_rows;
  iface->clear                = piface->clear;
  iface->prepend_row          = piface->prepend_row;
  iface->append_row           = piface->append_row;
  iface->insert_row           = piface->insert_row;
  iface->insert_row_before    = dee_transaction_insert_row_before;
  iface->remove               = dee_transaction_remove;
  iface->set_value            = dee_transaction_set_value;
  iface->set_row              = dee_transaction_set_row;
  iface->get_value            = dee_transaction_get_value;
  iface->get_first_iter       = dee_transaction_get_first_iter;
  iface->get_last_iter        = dee_transaction_get_last_iter;
  iface->get_iter_at_row      = piface->get_iter_at_row;
  iface->get_bool             = piface->get_bool;
  iface->get_uchar            = piface->get_uchar;
  iface->get_int32            = piface->get_int32;
  iface->get_uint32           = piface->get_uint32;
  iface->get_int64            = piface->get_int64;
  iface->get_uint64           = piface->get_uint64;
  iface->get_double           = piface->get_double;
  iface->get_string           = piface->get_string;
  iface->next                 = dee_transaction_next;
  iface->prev                 = dee_transaction_prev;
  iface->is_first             = piface->is_first;
  iface->is_last              = piface->is_last;
  iface->get_position         = piface->get_position;
  iface->register_tag         = dee_transaction_register_tag;
  iface->get_tag              = dee_transaction_get_tag;
  iface->set_tag              = dee_transaction_set_tag;
}

static void
dee_transaction_init (DeeTransaction *model)
{
  DeeTransactionPrivate *priv;

  priv = model->priv = DEE_TRANSACTION_GET_PRIVATE (model);
  priv->target = NULL;
  
  priv->journal = g_hash_table_new (g_direct_hash, g_direct_equal);
  priv->segments = g_hash_table_new (g_direct_hash, g_direct_equal);

  priv->target_row_added_handler = 0;
  priv->target_row_removed_handler = 0;
  priv->target_row_changed_handler = 0;
}

/*
 * DeeModel Interface Implementation
 */

/* Inherited methods */
// dee_transaction_set_schema_full ()
// dee_transaction_get_schema ()
// dee_transaction_get_column_schema ()
// dee_transaction_get_n_columns ()
// dee_transaction_get_n_rows () (slow O(n))
// dee_transaction_clear () (slow O(n))
// dee_transaction_append_row ()
// dee_transaction_prepend_row ()
// dee_transaction_insert_row ()

static const gchar*
dee_transaction_get_field_schema (DeeModel    *self,
                                  const gchar *field_name,
                                  guint       *out_column)
{
  DeeModelIface *piface;
  const gchar   *schema;

  schema = dee_model_get_field_schema (DEE_TRANSACTION_TARGET (self),
                                       field_name, out_column);
  if (schema) return schema;

  // maybe the names are registered just for the transaction instance?
  piface = g_type_interface_peek_parent (DEE_MODEL_GET_IFACE (self));
  return (* piface->get_field_schema) (self, field_name, out_column);
}

static DeeModelIter*
dee_transaction_insert_row_before (DeeModel      *self,
                                   DeeModelIter  *iter,
                                   GVariant     **row_members)
{
  DeeTransactionPrivate *priv;
  JournalIter           *new_jiter, *jiter;
  JournalSegment        *jseg;

  g_return_val_if_fail (DEE_IS_TRANSACTION (self), NULL);
  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (row_members != NULL, NULL);
  g_return_val_if_fail (!dee_transaction_is_committed (AS_TXN (self)), NULL);

  priv = DEE_TRANSACTION (self)->priv;

  /* new_jiter is what we'll eventually return */
  new_jiter = NULL;

  if (check_journal_iter (iter, &jiter))
    {
      /* If the jiter has a segment it must be an added row and we wire
       * into that segment */
      if (jiter->segment)
        {
          g_assert (jiter->change_type == CHANGE_TYPE_ADD);
          new_jiter = journal_segment_insert_before (jiter->segment,
                                                     jiter,
                                                     row_members);
        }
      else
        {
          g_assert (jiter->change_type == CHANGE_TYPE_CHANGE ||
                    jiter->change_type == CHANGE_TYPE_REMOVE);

          /* Inserting relative to a removed row is a consumer error, but
           * note we still want the assertion above to verify our own
           * invariants */
          if (G_UNLIKELY (jiter->change_type == CHANGE_TYPE_REMOVE))
            {
              g_critical ("Inserting new row relative to previously removed row");
              return iter;
            }

          /* Note that on commit time we might have removed the iter we attach
           * before. We handle that at that point by scanning forwards until we
           * find a valid iter */

          if ((jseg = get_journal_segment_before (iter)) != NULL)
            {
              new_jiter = journal_segment_append (jseg, row_members);
            }
          else
            {
              jseg = journal_segment_new_before (iter, DEE_TRANSACTION (self));
              new_jiter = journal_segment_append (jseg, row_members);
              set_journal_segment_before (iter, jseg);
            }
        }
    }
  else if ((jseg = get_journal_segment_before (iter)) != NULL)
    {
      /* This is an untouched row from the target model with a segment
       * attached before it */
      new_jiter = journal_segment_append (jseg, row_members);
    }
  else
    {
      /* This is an untouched row from the target model without any new rows
       * attached before it.
       * Set up a new segment before it and attach to that */
      jseg = journal_segment_new_before (iter, DEE_TRANSACTION (self));
      new_jiter = journal_segment_append (jseg, row_members);
      set_journal_segment_before (iter, jseg);
    }

  /* We have b0rked up this function if new_jiter is not set by now */
  g_assert (new_jiter != NULL);

  append_to_playback (new_jiter);
  register_journal_iter (new_jiter);

  dee_serializable_model_inc_seqnum (self);
  g_signal_emit_by_name (self, "row-added", new_jiter);

  return MODEL_ITER (new_jiter);
}

static void
dee_transaction_remove (DeeModel     *self,
                        DeeModelIter *iter)
{
  DeeTransactionPrivate *priv;
  JournalIter           *jiter;
  gboolean               should_free_jiter;

  g_return_if_fail (DEE_IS_TRANSACTION (self));
  g_return_if_fail (!dee_transaction_is_committed (AS_TXN (self)));

  priv = DEE_TRANSACTION (self)->priv;
  should_free_jiter = FALSE;

  if (check_journal_iter (iter, &jiter))
    {
      if (G_UNLIKELY (jiter->change_type == CHANGE_TYPE_REMOVE))
        {
          g_critical ("Row %p already removed from transaction", iter);
          return;
        }

      /* If jiter is something we've added we can just unlink it from the
       * playback queue and its segment and free it. If it's a change we
       * can simply mark it as a removal in stead.
       * Note that if a segment is attached to a removed row, we resolve
       * that at commit() time by scanning forwards to the next non-removed
       * row. */
      if (jiter->change_type == CHANGE_TYPE_CHANGE)
        {
          jiter->change_type = CHANGE_TYPE_REMOVE;
        }
      else
        {
          g_assert (jiter->change_type == CHANGE_TYPE_ADD);
          should_free_jiter = TRUE;
        }
    }
  else
    {
      jiter = journal_iter_new (CHANGE_TYPE_REMOVE);
      jiter->override_iter = iter;
      register_journal_iter (jiter);
      append_to_playback (jiter);
    }

  /* Emit the removed signal while the iter is still valid,
   * but after we increased the seqnum */
  dee_serializable_model_inc_seqnum (self);
  g_signal_emit_by_name (self, "row-removed",
                         jiter->override_iter ? jiter->override_iter : MODEL_ITER (jiter));

  if (should_free_jiter)
    {
      /* Detach from segment */
      if (jiter->segment->first_iter == jiter)
        jiter->segment->first_iter = jiter->next_iter;

      if (jiter->segment->last_iter == jiter)
        jiter->segment->last_iter = jiter->prev_iter;

      /* If our host segment is empty we must get rid of it,
       * else we unlink our selves from the internal list */
      if (jiter->segment->first_iter == NULL)
        {
          g_assert (jiter->segment->last_iter == NULL);
          g_hash_table_remove (priv->segments, jiter->segment->target_iter);
        }
      else{

          if (jiter->prev_iter)
            jiter->prev_iter->next_iter = jiter->next_iter;

          if (jiter->next_iter)
            jiter->next_iter->prev_iter = jiter->prev_iter;
      }

      /* Remove from playback queue */
      if (jiter->prev_playback)
        jiter->prev_playback->next_playback = jiter->next_playback;

      if (jiter->next_playback)
        jiter->next_playback->prev_playback = jiter->prev_playback;

      unregister_journal_iter (jiter);
    }
}

static void
dee_transaction_set_row (DeeModel       *self,
                         DeeModelIter   *iter,
                         GVariant      **row_members)
{
  DeeTransactionPrivate *priv;
  JournalIter           *jiter;
  GVariant              **v;

  g_return_if_fail (DEE_IS_TRANSACTION (self));
  g_return_if_fail (!dee_transaction_is_committed (AS_TXN (self)));

  priv = DEE_TRANSACTION (self)->priv;

  if (check_journal_iter (iter, &jiter))
    {
      if (G_UNLIKELY (jiter->change_type == CHANGE_TYPE_REMOVE))
        {
          g_critical ("Trying to update row which have been removed from "
                      "the transaction");
          return;
        }

      g_assert (jiter->row_data != NULL);
      for (v = jiter->row_data; *v != NULL; v++)
        {
          g_variant_unref (*v);
        }
      g_free (jiter->row_data);

      jiter->row_data = copy_row_data (row_members, priv->n_cols);
    }
  else
    {
      /* A simple check to raise the probability of iter being a valid
       * iter in the target */
      if (strcmp (g_variant_get_type_string (row_members[0]),
                  g_variant_get_type_string (dee_model_get_value (priv->target,
                                                                  iter, 0)))
                  != 0)
        {
          g_critical ("Error setting row in transaction %p. The iter is "
                      "probably not in the target model", self);
          return;
        }

      jiter = journal_iter_new (CHANGE_TYPE_CHANGE);
      jiter->row_data = copy_row_data (row_members, priv->n_cols);
      jiter->override_iter = iter;
      register_journal_iter (jiter);
      append_to_playback (jiter);
    }

  g_assert (jiter != NULL);
  g_assert (   (jiter->override_iter != NULL && jiter->change_type == CHANGE_TYPE_CHANGE)
            || (jiter->override_iter == NULL && jiter->change_type == CHANGE_TYPE_ADD));

  dee_serializable_model_inc_seqnum (self);
  g_signal_emit_by_name (self, "row-changed",
                         jiter->override_iter ? jiter->override_iter : MODEL_ITER (jiter));

}

static void
dee_transaction_set_value (DeeModel      *self,
                           DeeModelIter  *iter,
                           guint          column,
                           GVariant      *value)
{
  DeeTransactionPrivate *priv;
  JournalIter           *jiter;

  g_return_if_fail (DEE_IS_TRANSACTION (self));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (value != NULL);
  g_return_if_fail (!dee_transaction_is_committed (AS_TXN (self)));

  priv = DEE_TRANSACTION (self)->priv;

  g_return_if_fail (column < priv->n_cols);

  if (check_journal_iter (iter, &jiter))
    {
      if (G_UNLIKELY (jiter->change_type == CHANGE_TYPE_REMOVE))
        {
          g_critical ("Trying to change value of removed row");
          return;
        }

      g_variant_unref (jiter->row_data[column]);
      jiter->row_data[column] = g_variant_ref_sink (value);
    }
  else
    {
      /* We haven't touched this row before, which guarantees that the iter
       * must point to a row in the target model */
      jiter = journal_iter_new (CHANGE_TYPE_CHANGE);
      jiter->override_iter = iter;

      /* Assume row data */
      jiter->row_data = dee_model_get_row (priv->target, iter, NULL);
      g_variant_unref (jiter->row_data[column]);
      jiter->row_data[column] = g_variant_ref_sink (value);

      register_journal_iter (jiter);
      append_to_playback (jiter);
    }

  g_assert (jiter != NULL);
  g_assert (   (jiter->override_iter != NULL && jiter->change_type == CHANGE_TYPE_CHANGE)
            || (jiter->override_iter == NULL && jiter->change_type == CHANGE_TYPE_ADD));

  dee_serializable_model_inc_seqnum (self);
  g_signal_emit_by_name (self, "row-changed",
                         jiter->override_iter ? jiter->override_iter : MODEL_ITER (jiter));
}

static GVariant*
dee_transaction_get_value (DeeModel     *self,
                           DeeModelIter *iter,
                           guint         column)
{
  DeeTransactionPrivate *priv;
  JournalIter          *jiter;

  g_return_val_if_fail (DEE_IS_TRANSACTION (self), NULL);
  g_return_val_if_fail (!dee_transaction_is_committed (AS_TXN (self)), NULL);

  priv = DEE_TRANSACTION (self)->priv;

  if(check_journal_iter (iter, &jiter))
    {
      if (G_UNLIKELY (jiter->change_type == CHANGE_TYPE_REMOVE))
        {
          g_critical ("Trying to get value from a row that has been removed "
                      "from the transaction");
          return NULL;
        }

      g_return_val_if_fail (column < priv->n_cols, NULL);
      return g_variant_ref (jiter->row_data[column]);
    }
  else
    {
      return dee_model_get_value (priv->target, iter, column);
    }
}

/* Inherited methods */
// dee_transaction_get_bool ()

// dee_transaction_get_uchar ()

// dee_transaction_get_int32 ()

// dee_transaction_get_uint32 ()

// dee_transaction_get_int64 ()

// dee_transaction_get_uint64 ()

// dee_transaction_get_double ()

// dee_transaction_get_string ()

/* This method returns the next logical iter, which may be a removal */
static DeeModelIter*
dee_transaction_next_raw (DeeModel       *self,
                          DeeModelIter   *iter,
                          IterType       *out_iter_type)
{
  DeeTransactionPrivate *priv;
  JournalIter           *jiter, *jiter_next;
  JournalSegment        *jseg;
  DeeModelIter          *end;

  g_return_val_if_fail (DEE_IS_TRANSACTION (self), NULL);

  priv = DEE_TRANSACTION (self)->priv;
  end = dee_model_get_last_iter (self); /* end shared with target */

  g_return_val_if_fail (iter != end, (*out_iter_type = ITER_TYPE_TARGET, end));

  if (check_journal_iter(iter, &jiter))
    {

      *out_iter_type = ITER_TYPE_JOURNAL;

      /* If jiter points to a next jiter we're somewhere inside a segment and
       * that's well and good.
       *
       * Otherwise we reached the end of the segment and we return
       * the iter the segment was attached before (which may have
       * an override jiter, mind you). Or we wheren't in a segment to
       * begin with */
      if (jiter->next_iter)
        {
          return MODEL_ITER (jiter->next_iter);
        }
      else if (jiter->segment)
        {
          if (check_journal_iter (jiter->segment->target_iter, &jiter_next))
            {
              return MODEL_ITER (jiter_next);
            }
          else
            {
              *out_iter_type = ITER_TYPE_TARGET;
              return jiter->segment->target_iter;
            }
        }
      else
        {
          /* This was an overlay jiter, either CHANGE or REMOVE.
           * We use the structure of the target model to step.
           * If there's a segment before the next iter in the target model,
           * step into the segment. Otherwise just step normally on the target
           * */
          g_assert (jiter->override_iter != NULL);
          iter = dee_model_next (priv->target, jiter->override_iter);
          jseg = get_journal_segment_before (iter);
          if (jseg != NULL)
            {
              return MODEL_ITER (jseg->first_iter);
            }
          else
            {
              if (check_journal_iter (iter, &jiter_next))
                {
                  return MODEL_ITER (jiter_next);
                }
              else
                {
                  *out_iter_type = ITER_TYPE_TARGET;
                  return iter;
                }
            }
        }

      g_assert_not_reached ();
    }

  /* If we get here, then this was not a journal iter,
   * but an unmodifed row from the target model */

  /* We share end iter with the target model, so it's an error to hit that */
  if (iter == end)
    {
      g_critical ("Trying to step past end of transaction model");
      return iter;
    }

  /* If there's a segment before the next iter in the target model,
   * step into the segment. Otherwise just step normally on the target */
  iter = dee_model_next (priv->target, iter);
  jseg = get_journal_segment_before (iter);
  if (jseg != NULL)
    {
      *out_iter_type = ITER_TYPE_JOURNAL;
      return MODEL_ITER (jseg->first_iter);
    }
  else
    {
      *out_iter_type = ITER_TYPE_TARGET;
      return iter;
    }
}

static DeeModelIter*
dee_transaction_get_first_iter (DeeModel     *self)
{
  DeeTransactionPrivate *priv;
  JournalSegment        *jseg;
  JournalIter           *jiter;
  DeeModelIter          *iter;
  IterType               itype;

  g_return_val_if_fail (DEE_IS_TRANSACTION (self), NULL);
  g_return_val_if_fail (!dee_transaction_is_committed (AS_TXN (self)), NULL);

  priv = DEE_TRANSACTION (self)->priv;

  iter = dee_model_get_first_iter (priv->target);
  
  if ((jseg = get_journal_segment_before (iter)) != NULL)
    {
      g_assert (jseg->first_iter != NULL);
      itype = ITER_TYPE_JOURNAL;
      jiter = jseg->first_iter;
      iter = MODEL_ITER (jseg->first_iter);
    }
  else if (check_journal_iter (iter, &jiter))
    {
      /* Since iter is from the target, then jiter must
       * be an overlay and not in a segment (ie. not an addition) */
      g_assert (jiter->segment == NULL);
      g_assert (jiter->override_iter == iter);
      itype = ITER_TYPE_JOURNAL;
      iter = MODEL_ITER (jiter);
    }
  else
    {
      /* The first target iter is untouched */
      itype = ITER_TYPE_TARGET;
    }

  /* Now scan forwards until we have something which is not deleted.
   * This is complicated by the fact that we may have attached segments
   * before iters that have been marked removed */
  while (itype == ITER_TYPE_JOURNAL && journal_iter_is_removed (jiter))
    {
      iter = dee_transaction_next_raw (self, iter, &itype);

      if (itype == ITER_TYPE_JOURNAL)
        {
          jiter = JOURNAL_ITER (iter);
        }

      if ((jseg = get_journal_segment_before (iter)) != NULL)
        {
          return MODEL_ITER (jseg->first_iter);
        }
    }

  /* Finally - for override iters (changes, this shouldn't be a removal),
   * we return the original iter from the target model */
  if (itype == ITER_TYPE_JOURNAL && jiter->override_iter != NULL)
    {
      g_assert (!journal_iter_is_removed (jiter));
      iter = jiter->override_iter;
      itype = ITER_TYPE_TARGET;
    }

  return iter;
}

static DeeModelIter*
dee_transaction_get_last_iter (DeeModel *self)
{
  g_return_val_if_fail (DEE_IS_TRANSACTION (self), NULL);
  g_return_val_if_fail (!dee_transaction_is_committed (AS_TXN (self)), NULL);

  /* We share end iter with the target model */
  return dee_model_get_last_iter (DEE_TRANSACTION_TARGET (self));
}

// dee_transaction_get_iter_at_row () // Fall through to O(n) scan

static DeeModelIter*
dee_transaction_next (DeeModel     *self,
                      DeeModelIter *iter)
{
  DeeTransactionPrivate *priv;
  IterType               itype;
  JournalIter           *jiter;
  JournalSegment        *jseg;

  // FIXME: Strictly - this method will work even if 'iter' has been marked
  //        removed. It might be nice to complain if anyone does this...

  g_return_val_if_fail (DEE_IS_TRANSACTION (self), NULL);
  g_return_val_if_fail (!dee_transaction_is_committed (AS_TXN (self)), NULL);

  priv = DEE_TRANSACTION (self)->priv;

  iter = dee_transaction_next_raw (self, iter, &itype);

  /* Now scan forwards until we have something which is not deleted.
   * This is complicated by the fact that we may have attached segments
   * before iters that have been marked removed */
  jiter = JOURNAL_ITER (iter);
  while (itype == ITER_TYPE_JOURNAL && journal_iter_is_removed (jiter))
    {
      iter = dee_transaction_next_raw (self, iter, &itype);

      if (itype == ITER_TYPE_JOURNAL)
        {
          jiter = JOURNAL_ITER (iter);
        }

      if ((jseg = get_journal_segment_before (iter)) != NULL)
        {
          return MODEL_ITER (jseg->first_iter);
        }
    }

  return iter;
}

static DeeModelIter*
dee_transaction_prev (DeeModel     *self,
                      DeeModelIter *iter)
{
  DeeTransactionPrivate *priv;
  JournalIter           *jiter, *jiter_prev;
  JournalSegment        *jseg;

  g_return_val_if_fail (DEE_IS_TRANSACTION (self), NULL);
  g_return_val_if_fail (!dee_transaction_is_committed (AS_TXN (self)), NULL);

  priv = DEE_TRANSACTION (self)->priv;

  if (check_journal_iter(iter, &jiter))
    {
      if (jiter->prev_iter)
        {
          return MODEL_ITER (jiter->prev_iter);
        }
      else if (dee_model_is_first (priv->target, jiter->segment->target_iter))
        {
          g_critical ("Trying to step before beginning of transaction model");
          return MODEL_ITER (jiter);
        }
      else
        {
          iter = dee_model_prev (priv->target, jiter->segment->target_iter);
          if (check_journal_iter (iter, &jiter_prev))
            return MODEL_ITER (jiter_prev);
          else
            return iter;
        }
      g_assert_not_reached ();
    }

  /* If there's a segment before the current iter in the target model,
   * step into that segment. Otherwise just step normally on the target */
  jseg = get_journal_segment_before (iter);
  if (jseg != NULL)
    return MODEL_ITER (jseg->last_iter);
  else
    return dee_model_prev (priv->target, iter);
}

/* Inherited methods */

// dee_transaction_is_first ()
// dee_transaction_is_last ()
// dee_transaction_get_position () // Generic O(n) traversal

static DeeModelTag*
dee_transaction_register_tag    (DeeModel       *self,
                                 GDestroyNotify  tag_destroy)
{
  g_return_val_if_fail (DEE_IS_TRANSACTION (self), NULL);

  g_critical ("DeeTransaction models can not create new tags, "
              "only re-use those of the target model");

  return NULL;
}

static gpointer
dee_transaction_get_tag (DeeModel       *self,
                         DeeModelIter   *iter,
                         DeeModelTag    *tag)
{
  g_return_val_if_fail (DEE_IS_TRANSACTION (self), NULL);

  // FIXME: Read through, or fetch from journal
  g_error ("Not implemented");

  return NULL;
}

static void
dee_transaction_set_tag (DeeModel       *self,
                         DeeModelIter   *iter,
                         DeeModelTag    *tag,
                         gpointer        value)
{
  g_return_if_fail (DEE_IS_TRANSACTION (self));

  // FIXME: Write through or to journal
  g_error ("Not implemented");
}

/*
 * Signal handlers on the target model.
 * Used to detect concurrent changes - which we do not support
 */

static void
on_target_modified (DeeTransaction *self,
                    DeeModelIter  *iter)
{
  if (self->priv->error_code != DEE_TRANSACTION_ERROR_COMMITTED)
    self->priv->error_code = DEE_TRANSACTION_ERROR_CONCURRENT_MODIFICATION;
}

/*
 * PUBLIC API
 */

/**
 * dee_transaction_new:
 * @target: The #DeeModel the transaction applies against
 *
 * Returns: (transfer full) (type Dee.Transaction):
 *     A newly allocated #DeeTransaction. Free with g_object_unref() when 
 *     done using it - no matter if you call dee_transaction_commit() or not.
 */
DeeModel*
dee_transaction_new (DeeModel *target)
{
  g_return_val_if_fail (DEE_IS_MODEL (target), NULL);

  return DEE_MODEL (g_object_new (DEE_TYPE_TRANSACTION,
                                  "target", target,
                                  NULL));
}

/**
 * dee_transaction_get_target:
 * @self: The transaction to retrieve the target model for
 *
 * Get the target model of a transaction. This is just a convenience method
 * for accessing the :target property.
 *
 * Returns: (transfer none): The target model
 */
DeeModel*
dee_transaction_get_target (DeeTransaction *self)
{
  g_return_val_if_fail (DEE_IS_TRANSACTION (self), FALSE);

  return self->priv->target;
}

/**
 * dee_transaction_is_committed:
 * @self: The transaction to inspect
 *
 * Check if a #DeeTransaction has been committed. This method is mainly for
 * debugging and testing purposes.
 *
 * Returns: %TRUE if and only if dee_transaction_commit() has completed
 *          successfully on the transaction.
 */
gboolean
dee_transaction_is_committed (DeeTransaction  *self)
{
  g_return_val_if_fail (DEE_IS_TRANSACTION (self), FALSE);

  return self->priv->error_code == DEE_TRANSACTION_ERROR_COMMITTED;
}

static const gchar*
get_txn_error_string (guint error_code)
{
  switch (error_code)
  {
    case DEE_TRANSACTION_ERROR_COMMITTED:
      return "Already committed";
    case DEE_TRANSACTION_ERROR_CONCURRENT_MODIFICATION:
      return "Target model has been concurrently modified";
    default:
      return "Unknown error";
  }
}

/**
 * dee_transaction_commit:
 * @self: The transaction to commit
 * @error: (allow-none): Location to return a #GError in or %NULL to disregard
 *                       errors
 *
 * Apply a transaction to its target model. After this call the transaction
 * is invalidated and must be freed with g_object_unref().
 *
 * Returns: %TRUE if and only if the transaction successfully applies to :target.
 */
gboolean
dee_transaction_commit (DeeTransaction *self, GError **error)
{
  DeeTransactionPrivate *priv;
  JournalIter           *jiter, *seg_iter, *free_jiter;
  GSList                *segments_to_free;
  DeeModelIter          *iter;

  g_return_val_if_fail (DEE_IS_TRANSACTION (self), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  priv = self->priv;

  if (priv->error_code != 0)
    {
      g_set_error (error,
                   DEE_TRANSACTION_ERROR,
                   priv->error_code,
                   "Error committing transaction. %s",
                   get_txn_error_string (priv->error_code));
      return FALSE;
    }

  if (DEE_IS_SERIALIZABLE_MODEL (priv->target) &&
      priv->begin_seqnum != dee_serializable_model_get_seqnum (priv->target))
    {
      g_set_error (error,
                   DEE_TRANSACTION_ERROR,
                   DEE_TRANSACTION_ERROR_CONCURRENT_MODIFICATION,
                   "Target model seqnum has changed during the transaction");
      return FALSE;
    }

  /* Because we have many criss crossing references to the segments we need
   * to collect them carefully to free all of them; and each only once! */
  segments_to_free = NULL;

  /* To avoid an extra traversal on finalize() we free the journal iters
   * as we traverse them now. The txn is illegal after commit() by API
   * contract anyway */
  for (jiter = priv->first_playback; jiter != NULL; )
    {
      switch (jiter->change_type)
      {
        case CHANGE_TYPE_ADD:
          /* We can commit the whole segment since a segment is comprised
           * purely of additions */

          if (jiter->segment->is_committed)
            break;

          for (seg_iter = jiter->segment->first_iter,
               iter = jiter->segment->target_iter;
               seg_iter;
               seg_iter = seg_iter->next_iter)
            {
              dee_model_insert_row_before (priv->target,
                                           iter,
                                           seg_iter->row_data);
            }

          jiter->segment->is_committed = TRUE;
          segments_to_free = g_slist_prepend (segments_to_free, jiter->segment);
          break;
        case CHANGE_TYPE_REMOVE:
          dee_model_remove (priv->target, jiter->override_iter);
          break;
        case CHANGE_TYPE_CHANGE:
          dee_model_set_row (priv->target,
                             jiter->override_iter,
                             jiter->row_data);
          break;
        default:
          g_critical ("Unexpected change type %u", jiter->change_type);
          break;
      }

      free_jiter = jiter;
      jiter = jiter->next_playback;
      journal_iter_free (free_jiter);
    }

  /* By now all jiters  have been freed. Remaining is to free the segments */
  g_slist_free_full (segments_to_free, (GDestroyNotify) journal_segment_free);

  priv->first_playback = NULL;
  priv->last_playback = NULL;

  priv->error_code = DEE_TRANSACTION_ERROR_COMMITTED;
  return TRUE;
}

GQuark
dee_transaction_error_quark (void)
{
  return g_quark_from_static_string ("dee-transaction-error-quark");
}

