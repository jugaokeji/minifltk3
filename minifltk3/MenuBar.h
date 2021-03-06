//
// "$Id: MenuBar.h 9638 2012-07-24 04:41:38Z matt $"
//
// Menu bar header file for the Fast Light Tool Kit (FLTK).
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

/* \file
 fltk3::MenuBar widget . */

#ifndef Fltk3_Menu_Bar_H
#define Fltk3_Menu_Bar_H

#include "run.h"
#include "Menu_.h"
#include "draw.h"

namespace fltk3
{

/**
 This widget provides a standard menubar interface.  Usually you will
 put this widget along the top edge of your window.  The height of the
 widget should be 30 for the menu titles to draw correctly with the
 default font.
 <P>The items on the bar and the menus they bring up are defined by a
 single fltk3::MenuItem
 array.  Because a fltk3::MenuItem array defines a hierarchy, the
 top level menu defines the items in the menubar, while the submenus
 define the pull-down menus. Sub-sub menus and lower pop up to the right
 of the submenus. </P>
 <P ALIGN=CENTER>\image html  menubar.png</P>
 \image latex  menubar.png " menubar" width=12cm
 <P>If there is an item in the top menu that is not a title of a
 submenu, then it acts like a "button" in the menubar.  Clicking on it
 will pick it. </P>
 <P>When the user picks an item off the menu, the item's callback is
 done with the menubar as the fltk3::Widget* argument.  If the item
 does not have a callback the menubar's callback is done instead. </P>
 <P>Submenus will also pop up in response to shortcuts indicated by
 putting a '&' character in the name field of the menu item. If you put a
 '&' character in a top-level "button" then the shortcut picks it.  The
 '&' character in submenus is ignored until the menu is popped up. </P>
 <P>Typing the shortcut() of any of the menu items will cause
 callbacks exactly the same as when you pick the item with the mouse.
 */
class FLTK3_EXPORT MenuBar : public fltk3::Menu_
{

protected:
	void draw()
	{
		draw_box();
		if (!menu() || !menu()->text) return;
		const fltk3::MenuItem* m;
		int X = 6;
		for (m=menu()->first(); m->text; m = m->next()) {
			int W = m->measure(0,this) + 16;
			m->draw(X, 0, W, h(), this);
			X += W;
			if (m->flags & fltk3::MENU_DIVIDER) {
				int y1 = fltk3::box_dy(box());
				int y2 = y1 + h() - fltk3::box_dh(box()) - 1;

				// Draw a vertical divider between menus...
				fltk3::color(fltk3::DARK3);
				fltk3::yxline(X - 6, y1, y2);
				fltk3::color(fltk3::LIGHT3);
				fltk3::yxline(X - 5, y1, y2);
			}
		}
	}

public:
	int handle(int event)
	{
		const fltk3::MenuItem* v;
		if (menu() && menu()->text) 
			switch (event) {
			case fltk3::ENTER:
			case fltk3::LEAVE:
				return 1;
			case fltk3::PUSH:
				v = 0;
J1:
				v = menu()->pulldown(dx_window(), dy_window(), w(), h(), v, this, 0, 1);
				picked(v);
				return 1;
			case fltk3::SHORTCUT:
				if (visible_r()) {
					v = menu()->find_shortcut(0, true);
					if (v && v->submenu()) goto J1;
				}
				return test_shortcut() != 0;
			}
			return 0;
	}

	/**
	 Creates a new fltk3::MenuBar widget using the given position,
	 size, and label string. The default boxtype is fltk3::UP_BOX.
	 <P>The constructor sets menu() to NULL.  See
	 fltk3::Menu_ for the methods to set or change the menu. </P>
	 <P>labelsize(), labelfont(), and labelcolor()
	 are used to control how the menubar items are drawn.  They are
	 initialized from the fltk3::Menu static variables, but you can
	 change them if desired. </P>
	 <P>label() is ignored unless you change align() to
	 put it outside the menubar.
	 <P>The destructor removes the fltk3::MenuBar widget and all of its
	 menu items.
	 */
	MenuBar(int X, int Y, int W, int H,const char *l=0) : fltk3::Menu_(X,Y,W,H,l)
	{
	}
};

}

#endif

//
// End of "$Id: MenuBar.h 9638 2012-07-24 04:41:38Z matt $".
//
