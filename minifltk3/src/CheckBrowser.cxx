//
// "$Id: CheckBrowser.cxx 9046 2011-09-17 23:05:18Z matt $"
//
// fltk3::CheckBrowser header file for the Fast Light Tool Kit (FLTK).
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

#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#include "draw.h"
#include "CheckBrowser.h"

/* This uses a cache for faster access when you're scanning the list
 either forwards or backwards. */

fltk3::CheckBrowser::cb_item *fltk3::CheckBrowser::find_item(int n) const
{
	int i = n;
	cb_item *p = first;

	if (n <= 0 || n > nitems_ || p == 0) {
		return 0;
	}

	if (n == cached_item) {
		p = cache;
		n = 1;
	} else if (n == cached_item + 1) {
		p = cache->next;
		n = 1;
	} else if (n == cached_item - 1) {
		p = cache->prev;
		n = 1;
	}

	while (--n) {
		p = p->next;
	}

	/* Cast to not const and cache it. */

	((fltk3::CheckBrowser *)this)->cache = p;
	((fltk3::CheckBrowser *)this)->cached_item = i;

	return p;
}

int fltk3::CheckBrowser::lineno(cb_item *p0) const
{
	cb_item *p = first;

	if (p == 0) {
		return 0;
	}

	int i = 1;
	while (p) {
		if (p == p0) {
			return i;
		}
		i++;
		p = p->next;
	}

	return 0;
}

fltk3::CheckBrowser::CheckBrowser(int X, int Y, int W, int H, const char *l)
/**  The constructor makes an empty browser.*/
	: fltk3::Browser_(X, Y, W, H, l)
{
	type(fltk3::SELECT_BROWSER);
	when(fltk3::WHEN_NEVER);
	first = last = 0;
	nitems_ = nchecked_ = 0;
	cached_item = -1;
}

void *fltk3::CheckBrowser::item_first() const
{
	return first;
}

void *fltk3::CheckBrowser::item_next(void *l) const
{
	return ((cb_item *)l)->next;
}

void *fltk3::CheckBrowser::item_prev(void *l) const
{
	return ((cb_item *)l)->prev;
}

int fltk3::CheckBrowser::item_height(void *) const
{
	return textsize() + 2;
}

#define CHECK_SIZE (textsize()-2)

int fltk3::CheckBrowser::item_width(void *v) const
{
	fltk3::font(textfont(), textsize());
	return int(fltk3::width(((cb_item *)v)->text)) + CHECK_SIZE + 8;
}

void fltk3::CheckBrowser::item_draw(void *v, int X, int Y, int, int) const
{
	cb_item *i = (cb_item *)v;
	char *s = i->text;
	int tsize = textsize();
	fltk3::Color col = active_r() ? textcolor() : fltk3::inactive(textcolor());
	int cy = Y + (tsize + 1 - CHECK_SIZE) / 2;
	X += 2;

	fltk3::color(active_r() ? fltk3::FOREGROUND_COLOR : fltk3::inactive(fltk3::FOREGROUND_COLOR));
	fltk3::loop(X, cy, X, cy + CHECK_SIZE,
	            X + CHECK_SIZE, cy + CHECK_SIZE, X + CHECK_SIZE, cy);
	if (i->checked) {
		int tx = X + 3;
		int tw = CHECK_SIZE - 4;
		int d1 = tw/3;
		int d2 = tw-d1;
		int ty = cy + (CHECK_SIZE+d2)/2-d1-2;
		for (int n = 0; n < 3; n++, ty++) {
			fltk3::line(tx, ty, tx+d1, ty+d1);
			fltk3::line(tx+d1, ty+d1, tx+tw-1, ty+d1-d2+1);
		}
	}
	fltk3::font(textfont(), tsize);
	if (i->selected) {
		col = fltk3::contrast(col, selection_color());
	}
	fltk3::color(col);
	fltk3::draw(s, X + CHECK_SIZE + 8, Y + tsize - 1);
}

