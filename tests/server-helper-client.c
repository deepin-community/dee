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
 *              Michal Hruby <michal.hruby@canonical.com>
 *
 */

#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>

#include <gtx.h>
#include <dee.h>

static void
on_peer_found (DeePeer *peer, gchar *name, gint *n_peers)
{
  (*n_peers)++;
}

static void
on_peer_lost (DeePeer *peer, gchar *name, gint *n_peers)
{
  (*n_peers)++;
}

static int
peer_function (gchar *argv[])
{
  DeePeer      *peer;
  gint          n_peers = 0;

  peer = (DeePeer*) dee_client_new (argv[1]);
  
  g_signal_connect (peer, "peer-found", G_CALLBACK (on_peer_found), &n_peers);
  g_signal_connect (peer, "peer-lost", G_CALLBACK (on_peer_lost), &n_peers);

  if (gtx_wait_for_signal (G_OBJECT (peer), 10000, "notify::swarm-leader", NULL))
    g_error ("Peer helper timed out waiting for swarm leader");

  /* The main process should be swarm leaders. Not us */
  g_assert_cmpint (0, ==, dee_peer_is_swarm_leader (peer));

  /* At this point we shouldn't have emitted 'peer-found' yet */
  g_assert_cmpint (0, ==, n_peers);

  if (gtx_wait_for_signal (G_OBJECT (peer), 10000, "peer-found", NULL))
      g_error ("Peer helper timed out waiting for 'peer-found' signal");

  g_assert_cmpint (1, ==, n_peers);
  g_assert_cmpint (1, ==, g_strv_length (dee_peer_list_peers (peer)));

  gtx_assert_last_unref (peer);
  
  return 0;
}

static gint finished_children = 0;
static void
child_exited (GPid pid, gint status, gpointer user_data)
{
  finished_children++;
}

gint
main (gint argc, gchar *argv[])
{
  int num_clients, i;
  
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init ();
#endif
  if (argc < 3) g_error ("Invalid invocation");

  num_clients = i = atoi (argv[2]);
  while (i-- > 0)
    {
      GPid pid = (GPid) fork ();
      if (pid != 0) g_child_watch_add (pid, child_exited, NULL);
      else 
      {
        return peer_function (argv);
      }
    }

  for (i = 0; i < 10; i++)
  {
    gtx_yield_main_loop (200);
    if (finished_children == num_clients) break;
  }
  
  /* Give a window of opportunity for children to
   * flush stdout/err before we exit */
  gtx_yield_main_loop (200);

  g_assert_cmpint (num_clients, ==, finished_children);

  return 0;
}
