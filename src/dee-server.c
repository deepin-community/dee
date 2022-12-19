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
 * Authored by: Michal Hruby <michal.hruby@canonical.com>
 *
 */
/**
 * SECTION:dee-server
 * @short_description: Creates a server object you can connect to.
 * @include: dee.h
 *
 * #DeeServer allows you to create private connections (connections which 
 * are not routed via dbus-daemon). Note that, unlike #DeePeer, #DeeServer
 * will always be swarm leader, and clients connected to it cannot overtake
 * swarm leadership once the server connection is closed.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>

#include "dee-server.h"
#include "dee-marshal.h"
#include "trace-log.h"

G_DEFINE_TYPE (DeeServer, dee_server, DEE_TYPE_PEER)

#define GET_PRIVATE(o) \
      (G_TYPE_INSTANCE_GET_PRIVATE ((o), DEE_TYPE_SERVER, DeeServerPrivate))

#define ACTIVE_CONNECTIONS_KEY "dee-active-connections-list"
#define CONNECTION_ACCEPTED_KEY "dee-connection-accepted"

/**
 * DeeServerPrivate:
 *
 * Ignore this structure.
 **/
struct _DeeServerPrivate
{
  GCredentials *our_creds;
  GDBusServer  *server;
  gchar        *bus_address;
  gboolean      same_user_only;
  guint         initialize_server_timer_id;

  GSList       *active_connections;
  guint         connection_id;
  GHashTable   *connection_id_map;
};

/* Globals */
enum
{
  PROP_0,
  PROP_BUS_ADDRESS,
  PROP_SAME_USER_ONLY
};

enum
{
  LAST_SIGNAL
};

//static guint32 _server_signals[LAST_SIGNAL] = { 0 };
static GHashTable *active_servers = NULL;

/* Forwards */
static gboolean on_new_connection               (GDBusServer *server,
                                                 GDBusConnection *connection,
                                                 gpointer user_data);

static void     on_connection_closed            (GDBusConnection *connection,
                                                 gboolean remote_peer_vanished,
                                                 GError *error,
                                                 gpointer user_data);

static gboolean dee_server_is_swarm_leader      (DeePeer *peer);

static const gchar* dee_server_get_swarm_leader (DeePeer *peer);

static GSList*  dee_server_get_connections      (DeePeer *peer);

static gchar**  dee_server_list_peers           (DeePeer *peer);