void fltk3::CheckBrowser::item_select(void *v, int state)
{
	cb_item *i = (cb_item *)v;

	if (state) {
		if (i->checked) {
			i->checked = 0;
			nchecked_--;
		} else {
			i->checked = 1;
			nchecked_++;
		}
	}
}

int fltk3::CheckBrowser::item_selected(void *v) const
{
	cb_item *i = (cb_item *)v;
	return i->selected;
}
/**
 Add a new unchecked line to the end of the browser.
 \see add(char *s, int b)
 */
int fltk3::CheckBrowser::add(char *s)
{
	return (add(s, 0));
}

/**
 Add a new line to the end of the browser.  The text is copied
 using the strdup() function.  It may also be NULL to make
 a blank line.  It can set the item checked if \p b is not 0.
 */
int fltk3::CheckBrowser::add(char *s, int b)
{
	cb_item *p = (cb_item *)malloc(sizeof(cb_item));
	p->next = 0;
	p->prev = 0;
	p->checked = b;
	p->selected = 0;
	p->text = strdup(s);

	if (b) {
		nchecked_++;
	}

	if (last == 0) {
		first = last = p;
	} else {
		last->next = p;
		p->prev = last;
		last = p;
	}
	nitems_++;

	return (nitems_);
}

/**
 Remove line n and make the browser one line shorter. Returns the
 number of lines left in the browser.
 */
int fltk3::CheckBrowser::remove(int item)
{
	cb_item *p = find_item(item);

	// line at item exists
	if(p) {
		// tell the Browser_ what we will do
		deleting(p);

		// fix checked count
		if(p->checked)
			--nchecked_;

		// remove the node
		if (p->prev)
			p->prev->next = p->next;
		else
			first = p->next;
		if (p->next)
			p->next->prev = p->prev;
		else
			last = p->prev;

		free(p->text);
		free(p);

		--nitems_;
		cached_item = -1;
	}

	return (nitems_);
}

/**  Remove every item from the browser.*/
void fltk3::CheckBrowser::clear()
{
	cb_item *p = first;
	cb_item *next;

	if (p == 0) {
		return;
	}

	new_list();
	do {
		next = p->next;
		free(p->text);
		free(p);
		p = next;
	} while (p);

	first = last = 0;
	nitems_ = nchecked_ = 0;
	cached_item = -1;
}

/** Gets the current status of item item. */
int fltk3::CheckBrowser::checked(int i) const
{
	cb_item *p = find_item(i);

	if (p) return p->checked;
	return 0;
}

/** Sets the check status of item item to b. */
void fltk3::CheckBrowser::checked(int i, int b)
{
	cb_item *p = find_item(i);

	if (p && (p->checked ^ b)) {
		p->checked = b;
		if (b) {
			nchecked_++;
		} else {
			nchecked_--;
		}
		redraw();
	}
}

/**  Returns the index of the currently selected item.*/
int fltk3::CheckBrowser::value() const
{
	return lineno((cb_item *)selection());
}

/**  Return a pointer to an internal buffer holding item item's text.*/
char *fltk3::CheckBrowser::text(int i) const
{
	cb_item *p = find_item(i);

	if (p) return p->text;
	return 0;
}

/**  Sets all the items checked.*/
void fltk3::CheckBrowser::check_all()
{
	cb_item *p;

	nchecked_ = nitems_;
	for (p = first; p; p = p->next) {
		p->checked = 1;
	}
	redraw();
}

/**  Sets all the items unchecked.*/
void fltk3::CheckBrowser::check_none()
{
	cb_item *p;

	nchecked_ = 0;
	for (p = first; p; p = p->next) {
		p->checked = 0;
	}
	redraw();
}

int fltk3::CheckBrowser::handle(int event)
{
	if (event==fltk3::PUSH)
		deselect();
	return Browser_::handle(event);
}

//
// End of "$Id: CheckBrowser.cxx 9046 2011-09-17 23:05:18Z matt $".
//
