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

#include <gtx.h>
#include <dee.h>

static gint n_peers = 0;

static void
on_peer_found (DeePeer *peer)
{
  n_peers++;
}

static void
on_peer_lost (DeePeer *peer)
{
  n_peers--;
}

/* Expects a clone with 3 rows in it */
gint
main (gint argc, gchar *argv[])
{
  DeePeer  *peer;
  unsigned  num_peers;    
  
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init (); 
#endif
  peer = dee_peer_new (argv[1]);
  
  g_signal_connect (peer, "peer-found", G_CALLBACK (on_peer_found), NULL);
  g_signal_connect (peer, "peer-lost", G_CALLBACK (on_peer_lost), NULL);

  if (gtx_wait_for_signal (G_OBJECT (peer), 100000, "notify::swarm-leader", NULL))
    g_error ("Peer helper timed out waiting for swarm leader");

  /* The main process should be swarm leaders. Not us */
  g_assert_cmpint (0, ==, dee_peer_is_swarm_leader (peer));

  /* At this point we shouldn't have emitted 'peer-found' yet */
  g_assert_cmpint (0, ==, n_peers);

  if (gtx_wait_for_signal (G_OBJECT (peer), 100000, "peer-found", NULL))
      g_error ("Peer helper timed out waiting for 'peer-found' signal");

  g_assert_cmpint (1, ==, n_peers);
  /* Listing of peers includes also self */
  /* Listing of peers is in flaky state atm (should == 2), see lp:1076971 */
  num_peers = g_strv_length (dee_peer_list_peers (peer));
  g_assert_cmpint (2, >=, num_peers);
  g_assert_cmpint (1, <=, num_peers);

  return 0;
}
