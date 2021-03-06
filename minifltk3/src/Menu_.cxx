//
// "$Id: Menu_.cxx 9330 2012-04-09 08:53:49Z manolo $"
//
// Common menu code for the Fast Light Tool Kit (FLTK).
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
// This is a base class for all items that have a menu:
//	fltk3::MenuBar, fltk3::MenuButton, fltk3::Choice
// This provides storage for a menu item, functions to add/modify/delete
// items, and a call for when the user picks a menu item.

// More code in Fl_Menu_add.cxx

#include "run.h"
#include "Menu_.h"
#include "flstring.h"
#include <stdio.h>
#include <stdlib.h>

#define SAFE_STRCAT(s) { len += (int)strlen(s); if ( len >= namelen ) { *name='\0'; return(-2); } else strcat(name,(s)); }

/** Get the menu 'pathname' for the specified menuitem.

    If finditem==NULL, mvalue() is used (the most recently picked menuitem).

    \b Example:
    \code
      fltk3::MenuBar *menubar = 0;
      void my_menu_callback(fltk3::Widget*,void*) {
        char name[80];
        if ( menubar->item_pathname(name, sizeof(name)-1) == 0 ) {   // recently picked item
          if ( strcmp(name, "File/&Open") == 0 ) { .. }              // open invoked
          if ( strcmp(name, "File/&Save") == 0 ) { .. }              // save invoked
          if ( strcmp(name, "Edit/&Copy") == 0 ) { .. }              // copy invoked
        }
      }
      int main() {
        [..]
        menubar = new fltk3::MenuBar(..);
        menubar->add("File/&Open",  0, my_menu_callback);
        menubar->add("File/&Save",  0, my_menu_callback);
        menubar->add("Edit/&Copy",  0, my_menu_callback);
        [..]
      }
    \endcode

    \returns
	-   0 : OK (name has menuitem's pathname)
	-  -1 : item not found (name="")
	-  -2 : 'name' not large enough (name="")
    \see find_item()
*/
int fltk3::Menu_::item_pathname(char *name, int namelen, const fltk3::MenuItem *finditem) const
{
	int len = 0;
	finditem = finditem ? finditem : mvalue();
	name[0] = '\0';
	for ( int t=0; t<size(); t++ ) {
		const fltk3::MenuItem *m = &(menu()[t]);
		if ( m->submenu() ) {				// submenu? descend
			if (*name) SAFE_STRCAT("/");
			if (m->label()) SAFE_STRCAT(m->label());
			if ( m == finditem ) return(0);		// found? done.
		} else {
			if (m->label()) {				// menu item?
				if ( m == finditem ) {			// found? tack on itemname, done.
					SAFE_STRCAT("/");
					SAFE_STRCAT(m->label());
					return(0);
				}
			} else {					// end of submenu? pop
				char *ss = strrchr(name, '/');
				if ( ss ) {
					*ss = 0;        // "File/Edit" -> "File"
					len = (int)strlen(name);
				} else {
					name[0] = '\0';        // "File" -> ""
					len = 0;
				}
				continue;
			}
		}
	}
	*name = '\0';
	return(-1);						// item not found
}

/**
 Find the menu item for a given menu \p pathname, such as "Edit/Copy".

 This method finds a menu item in the menu array, also traversing submenus, but
 not submenu pointers.

 To get the menu item's index, use find_index(const char*)

  \b Example:
  \code
    fltk3::MenuBar *menubar = new fltk3::MenuBar(..);
    menubar->add("File/&Open");
    menubar->add("File/&Save");
    menubar->add("Edit/&Copy");
    // [..]
    fltk3::MenuItem *item;
    if ( ( item = (fltk3::MenuItem*)menubar->find_item("File/&Open") ) != NULL ) {
	item->labelcolor(fltk3::RED);
    }
    if ( ( item = (fltk3::MenuItem*)menubar->find_item("Edit/&Copy") ) != NULL ) {
	item->labelcolor(fltk3::GREEN);
    }
  \endcode

  \param pathname The path and name of the menu item
  \returns The item found, or NULL if not found
  \see find_index(const char*), find_item(fltk3::Callback*), item_pathname()
*/
const fltk3::MenuItem * fltk3::Menu_::find_item(const char *pathname)
{
	int i = find_index(pathname);
	return( (i==-1) ? 0 : (const fltk3::MenuItem*)(menu_+i));
}

/**
 Find the index the menu array for given \p item.

 A way to convert a menu item pointer into an index.

 Current implementation is fast and not expensive.

 \code
   // Convert an index-to-item
   int index = 12;
   const fltk3::MenuItem *item = mymenu->menu() + index;

   // Convert an item-to-index
   int index = mymenu->find_index(item);
   if ( index == -1 ) { ..error.. }
 \endcode

 \param item The *item to be found
 \returns    The index of the item, or -1 if not found.
 \see        menu()
*/
int fltk3::Menu_::find_index(const fltk3::MenuItem *item) const
{
	fltk3::MenuItem *max = menu_+size();
	if (item<menu_ || item>=max) return(-1);
	return (int)(item-menu_);
}

