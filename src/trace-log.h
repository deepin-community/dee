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

/*
 * This file contains some special GObject-centric debugging macros that
 * can be compiled completely out out of the final binary 
 */
 
#ifndef _TRACE_LOG_H
#define _TRACE_LOG_H

#include "config.h"

G_BEGIN_DECLS

#ifndef TRACE_LOG_DOMAIN

/**
 * TRACE_LOG_DOMAIN: (skip)
 *
 * The log domain of libdee
 */
#define TRACE_LOG_DOMAIN    "Dee"
#endif  /* TRACE_LOG_DOMAIN */

/*
 * Make trace() a noop if ENABLE_TRACE_LOG is not defined
 */
#ifdef ENABLE_TRACE_LOG

void     trace_object_va   (void *obj, const gchar *format, va_list args);
void     trace_object_real (void *obj, const gchar *format, ...);

#   ifdef G_HAVE_ISO_VARARGS
#	   define trace(...) g_log (TRACE_LOG_DOMAIN, \
                              G_LOG_LEVEL_DEBUG,  \
                              __VA_ARGS__)
#	   define trace_object(object, ...) trace_object_real (object, __VA_ARGS__)

#   elif defined(G_HAVE_GNUC_VARARGS)
#	   define trace(format...) g_log (TRACE_LOG_DOMAIN,   \
                                    G_LOG_LEVEL_DEBUG,	\
                                    format)
#	   define trace_object(object, format...) trace_object_real (object, format)
#   else   /* no varargs macros */
static void
trace (const gchar *format,
       ...)
{
	va_list args;
	va_start (args, format);
	g_logv (TRACE_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format, args);
	va_end (args);
}

static void
trace_object (void        *obj,
              const gchar *format,
              ...)
{
	va_list args;
	va_start (args, format);
	trace_object_va (obj, format, args);
	va_end (args);
}
#   endif  /* !__GNUC__ */

#else /* NO TRACE LOGGING OUTPUT */

#   ifdef G_HAVE_ISO_VARARGS
#	   define trace(...) G_STMT_START{ (void)0; }G_STMT_END
#	   define trace_object(object, ...) G_STMT_START{ (void)0; }G_STMT_END
#   elif defined(G_HAVE_GNUC_VARARGS)
#	   define trace(format...) G_STMT_START{ (void)0; }G_STMT_END
#	   define trace_object(object, format...) G_STMT_START{ (void)0; }G_STMT_END
#   else   /* no varargs macros */

static void trace (const gchar *format, ...) { ; }
static void trace_object (GObject *obj, const gchar *format, ...) { ; }

#   endif /* !__GNUC__ */

#endif /* ENABLE_TRACE_LOG */

G_END_DECLS

#endif /* _TRACE_LOG_H */
