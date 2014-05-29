//
// "$Id: Menu_global.cxx 9011 2011-08-25 21:50:59Z matt $"
//
// Global menu shortcut code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

// Make all the shortcuts in this menu global.
// Currently only one menu at a time and you cannot destruct the menu,
// is this sufficient?

#include "run.h"
#include "Menu_.h"

static fltk3::Menu_* the_widget;

static int handler(int e)
{
	if (e != fltk3::SHORTCUT || fltk3::modal()) return 0;
	fltk3::first_window(the_widget->window());
	return the_widget->handle(e);
}

/**
  Make the shortcuts for this menu work no matter what window has the
  focus when you type it.  This is done by using
  fltk3::add_handler().  This fltk3::Menu_ widget does not
  have to be visible (ie the window it is in can be hidden, or it does
  not have to be put in a window at all).
  <P>Currently there can be only one global()menu.  Setting a new
  one will replace the old one.  There is no way to remove the
  global() setting (so don't destroy the widget!)
*/
void fltk3::Menu_::global()
{
	if (!the_widget) fltk3::add_handler(handler);
	the_widget = this;
}

//
// End of "$Id: Menu_global.cxx 9011 2011-08-25 21:50:59Z matt $".
//