/**
 Find the index into the menu array for a given callback \p cb.

 This method finds a menu item's index position, also traversing submenus, but
 not submenu pointers. This is useful if an application uses internationalisation
 and a menu item can not be found using its label. This search is also much faster.

 \param cb Find the first item with this callback
 \returns  The index of the item with the specific callback, or -1 if not found
 \see      find_index(const char*)
 */
int fltk3::Menu_::find_index(fltk3::Callback *cb) const
{
	for ( int t=0; t < size(); t++ )
		if (menu_[t].callback_==cb)
			return(t);
	return(-1);
}

/**
 Find the menu item index for a given menu \p pathname, such as "Edit/Copy".

 This method finds a menu item's index position for the given menu pathname,
 also traversing submenus, but not submenu pointers.

 To get the menu item pointer for a pathname, use find_item()

 \param pathname The path and name of the menu item index to find
 \returns        The index of the matching item, or -1 if not found.
 \see            item_pathname()

*/
int fltk3::Menu_::find_index(const char *pathname) const
{
	char menupath[1024] = "";	// File/Export
	for ( int t=0; t < size(); t++ ) {
		fltk3::MenuItem *m = menu_ + t;
		if (m->flags&fltk3::SUBMENU) {
			// IT'S A SUBMENU
			// we do not support searches through fltk3::SUBMENU_POINTER links
			if (menupath[0]) strlcat(menupath, "/", sizeof(menupath));
			strlcat(menupath, m->label(), sizeof(menupath));
			if (!strcmp(menupath, pathname)) return(t);
		} else {
			if (!m->label()) {
				// END OF SUBMENU? Pop back one level.
				char *ss = strrchr(menupath, '/');
				if ( ss ) *ss = 0;
				else menupath[0] = '\0';
				continue;
			}
			// IT'S A MENU ITEM
			char itempath[1024];	// eg. Edit/Copy
			strcpy(itempath, menupath);
			if (itempath[0]) strlcat(itempath, "/", sizeof(itempath));
			strlcat(itempath, m->label(), sizeof(itempath));
			if (!strcmp(itempath, pathname)) return(t);
		}
	}
	return(-1);
}

/**
 Find the menu item for the given callback \p cb.

 This method finds a menu item in a menu array, also traversing submenus, but
 not submenu pointers. This is useful if an application uses
 internationalisation and a menu item can not be found using its label. This
 search is also much faster.

 \param cb find the first item with this callback
 \returns The item found, or NULL if not found
 \see find_item(const char*)
 */
const fltk3::MenuItem * fltk3::Menu_::find_item(fltk3::Callback *cb)
{
	for ( int t=0; t < size(); t++ ) {
		const fltk3::MenuItem *m = menu_ + t;
		if (m->callback_==cb) {
			return m;
		}
	}
	return (const fltk3::MenuItem *)0;
}

/**
  The value is the index into menu() of the last item chosen by
  the user.  It is zero initially.  You can set it as an integer, or set
  it with a pointer to a menu item.  The set routines return non-zero if
  the new value is different than the old one.
*/
int fltk3::Menu_::value(const fltk3::MenuItem* m)
{
	clear_changed();
	if (value_ != m) {
		value_ = m;
		return 1;
	}
	return 0;
}

/**
 When user picks a menu item, call this.  It will do the callback.
 Unfortunately this also casts away const for the checkboxes, but this
 was necessary so non-checkbox menus can really be declared const...
*/
const fltk3::MenuItem* fltk3::Menu_::picked(const fltk3::MenuItem* v)
{
	if (v) {
		if (v->radio()) {
			if (!v->value()) { // they are turning on a radio item
				set_changed();
				((fltk3::MenuItem*)v)->setonly();
			}
			redraw();
		} else if (v->flags & fltk3::MENU_TOGGLE) {
			set_changed();
			((fltk3::MenuItem*)v)->flags ^= fltk3::MENU_VALUE;
			redraw();
		} else if (v != value_) { // normal item
			set_changed();
		}
		value_ = v;
		if (when()&(fltk3::WHEN_CHANGED|fltk3::WHEN_RELEASE)) {
			if (changed() || when()&fltk3::WHEN_NOT_CHANGED) {
				if (value_ && value_->callback_) value_->do_callback((fltk3::Widget*)this);
				else do_callback();
			}
		}
	}
	return v;
}

/** Turns the radio item "on" for the menu item and turns off adjacent radio items set. */
void fltk3::MenuItem::setonly()
{
	flags |= fltk3::MENU_RADIO | fltk3::MENU_VALUE;
	fltk3::MenuItem* j;
	for (j = this; ; ) {	// go down
		if (j->flags & fltk3::MENU_DIVIDER) break; // stop on divider lines
		j++;
		if (!j->text || !j->radio()) break; // stop after group
		j->clear();
	}
	for (j = this-1; ; j--) { // go up
		if (!j->text || (j->flags&fltk3::MENU_DIVIDER) || !j->radio()) break;
		j->clear();
	}
}