/* GObject methods */
static void
dee_server_get_property (GObject *object, guint property_id,
                         GValue *value, GParamSpec *pspec)
{
  DeeServerPrivate *priv = DEE_SERVER (object)->priv;

  switch (property_id)
    {
      case PROP_BUS_ADDRESS:
        g_value_set_string (value, priv->bus_address);
        break;
      case PROP_SAME_USER_ONLY:
        g_value_set_boolean (value, priv->same_user_only);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
dee_server_set_property (GObject *object, guint property_id,
                         const GValue *value, GParamSpec *pspec)
{
  DeeServerPrivate *priv = DEE_SERVER (object)->priv;

  switch (property_id)
    {
      case PROP_BUS_ADDRESS:
        if (priv->bus_address) g_free (priv->bus_address);
        priv->bus_address = g_value_dup_string (value);
        break;
      case PROP_SAME_USER_ONLY:
        priv->same_user_only = g_value_get_boolean (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
remove_connection (GDBusConnection *connection, gboolean remote_peer_vanished,
                   GError *error, GDBusServer *server)
{
  GSList *list;

  trace_object (server, "Removing [%p] from list of active connections",
                connection);
  list = (GSList*) g_object_steal_data (G_OBJECT (server),
                                        ACTIVE_CONNECTIONS_KEY);

  list = g_slist_remove (list, connection);

  g_object_set_data_full (G_OBJECT (server), ACTIVE_CONNECTIONS_KEY,
                          list, (GDestroyNotify) g_slist_free);
}

static void
connection_finalized (gpointer server, GObject *connection)
{
  remove_connection ((GDBusConnection*) connection, FALSE, NULL,
                     (GDBusServer*) server);
}

static gboolean
add_new_connection (GDBusServer *server, GDBusConnection *connection,
                    gpointer user_data)
{
  GSList *list;
  gpointer *data;

  data = g_object_steal_data (G_OBJECT (connection), CONNECTION_ACCEPTED_KEY);

  if (data != NULL)
    {
      list = (GSList*) g_object_steal_data (G_OBJECT (server),
                                            ACTIVE_CONNECTIONS_KEY);
      list = g_slist_prepend (list, connection);
      g_object_set_data_full (G_OBJECT (server), ACTIVE_CONNECTIONS_KEY,
                              list, (GDestroyNotify) g_slist_free);

      g_signal_connect (connection, "closed",
                        G_CALLBACK (remove_connection), server);
      /* the connections in our list are weak references, need to make sure
       * they're not used after they're freed */
      g_object_weak_ref (G_OBJECT (connection), connection_finalized, server);
    }

  /* accept the connection if any of the DeeServers accepted it */
  return data != NULL;
}

static void
server_toggle_cb (gpointer data, GObject *object, gboolean is_last_ref)
{
  GSList *list, *iter;
  if (!is_last_ref) return;

  g_hash_table_remove (active_servers, data);

  g_dbus_server_stop (G_DBUS_SERVER (object));

  list = (GSList*) g_object_get_data (object, ACTIVE_CONNECTIONS_KEY);
  for (iter = list; iter != NULL; iter = iter->next)
    {
      g_object_weak_unref (iter->data, connection_finalized, object);
      g_signal_handlers_disconnect_by_func (iter->data,
                                            remove_connection, object);
    }

  /* and this will finalize the object */
  g_object_remove_toggle_ref (object, server_toggle_cb, data);
}

static GDBusServer*
get_server_for_address (const gchar *bus_address, GError **error)
{
  gchar            *guid;
  gchar            *address;
  GDBusServer      *server;
  GDBusServerFlags  server_flags;

  server = g_hash_table_lookup (active_servers, bus_address);
  if (server != NULL)
    {
      return g_object_ref (server);
    }

  /* create new GDBusServer instance */
  guid = g_dbus_generate_guid ();
  server_flags = G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;

  server = g_dbus_server_new_sync (bus_address, server_flags, guid,
                                   NULL, NULL, error);

  if (error && *error) return NULL;

  /* need to keep a list of all connections for this GDBusServer */
  g_signal_connect_after (server, "new-connection",
                          G_CALLBACK (add_new_connection), NULL);

  address = g_strdup (bus_address);
  /* transfer ownership of address to the hash table */
  g_hash_table_insert (active_servers, address, server);
  /* we need to stop the server before unreffing it the last time, so we'll
   * use toggle ref. FIXME: toggle ref is odd, use just weak ref (to remove
   * it from the active_servers hash table) once it properly stops
   * the listener on finalize. */
  g_object_add_toggle_ref (G_OBJECT (server), server_toggle_cb, address);

  g_free (guid);

  return server;
}

static gboolean
initialize_server (DeeServer *self)
{
  DeeServerPrivate *priv;
  GSList           *connections;
  GError           *error = NULL;

  priv = self->priv;

  priv->initialize_server_timer_id = 0;

  /* create new server or get the existing instance for this bus_address */
  priv->server = get_server_for_address (priv->bus_address, &error);

  if (error)
    {
      g_critical ("Unable to set up DBusServer: %s", error->message);

      g_error_free (error);

      g_object_notify (G_OBJECT (self), "swarm-leader");
      return FALSE;
    }

  g_signal_connect (priv->server, "new-connection",
                    G_CALLBACK (on_new_connection), self);

  g_dbus_server_start (priv->server);

  g_object_notify (G_OBJECT (self), "swarm-leader");

  /* were there any connections already? */
  connections = (GSList*) g_object_get_data (G_OBJECT (priv->server),
                                             ACTIVE_CONNECTIONS_KEY);
  for ( ; connections != NULL; connections = connections->next)
    {
      on_new_connection (priv->server, (GDBusConnection*) connections->data,
                         self);
    }

  return FALSE;
}

static void
dee_server_constructed (GObject *self)
{
  DeeServerPrivate *priv;
  const gchar      *swarm_name;

  priv = DEE_SERVER (self)->priv;

  /* we should chain up the constructed method here, but peer does things we
   * don't want to, so not chaining up... */

  swarm_name = dee_peer_get_swarm_name (DEE_PEER (self));
  if (swarm_name == NULL)
    {
      g_critical ("DeeServer created without a swarm name. You must specify "
                  "a non-NULL swarm name");
      return;
    }

  priv->our_creds = g_credentials_new ();

  if (!priv->bus_address)
  {
    priv->bus_address = dee_server_bus_address_for_name (swarm_name, 
                                                         priv->same_user_only);
  }

  /* Ideally we'd call the async variant of g_dbus_server_new (which doesn't
   * exist atm) */
  priv->initialize_server_timer_id = g_idle_add_full (G_PRIORITY_DEFAULT,
      (GSourceFunc)initialize_server, self, NULL);
}

static void
close_connection (gpointer data, gpointer user_data)
{
  GDBusConnection *connection = G_DBUS_CONNECTION (data);

  g_signal_handlers_disconnect_by_func (connection, on_connection_closed,
                                        user_data);

  // FIXME: should we use the sync variant? and flush first?
  //g_dbus_connection_close (connection, NULL, NULL, NULL);
}

static void
dee_server_finalize (GObject *object)
{
  DeeServerPrivate *priv;

  priv = DEE_SERVER (object)->priv;

  if (priv->initialize_server_timer_id)
    {
      g_source_remove (priv->initialize_server_timer_id);
      priv->initialize_server_timer_id = 0;
    }
  
  if (priv->active_connections)
    {
      g_slist_foreach (priv->active_connections, close_connection, object);
      g_slist_free_full (priv->active_connections, g_object_unref);
      priv->active_connections = NULL;
    }

  if (priv->server)
    {
      g_signal_handlers_disconnect_by_func (priv->server, on_new_connection, object);

      /* this should be done automatically on unref, but it doesn't seem so */
      g_dbus_server_stop (priv->server);

      g_object_unref (priv->server);
    }

  if (priv->connection_id_map)
    {
      g_hash_table_unref (priv->connection_id_map);
      priv->connection_id_map = NULL;
    }

  if (priv->bus_address)
    {
      g_free (priv->bus_address);
    }

  if (priv->our_creds)
    {
      g_object_unref (priv->our_creds);
      priv->our_creds = NULL;
    }

  G_OBJECT_CLASS (dee_server_parent_class)->finalize (object);
}

static void
dee_server_class_init (DeeServerClass *klass)
{
  GParamSpec   *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DeePeerClass *peer_class = DEE_PEER_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DeeServerPrivate));

  object_class->constructed = dee_server_constructed;
  object_class->get_property = dee_server_get_property;
  object_class->set_property = dee_server_set_property;
  object_class->finalize = dee_server_finalize;

  peer_class->is_swarm_leader  = dee_server_is_swarm_leader;
  peer_class->get_swarm_leader = dee_server_get_swarm_leader;
  peer_class->get_connections  = dee_server_get_connections;
  peer_class->list_peers       = dee_server_list_peers;

  /**
   * DeeServer::bus-address:
   *
   * D-Bus address the server is bound to. If you do not specify this property
   * #DeeServer will use dee_server_bus_address_for_name() using current swarm
   * name to determine the value of this property.
   * You can use dee_server_get_client_address() to get address string
   * that can be used by clients to connect to this #DeeServer instance.
   */
  pspec = g_param_spec_string ("bus-address", "Bus address",
                               "Bus address to use for the connection",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
                               | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_BUS_ADDRESS, pspec);

  /**
   * DeeServer::same-user-only:
   *
   * A boolean specifying whether the server should only accept connections
   * from same user as the current process owner.
   */
  pspec = g_param_spec_boolean ("same-user-only", "Same user only",
                                "Accept connections from current user only",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
                                | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SAME_USER_ONLY, pspec);

  /* we'll use this variable to share GDBusServer instances between mutliple
   * DeeServers - this will enable us to use one connection with multiple
   * models with different swarm_names */
  active_servers = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
dee_server_init (DeeServer *self)
{
  self->priv = GET_PRIVATE (self);

  self->priv->connection_id_map = g_hash_table_new_full (g_direct_hash,
                                                         g_direct_equal,
                                                         NULL,
                                                         g_free);
}

/**
 * dee_server_new:
 * @swarm_name: Name of swarm to join.
 *
 * Creates a new instance of #DeeServer and tries to bind 
 * to #DeeServer:bus-address. The #DeePeer:swarm-leader property will be set
 * when the binding succeeds.
 *
 * <note>
 *   <para>
 *     Note that this function will automatically determine the value
 *     of #DeeServer:bus-address property and will generally cause your
 *     application to use new socket for every #DeeServer with different swarm
 *     name. See dee_server_new_for_address() if you'd like to share one
 *     connection between multiple #DeeServer instances.
 *   </para>
 * </note>
 *
 * Return value: (transfer full): A newly constructed #DeeServer.
 */
DeeServer*
dee_server_new (const gchar* swarm_name)
{
  g_return_val_if_fail (swarm_name != NULL, NULL);

  return DEE_SERVER (g_object_new (DEE_TYPE_SERVER,
                                   "swarm-name", swarm_name, NULL));
}

/**
 * dee_server_new_for_address:
 * @swarm_name: Name of swarm to join.
 * @bus_address: D-Bus address to use for the connection.
 *
 * Creates a new instance of #DeeServer and tries to bind to @bus_address.
 * The #DeePeer:swarm-leader property will be set when the binding succeeds.
 *
 * If there is already a #DeeServer instance bound to @bus_address,
 * the connection will be shared with the newly constructed instance.
 *
 * <note>
 *   <para>
 *     This function is primarily meant for sharing of one connection (socket)
 *     between multiple DeeServers, so that you can create #DeeServer instances
 *     with varying swarm names, but the same bus address, which will cause
 *     them to share the connection (the sharing is possible only within
 *     the same process though).
 *   </para>
 * </note>
 *
 * Return value: (transfer full): A newly constructed #DeeServer.
 */
DeeServer*
dee_server_new_for_address (const gchar* swarm_name, const gchar* bus_address)
{
  g_return_val_if_fail (swarm_name != NULL, NULL);

  return DEE_SERVER (g_object_new (DEE_TYPE_SERVER,
                                   "swarm-name", swarm_name,
                                   "bus-address", bus_address, NULL));
}

/**
 * dee_server_get_client_address:
 * @server: A #DeeServer.
 *
 * Gets a D-Bus address string that can be used by clients to connect to server.
 *
 * Return value: A D-Bus address string. Do not free.
 */
const gchar*
dee_server_get_client_address (DeeServer *server)
{
  DeeServerPrivate *priv;
  g_return_val_if_fail (DEE_IS_SERVER (server), NULL);

  priv = server->priv;

  return priv->server != NULL ? 
    g_dbus_server_get_client_address (priv->server) : NULL;
}

/**
 * dee_server_bus_address_for_name:
 * @name: A name to create bus address for.
 * @include_username: Include current user name as part of the bus address.
 *
 * Helper method which creates bus address string for the given name, which
 * should have the same format as a DBus unique name.
 * 
 * Return value: (transfer full): Newly allocated string with bus address.
 *                                Use g_free() to free.
 */
gchar*
dee_server_bus_address_for_name (const gchar *name, gboolean include_username)
{
  gchar *result;

  g_return_val_if_fail (name != NULL, NULL);

  if (g_unix_socket_address_abstract_names_supported ())
    {
      result = include_username ? 
        g_strdup_printf ("unix:abstract=%s-%s", g_get_user_name (), name) :
        g_strdup_printf ("unix:abstract=%s", name);
    }
  else
    {
      result = include_username ?
        g_strdup_printf ("unix:path=%s/%s-%s", g_get_tmp_dir (), 
                                               g_get_user_name (), name) :
        g_strdup_printf ("unix:path=%s/%s", g_get_tmp_dir (), name);
    }

  return result;
}

/* Private Methods */
static gboolean
on_new_connection (GDBusServer *server,
                   GDBusConnection *connection,
                   gpointer user_data)
{
  gchar *connection_name;
  GCredentials *creds;
  DeeServer *self = DEE_SERVER (user_data);
  DeeServerPrivate *priv = self->priv;

  trace_object (self, "New connection: [%p]", connection);

  creds = g_dbus_connection_get_peer_credentials (connection);
  if (!g_credentials_is_same_user (creds, priv->our_creds, NULL)
      && priv->same_user_only)
    {
      trace_object (self, "User id doesn't match, rejecting connection");
      return FALSE;
    }

  priv->active_connections = g_slist_prepend (priv->active_connections,
                                              g_object_ref (connection));

  g_signal_connect (connection, "closed",
                    G_CALLBACK (on_connection_closed), self);

  /* time to register dbus objects on this connection */
  g_signal_emit_by_name (self, "connection-acquired", connection);

  connection_name = g_strdup_printf ("%s:%u",
                                     g_dbus_server_get_guid (priv->server),
                                     ++priv->connection_id);
  /* hash table assumes ownership of connection_name */
  g_hash_table_insert (priv->connection_id_map, connection, connection_name);

  g_signal_emit_by_name (self, "peer-found", connection_name);

  g_object_set_data (G_OBJECT (connection), CONNECTION_ACCEPTED_KEY,
                     GINT_TO_POINTER (1));

  /* we can't return TRUE, otherwise handlers in other DeeServers wouldn't run,
   * see add_new_connection callback */
  return FALSE;
}

static void
on_connection_closed (GDBusConnection *connection,
                      gboolean remote_peer_vanished,
                      GError *error,
                      gpointer user_data)
{
  GSList           *element;
  DeeServer        *self = DEE_SERVER (user_data);
  DeeServerPrivate *priv = self->priv;

  trace_object (self, "Connection [%p] was closed", connection);

  element = g_slist_find (priv->active_connections, connection);
  if (element == NULL)
    {
      g_warning ("Connection closed for element which isn't "
                 "in active_connections");
      return;
    }

  priv->active_connections = g_slist_delete_link (priv->active_connections,
                                                  element);

  /* reverse order of signals than in new-connection handler */
  g_signal_emit_by_name (self, "peer-lost",
                         g_hash_table_lookup (priv->connection_id_map,
                                              connection));
  g_hash_table_remove (priv->connection_id_map, connection);

  g_signal_emit_by_name (self, "connection-closed", connection);

  g_object_unref (connection);
}

static gboolean
dee_server_is_swarm_leader (DeePeer *peer)
{
  DeeServerPrivate *priv = DEE_SERVER (peer)->priv;
  return priv->server != NULL;
}

static const gchar*
dee_server_get_swarm_leader (DeePeer *peer)
{
  DeeServerPrivate *priv = DEE_SERVER (peer)->priv;

  return priv->server ? g_dbus_server_get_guid (priv->server) : NULL;
}

static GSList*
dee_server_get_connections (DeePeer *peer)
{
  DeeServerPrivate *priv = DEE_SERVER (peer)->priv;

  return g_slist_copy (priv->active_connections);
}

static gchar**
dee_server_list_peers (DeePeer *peer)
{
  GSList *it;
  gchar **result;
  int i;
  DeeServerPrivate *priv = DEE_SERVER (peer)->priv;

  result = g_new (gchar*, g_slist_length (priv->active_connections) + 1);
  i = 0;

  for (it = priv->active_connections; it != NULL; it = it->next)
    {
      result[i++] = g_strdup ((gchar*) g_hash_table_lookup (priv->connection_id_map, it->data));
    }

  result[i] = NULL;

  return result;
}

