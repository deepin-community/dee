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

#include <time.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>

#include <dee.h>

static void
on_swarm_leader_changed (GObject *peer_o, GParamSpec *pspec, gpointer user_data)
{
  gchar *swarm_leader;

  g_object_get (peer_o, "swarm-leader", &swarm_leader, NULL);
  g_printf ("Swarm leader changed: %s\n", swarm_leader);
  g_free (swarm_leader);
}

static void
on_peer_found (DeePeer *self, const gchar* peer_name)
{
  g_printf ("Peer found: %s\n", peer_name);
}

static void
on_peer_lost (DeePeer *self, const gchar* peer_name)
{
  g_printf ("Peer lost: %s\n", peer_name);
}

gint
main (gint argc, gchar *argv[])
{
  GMainLoop *loop;
  DeePeer  *peer;
  
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init ();
#endif
#if !GLIB_CHECK_VERSION(2, 32, 0)
  g_thread_init (NULL);
#endif

  peer = g_object_new (DEE_TYPE_PEER,
                       "swarm-name", "com.canonical.DeePeer.Test",
                       NULL);
  
  g_signal_connect (peer, "notify::swarm-leader", G_CALLBACK (on_swarm_leader_changed), NULL);
  g_signal_connect (peer, "peer-found", G_CALLBACK (on_peer_found), NULL);
  g_signal_connect (peer, "peer-lost", G_CALLBACK (on_peer_lost), NULL);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  return 0;
}