/**
 Creates a new fltk3::Menu_ widget using the given position, size,
 and label string.  menu() is initialized to null.
 */
fltk3::Menu_::Menu_(int X,int Y,int W,int H,const char* l)
	: fltk3::Widget(X,Y,W,H,l)
{
	set_flag(SHORTCUT_LABEL);
	//box(fltk3::UP_BOX);
	box(fltk3::NO_BOX);
	when(fltk3::WHEN_RELEASE_ALWAYS);
	value_ = menu_ = 0;
	alloc = 0;
	selection_color(fltk3::SELECTION_COLOR);
	textfont(fltk3::HELVETICA);
	textsize(fltk3::NORMAL_SIZE);
	textcolor(fltk3::FOREGROUND_COLOR);
	down_box(fltk3::NO_BOX);
}

/**
  This returns the number of fltk3::MenuItem structures that make up the
  menu, correctly counting submenus.  This includes the "terminator"
  item at the end.  To copy a menu array you need to copy
  size()*sizeof(fltk3::MenuItem) bytes.  If the menu is
  NULL this returns zero (an empty menu will return 1).
*/
int fltk3::Menu_::size() const
{
	if (!menu_) return 0;
	return menu_->size();
}

/**
    Sets the menu array pointer directly.  If the old menu is private it is
    deleted.  NULL is allowed and acts the same as a zero-length
    menu.  If you try to modify the array (with add(), replace(), or
    remove()) a private copy is automatically done.
*/
void fltk3::Menu_::menu(const fltk3::MenuItem* m)
{
	clear();
	value_ = menu_ = (fltk3::MenuItem*)m;
}

// this version is ok with new Fl_Menu_add code with fltk3::menu_array_owner:

/**
  Sets the menu array pointer with a copy of m that will be automatically deleted.
  If userdata \p ud is not NULL, then all user data pointers are changed in the menus as well.
  See void fltk3::Menu_::menu(const fltk3::MenuItem* m).
*/
void fltk3::Menu_::copy(const fltk3::MenuItem* m, void* ud)
{
	int n = m->size();
	fltk3::MenuItem* newMenu = new fltk3::MenuItem[n];
	memcpy(newMenu, m, n*sizeof(fltk3::MenuItem));
	menu(newMenu);
	alloc = 1; // make destructor free array, but not strings
	// for convenience, provide way to change all the user data pointers:
	if (ud) for (; n--;) {
			if (newMenu->callback_) newMenu->user_data_ = ud;
			newMenu++;
		}
}

fltk3::Menu_::~Menu_()
{
	clear();
}

// Fl_Menu::add() uses this to indicate the owner of the dynamically-
// expanding array.  We must not free this array:
namespace fltk3
{
fltk3::Menu_* menu_array_owner = 0;
}

/**
  Same as menu(NULL), set the array pointer to null, indicating
  a zero-length menu.

  Menus must not be cleared during a callback to the same menu.
*/
void fltk3::Menu_::clear()
{
	if (alloc) {
		if (alloc>1) for (int i = size(); i--;)
				if (menu_[i].text) free((void*)menu_[i].text);
		if (this == fltk3::menu_array_owner)
			fltk3::menu_array_owner = 0;
		else
			delete[] menu_;
		menu_ = 0;
		value_ = 0;
		alloc = 0;
	}
}

/**
 Clears the specified submenu pointed to by \p index of all menu items.

 This method is useful for clearing a submenu so that it can be
 re-populated with new items. Example: a "File/Recent Files/..." submenu
 that shows the last few files that have been opened.

 The specified \p index must point to a submenu.

 The submenu is cleared with remove().
 If the menu array was directly set with menu(x), then copy()
 is done to make a private array.

 \warning Since this method can change the internal menu array, any menu
 item pointers or indecies the application may have cached can become
 stale, and should be recalculated/refreshed.

 \b Example:
 \code
   int index = menubar->find_index("File/Recent");    // get index of "File/Recent" submenu
   if ( index != -1 ) menubar->clear_submenu(index);  // clear the submenu
   menubar->add("File/Recent/Aaa");
   menubar->add("File/Recent/Bbb");
   [..]
 \endcode

 \param index The index of the submenu to be cleared
 \returns 0 on success, -1 if the index is out of range or not a submenu
 \see remove(int)
 */
int fltk3::Menu_::clear_submenu(int index)
{
	if ( index < 0 || index >= size() ) return(-1);
	if ( ! (menu_[index].flags & fltk3::SUBMENU) ) return(-1);
	++index;					// advance to first item in submenu
	while ( index < size() ) {                    // keep remove()ing top item until end is reached
		if ( menu_[index].text == 0 ) break;	// end of this submenu? done
		remove(index);				// remove items/submenus
	}
	return(0);
}

//
// End of "$Id: Menu_.cxx 9330 2012-04-09 08:53:49Z manolo $".
//
