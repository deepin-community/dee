/*
 * Copyright (C) 2010 Canonical, Ltd.
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

#if !defined (_DEE_H_INSIDE) && !defined (DEE_COMPILATION)
#error "Only <dee.h> can be included directly."
#endif

#ifndef _HAVE_DEE_FILTER_MODEL_H
#define _HAVE_DEE_FILTER_MODEL_H

#include <glib.h>
#include <glib-object.h>

#include <dee-model.h>
#include <dee-proxy-model.h>

G_BEGIN_DECLS

#define DEE_TYPE_FILTER_MODEL (dee_filter_model_get_type ())

#define DEE_FILTER_MODEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        DEE_TYPE_FILTER_MODEL, DeeFilterModel))

#define DEE_FILTER_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
        DEE_TYPE_FILTER_MODEL, DeeFilterModelClass))

#define DEE_IS_FILTER_MODEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        DEE_TYPE_FILTER_MODEL))

#define DEE_IS_FILTER_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        DEE_TYPE_FILTER_MODEL))

#define DEE_FILTER_MODEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        DBUS_TYPE_FILTER_MODEL, DeeFilterModelClass))

typedef struct _DeeFilterModel DeeFilterModel;
typedef struct _DeeFilterModelClass DeeFilterModelClass;
typedef struct _DeeFilterModelPrivate DeeFilterModelPrivate;

/* We need this one here to avoid circular refs */
typedef struct _DeeFilter DeeFilter;

/**
 * DeeFilterModel:
 *
 * All fields in the DeeFilterModel structure are private and should never be
 * accessed directly
 */
struct _DeeFilterModel
{
  /*< private >*/
  DeeProxyModel          parent;

  DeeFilterModelPrivate *priv;
};

struct _DeeFilterModelClass
{
  /*< private >*/
  DeeProxyModelClass parent_class;

  /*< private >*/
  void (*_dee_filter_model_1) (void);
  void (*_dee_filter_model_2) (void);
  void (*_dee_filter_model_3) (void);
  void (*_dee_filter_model_4) (void);
};

/**
 * dee_filter_model_get_type:
 *
 * The GType of #DeeFilterModel
 *
 * Return value: the #GType of #DeeFilterModel
 **/
GType                 dee_filter_model_get_type        (void);

DeeModel*             dee_filter_model_new             (DeeModel  *orig_model,
                                                        DeeFilter *filter);

gboolean              dee_filter_model_contains        (DeeFilterModel *self,
                                                        DeeModelIter   *iter);

DeeModelIter*         dee_filter_model_append_iter     (DeeFilterModel *self,
                                                        DeeModelIter   *iter);

DeeModelIter*         dee_filter_model_prepend_iter     (DeeFilterModel *self,
                                                         DeeModelIter   *iter);

DeeModelIter*         dee_filter_model_insert_iter     (DeeFilterModel *self,                                                        
                                                        DeeModelIter   *iter,
                                                        guint           pos);

DeeModelIter*         dee_filter_model_insert_iter_before (DeeFilterModel *self,
                                                           DeeModelIter   *iter,
                                                           DeeModelIter   *pos);

DeeModelIter*         dee_filter_model_insert_iter_with_original_order (DeeFilterModel *self,
                                                                        DeeModelIter   *iter);

G_END_DECLS

#endif /* _HAVE_DEE_FILTER_MODEL_H */
