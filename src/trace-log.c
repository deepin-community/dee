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

#include <glib-object.h>

#include "trace-log.h"

void
trace_object_va (void        *obj,
                 const gchar *format,
                 va_list      args)
{
  GString   *tmp;

  if (!G_IS_OBJECT(obj)) {
    g_critical ("Failed to log '%s' for object. Not an object.", format);
    return;
  }

  tmp = g_string_sized_new (512);
  g_string_printf (tmp, "(%s@%p): ", g_type_name(G_OBJECT_TYPE(obj)), obj);
  g_string_append (tmp, format);
  g_logv (TRACE_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, tmp->str, args);

  g_string_free (tmp, TRUE);
}

void
trace_object_real (void   		 *obj,
                   const gchar *format,
                   ...)
{
  va_list args;
  va_start (args, format);
  trace_object_va (obj, format, args);
  va_end (args);
}
