/*
 * Copyright (C) 2010-2012 Canonical Ltd
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

#include <gtx.h>
#include <dee.h>

/* Joins an existing model, and then tries to append a new row */
gint
main (gint argc, gchar *argv[])
{
  DeeModel     *model;
  
#if !GLIB_CHECK_VERSION(2, 35, 1)
  g_type_init (); 
#endif

  if (argc == 2)
    model = dee_shared_model_new (argv[1]);
  else
    model = dee_shared_model_new_for_peer ((DeePeer*) dee_client_new (argv[1]));

  if (gtx_wait_for_signal (G_OBJECT (model), 300, "notify::synchronized", NULL))
    {
      g_critical ("Model never synchronized");
      return 1;
    }

  dee_model_append (model, 68, "wumbo");

  gtx_yield_main_loop (500);
  
  gtx_assert_last_unref (model);
  
  return 0;
}
