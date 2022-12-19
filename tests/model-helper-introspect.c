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

#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <string.h>
#include <gtx.h>

static gchar*
build_model_path (const gchar *model_name)
{
  gchar *dum = g_strdup (model_name);
  gchar *path;

  path = g_strconcat ("/com/canonical/dee/model/",
                      g_strdelimit (dum, ".", '/'),
                      NULL);
  g_free (dum);
  return path;
}

/* Does DBus introspection on a remote DeeModel */
gint
main (gint argc, gchar *argv[])
{
  GDBusConnection *conn;
  gchar           *model_path;
  gchar           *introspection_data;
  GError          *error;
  GVariant        *introspection_value;
  
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init (); 
#endif
  error = NULL;
  conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  if (error != NULL)
    {
      g_critical ("Unable to connect to session bus: %s", error->message);
      g_error_free (error);
      return 1;
    }

  model_path = build_model_path (argv[1]);

  error = NULL;
  introspection_value =
  g_dbus_connection_call_sync (conn,
                               argv[1],                          /* name */
                               model_path,                       /* obj path */
                               "org.freedesktop.DBus.Introspectable",
                               "Introspect",                     /* member */
                               NULL,                             /* arguments */
                               G_VARIANT_TYPE ("(s)"),           /* repl type*/
                               G_DBUS_CALL_FLAGS_NONE,
                               -1,                               /* timeout */
                               NULL,                             /* cancel */
                               &error);
  if (error != NULL)
    {
      g_critical ("Unable to get introspection data: %s", error->message);
      g_error_free (error);
      return 2;
    }

  if (introspection_value == NULL)
    {
      g_critical ("Introspection data was NULL");
      return 3;
    }

  g_variant_get_child (introspection_value, 0, "s", &introspection_data);
  
  if (strstr (introspection_data, "Clone") == NULL)  
    {
      g_critical ("Introspection data does not declare Clone method:\n%s",
                  introspection_data);
      return 4;
    }

  g_variant_unref (introspection_value);
  g_free (introspection_data);
  g_free (model_path);
  g_object_unref (conn);
  
  return 0;
}
