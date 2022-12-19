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
 * Compile with:
 *
 * valac --vapidir ../vapi --pkg glib-2.0 --pkg gtk+-2.0 --pkg dee-1.0 master-model.vala -o master-model-vala -X -I../dee
 */

using GLib;
using Dee;

public class Master
{
  private Model model;

  public Master ()
  {
    this.model = new SharedModel ("com.canonical.Dee.Model.Example");
    model.set_schema ("i", "s");
    
    this.model.row_added.connect (this.on_row_added);

    GLib.Timeout.add_seconds (2, (GLib.SourceFunc) this.add);
  }

  private void on_row_added (ModelIter iter)
  {
    int            i;
    unowned string s;

    this.model.get (iter, 0, out i, out s);
  
    i = this.model.get_int32 (iter, 0);
    var ss = this.model.get_string (iter, 1);

    print (@"Master: $i $ss\n");
  }

  private bool add (Model *model)
  {
    this.model.append (10, "Rooney");
    return true;
  }

}

public static int main (string[] args)
{
  Master master;
  
  master = new Master ();

  Gtk.main ();

  return 0;
}
