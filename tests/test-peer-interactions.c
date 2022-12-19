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
 * Authored by Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */

#include <glib.h>
#include <glib-object.h>
#include <dee.h>
#include <gtx.h>

#define TIMEOUT 100
#define PEER_NAME "com.canonical.Dee.Peer.Tests.Interactions"

/* A command line that launches the appropriate peer-helper-* executable,
 * giving $name as first argument */
#define PEER_HELPER(helper,name) \
  (gchar *[]) { "./peer-helper-"#helper, name, NULL }


typedef struct
{
  DeePeer *peer;

} Fixture;

static void peer_setup         (Fixture *fix, gconstpointer data);
static void peer_teardown      (Fixture *fix, gconstpointer data);

static void test_allocation        (Fixture *fix, gconstpointer data);
static void test_become_leader     (Fixture *fix, gconstpointer data);
static void test_1peer             (Fixture *fix, gconstpointer data);

void
test_peer_interactions_create_suite (void)
{
#define DOMAIN "/Peer/Interactions"

  g_test_add (DOMAIN"/Allocation", Fixture, 0,
              peer_setup, test_allocation, peer_teardown);
  g_test_add (DOMAIN"/BecomeLeader", Fixture, 0,
              peer_setup, test_become_leader, peer_teardown);
  g_test_add (DOMAIN"/OnePeer", Fixture, 0,
              peer_setup, test_1peer, peer_teardown);
}

static void
peer_setup (Fixture *fix, gconstpointer data)
{   
  fix->peer = dee_peer_new (PEER_NAME);

  g_assert_cmpint (0, ==, dee_peer_is_swarm_leader (fix->peer));

  g_assert (DEE_IS_PEER (fix->peer));
}

static void
peer_teardown (Fixture *fix, gconstpointer data)
{
  gtx_assert_last_unref (fix->peer);

  /* Spin the mainloop a bit to check if we have any post-test
   * async effect crashing us */
  gtx_yield_main_loop (200);
}

static void
test_allocation (Fixture *fix, gconstpointer data)
{
  /* Do nothing, this test basically just asserts that
   * the fix->peer is cleaned up after immediate construction */
}

static void
test_become_leader (Fixture *fix, gconstpointer data)
{
  gtx_wait_for_signal (G_OBJECT (fix->peer), TIMEOUT,
                       "notify::swarm-leader", NULL);
  
  /* Assert that we have become swarm leaders.
   * No other peers should be running */
  g_assert_cmpint (0, !=, dee_peer_is_swarm_leader (fix->peer));
}

static void
test_1peer (Fixture *fix, gconstpointer data)
{
  /* Wait for us to become swarm leaders */
  gtx_wait_for_signal (G_OBJECT (fix->peer), TIMEOUT,
                         "notify::swarm-leader", NULL);

  g_assert_cmpint (0, !=, dee_peer_is_swarm_leader (fix->peer));

  /* We are now leaders - launch the helper */
  if (gtx_wait_for_command (TESTDIR,
                            PEER_HELPER (1peer, PEER_NAME),
                            1000))
    g_critical ("Peer helper timed out");

  gtx_assert_last_command_status (0);
}
