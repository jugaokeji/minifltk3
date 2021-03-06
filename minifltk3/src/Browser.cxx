//
// "$Id: Browser.cxx 9638 2012-07-24 04:41:38Z matt $"
//
// Browser widget for the Fast Light Tool Kit (FLTK).
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

#include "run.h"
#include "Browser.h"
#include "draw.h"
#include "flstring.h"
#include <stdlib.h>
#include <math.h>

#include "HoldBrowser.h"
#include "MultiBrowser.h"
#include "SelectBrowser.h"


// I modified this from the original Forms data to use a linked list
// so that the number of items in the browser and size of those items
// is unlimited. The only problem is that the old browser used an
// index number to identify a line, and it is slow to convert from/to
// a pointer. I use a cache of the last match to try to speed this up.

// Also added the ability to "hide" a line. This sets its height to
// zero, so the fltk3::Browser_ cannot pick it.

#define SELECTED 1
#define NOTDISPLAYED 2

namespace fltk3
{

// WARNING:
//       fltk3::FileChooser.cxx also has a definition of this structure (fltk3::BrowserLine_).
//       Changes to fltk3::BrowserLine_ *must* be reflected in fltk3::FileChooser.cxx as well.
//       This hack in fltk3::FileChooser should be solved.
//
struct BrowserLine_ {	// data is in a linked list of these
	fltk3::BrowserLine_* prev;
	fltk3::BrowserLine_* next;
	void* data;
	fltk3::Image* icon;
	short length;		// sizeof(txt)-1, may be longer than string
	char flags;		// selected, displayed
	char txt[1];		// start of allocated array
};

}


void fltk3::Browser::item_swap(void *a, void *b)
{
	//FLTK3_WIDGET_VCALLS_WRAPPER(item_swap(a, b), BrowserItemSwap)
	swap((fltk3::BrowserLine_*)a, (fltk3::BrowserLine_*)b);
}

void *fltk3::Browser::item_at(int line) const
{
	//FLTK3_WIDGET_VCALLS_WRAPPER_RET(void*, item_at(line), BrowserItemAt)
	return (void*)find_line(line);
}

/**
  Returns the very first item in the list.
  Example of use:
  \code
  // Walk the browser from beginning to end
  for ( void *i=item_first(); i; i=item_next(i) ) {
      printf("item label='%s'\n", item_text(i));
  }
  \endcode
  \returns The first item, or NULL if list is empty.
  \see item_first(), item_last(), item_next(), item_prev()
*/
void* fltk3::Browser::item_first() const
{
	//FLTK3_WIDGET_VCALLS_WRAPPER_RET(void*, item_first(), BrowserItemFirst)
	return first;
}

/**
  Returns the next item after \p item.
  \param[in] item The 'current' item
  \returns The next item after \p item, or NULL if there are none after this one.
  \see item_first(), item_last(), item_next(), item_prev()
*/
void* fltk3::Browser::item_next(void* item) const
{
	//FLTK3_WIDGET_VCALLS_WRAPPER_RET(void*, item_next(item), BrowserItemNext)
	return ((fltk3::BrowserLine_*)item)->next;
}

/**
  Returns the previous item before \p item.
  \param[in] item The 'current' item
  \returns The previous item before \p item, or NULL if there none before this one.
  \see item_first(), item_last(), item_next(), item_prev()
*/
void* fltk3::Browser::item_prev(void* item) const
{
	//FLTK3_WIDGET_VCALLS_WRAPPER_RET(void*, item_prev(item), BrowserItemPrev)
	return ((fltk3::BrowserLine_*)item)->prev;
}

/**
  Returns the very last item in the list.
  Example of use:
  \code
  // Walk the browser in reverse, from end to start
  for ( void *i=item_last(); i; i=item_prev(i) ) {
      printf("item label='%s'\n", item_text(i));
  }
  \endcode
  \returns The last item, or NULL if list is empty.
  \see item_first(), item_last(), item_next(), item_prev()
*/
void* fltk3::Browser::item_last() const
{
	//FLTK3_WIDGET_VCALLS_WRAPPER_RET(void*, item_last(), BrowserItemLast)
	return last;
}

/**
  See if \p item is selected.
  \param[in] item The item whose selection state is to be checked.
  \returns 1 if selected, 0 if not.
  \see select(), selected(), value(), item_select(), item_selected()
*/
int fltk3::Browser::item_selected(void* item) const
{
	//FLTK3_WIDGET_VCALLS_WRAPPER_RET(int, item_selected(item), BrowserItemSelected)
	return ((fltk3::BrowserLine_*)item)->flags&SELECTED;
}
/**
  Change the selection state of \p item to the value \p val.
  \param[in] item The item to be changed.
  \param[in] val The new selection state: 1 selects, 0 de-selects.
  \see select(), selected(), value(), item_select(), item_selected()
*/
void fltk3::Browser::item_select(void *item, int val)
{
	//FLTK3_WIDGET_VCALLS_WRAPPER(item_select(item, val), BrowserItemSelect)
	if (val) ((fltk3::BrowserLine_*)item)->flags |= SELECTED;
	else     ((fltk3::BrowserLine_*)item)->flags &= ~SELECTED;
}

/**
  Returns the label text for \p item.
  \param[in] item The item whose label text is returned.
  \returns The item's text string. (Can be NULL)
*/
const char *fltk3::Browser::item_text(void *item) const
{
	//FLTK3_WIDGET_VCALLS_WRAPPER_RET(const char *, item_text(item), BrowserItemText)
	return ((fltk3::BrowserLine_*)item)->txt;
}

/**
  Returns the item for specified \p line.

  Note: This call is slow. It's fine for e.g. responding to user
  clicks, but slow if called often, such as in a tight sorting loop.
  Finding an item 'by line' involves a linear lookup on the internal
  linked list. The performance hit can be significant if the browser's
  contents is large, and the method is called often (e.g. during a sort).
  If you're writing a subclass, use the protected methods item_first(),
  item_next(), etc. to access the internal linked list more efficiently.

  \param[in] line The line number of the item to return. (1 based)
  \retval item that was found.
  \retval NULL if line is out of range.
  \see item_at(), find_line(), lineno()
*/
fltk3::BrowserLine_* fltk3::Browser::find_line(int line) const
{
	int n;
	fltk3::BrowserLine_* l;
	if (line == cacheline) return cache;
	if (cacheline && line > (cacheline/2) && line < ((cacheline+lines)/2)) {
		n = cacheline;
		l = cache;
	} else if (line <= (lines/2)) {
		n = 1;
		l = first;
	} else {
		n = lines;
		l = last;
	}
	for (; n < line && l; n++) l = l->next;
	for (; n > line && l; n--) l = l->prev;
	((fltk3::Browser*)this)->cacheline = line;
	((fltk3::Browser*)this)->cache = l;
	return l;
}

/**
  Returns line number corresponding to \p item, or zero if not found.
  Caveat: See efficiency note in find_line().
  \param[in] item The item to be found
  \returns The line number of the item, or 0 if not found.
  \see item_at(), find_line(), lineno()
*/
int fltk3::Browser::lineno(void *item) const
{
	fltk3::BrowserLine_* l = (fltk3::BrowserLine_*)item;
	if (!l) return 0;
	if (l == cache) return cacheline;
	if (l == first) return 1;
	if (l == last) return lines;
	if (!cache) {
		((fltk3::Browser*)this)->cache = first;
		((fltk3::Browser*)this)->cacheline = 1;
	}
	// assume it is near cache, search both directions:
	fltk3::BrowserLine_* b = cache->prev;
	int bnum = cacheline-1;
	fltk3::BrowserLine_* f = cache->next;
	int fnum = cacheline+1;
	int n = 0;
	for (;;) {
		if (b == l) {
			n = bnum;
			break;
		}
		if (f == l) {
			n = fnum;
			break;
		}
		if (b) {
			b = b->prev;
			bnum--;
		}
		if (f) {
			f = f->next;
			fnum++;
		}
	}
	((fltk3::Browser*)this)->cache = l;
	((fltk3::Browser*)this)->cacheline = n;
	return n;
}

/**
  Removes the item at the specified \p line.
  Caveat: See efficiency note in find_line().
  You must call redraw() to make any changes visible.
  \param[in] line The line number to be removed. (1 based) Must be in range!
  \returns Pointer to browser item that was removed (and is no longer valid).
  \see add(), insert(), remove(), swap(int,int), clear()
*/
fltk3::BrowserLine_* fltk3::Browser::_remove(int line)
{
	fltk3::BrowserLine_* ttt = find_line(line);
	deleting(ttt);

	cacheline = line-1;
	cache = ttt->prev;
	lines--;
	full_height_ -= item_height(ttt);
	if (ttt->prev) ttt->prev->next = ttt->next;
	else first = ttt->next;
	if (ttt->next) ttt->next->prev = ttt->prev;
	else last = ttt->prev;

	return(ttt);
}

/**
  Remove entry for given \p line number, making the browser one line shorter.
  You must call redraw() to make any changes visible.
  \param[in] line Line to be removed. (1 based) \n
                  If \p line is out of range, no action is taken.
  \see add(), insert(), remove(), swap(int,int), clear()
*/
void fltk3::Browser::remove(int line)
{
	if (line < 1 || line > lines) return;
	free(_remove(line));
}

/**
  Insert specified \p item above \p line.
  If \p line > size() then the line is added to the end.

  Caveat: See efficiency note in find_line().

  \param[in] line  The new line will be inserted above this line (1 based).
  \param[in] item  The item to be added.
*/
void fltk3::Browser::insert(int line, fltk3::BrowserLine_* item)
{
	if (!first) {
		item->prev = item->next = 0;
		first = last = item;
	} else if (line <= 1) {
		inserting(first, item);
		item->prev = 0;
		item->next = first;
		item->next->prev = item;
		first = item;
	} else if (line > lines) {
		item->prev = last;
		item->prev->next = item;
		item->next = 0;
		last = item;
	} else {
		fltk3::BrowserLine_* n = find_line(line);
		inserting(n, item);
		item->next = n;
		item->prev = n->prev;
		item->prev->next = item;
		n->prev = item;
	}
	cacheline = line;
	cache = item;
	lines++;
	full_height_ += item_height(item);
	redraw_line(item);
}

/**
  Insert a new entry whose label is \p newtext \e above given \p line, optional data \p d.

  Text may contain format characters; see format_char() for details.
  \p newtext is copied using the strdup() function, and can be NULL to make a blank line.

  The optional void * argument \p d will be the data() of the new item.

  \param[in] line Line position for insert. (1 based) \n
             If \p line > size(), the entry will be added at the end.
  \param[in] newtext The label text for the new line.
  \param[in] d Optional pointer to user data to be associated with the new line.
*/
void fltk3::Browser::insert(int line, const char* newtext, void* d)
{
	int l = (int)strlen(newtext);
	fltk3::BrowserLine_* t = (fltk3::BrowserLine_*)malloc(sizeof(fltk3::BrowserLine_)+l);
	t->length = (short)l;
	t->flags = 0;
	strcpy(t->txt, newtext);
	t->data = d;
	t->icon = 0;
	insert(line, t);
}

/**
  Line \p from is removed and reinserted at \p to.
  Note: \p to is calculated \e after line \p from gets removed.
  \param[in] to Destination line number (calculated \e after line \p from is removed)
  \param[in] from Line number of item to be moved
*/
void fltk3::Browser::move(int to, int from)
{
	if (from < 1 || from > lines) return;
	insert(to, _remove(from));
}

/**
  Sets the text for the specified \p line to \p newtext.

  Text may contain format characters; see format_char() for details.
  \p newtext is copied using the strdup() function, and can be NULL to make a blank line.

  Does nothing if \p line is out of range.

  \param[in] line The line of the item whose text will be changed. (1 based)
  \param[in] newtext The new string to be assigned to the item.
*/
void fltk3::Browser::text(int line, const char* newtext)
{
	if (line < 1 || line > lines) return;
	fltk3::BrowserLine_* t = find_line(line);
	int l = (int)strlen(newtext);
	if (l > t->length) {
		fltk3::BrowserLine_* n = (fltk3::BrowserLine_*)malloc(sizeof(fltk3::BrowserLine_)+l);
		replacing(t, n);
		cache = n;
		n->data = t->data;
		n->icon = t->icon;
		n->length = (short)l;
		n->flags = t->flags;
		n->prev = t->prev;
		if (n->prev) n->prev->next = n;
		else first = n;
		n->next = t->next;
		if (n->next) n->next->prev = n;
		else last = n;
		free(t);
		t = n;
	}
	strcpy(t->txt, newtext);
	redraw_line(t);
}

/**
  Sets the user data for specified \p line to \p d.
  Does nothing if \p line is out of range.
  \param[in] line The line of the item whose data() is to be changed. (1 based)
  \param[in] d The new data to be assigned to the item. (can be NULL)
*/
void fltk3::Browser::data(int line, void* d)
{
	if (line < 1 || line > lines) return;
	find_line(line)->data = d;
}

/**
  Returns height of \p item in pixels.
  This takes into account embedded \@ codes within the text() label.
  \param[in] item The item whose height is returned.
  \returns The height of the item in pixels.
  \see item_height(), item_width(),\n
       incr_height(), full_height()
*/
int fltk3::Browser::item_height(void *item) const
{
	//FLTK3_WIDGET_VCALLS_WRAPPER_RET(int, item_height(item), BrowserItemHeight)
	fltk3::BrowserLine_* l = (fltk3::BrowserLine_*)item;
	if (l->flags & NOTDISPLAYED) return 0;

	int hmax = 2; // use 2 to insure we don't return a zero!

	if (!l->txt[0]) {
		// For blank lines set the height to exactly 1 line!
		fltk3::font(textfont(), textsize());
		int hh = fltk3::height();
		if (hh > hmax) hmax = hh;
	} else {
		const int* i = column_widths();
		// do each column separately as they may all set different fonts:
		for (char* str = l->txt; str && *str; str++) {
			fltk3::Font font = textfont(); // default font
			int tsize = textsize(); // default size
			while (*str==format_char()) {
				str++;
				switch (*str++) {
				case 'l':
				case 'L':
					tsize = 24;
					break;
				case 'm':
				case 'M':
					tsize = 18;
					break;
				case 's':
					tsize = 11;
					break;
				case 'b':
					font = (fltk3::Font)(font|fltk3::BOLD);
					break;
				case 'i':
					font = (fltk3::Font)(font|fltk3::ITALIC);
					break;
				case 'f':
				case 't':
					font = fltk3::COURIER;
					break;
				case 'B':
				case 'C':
					while (isdigit(*str & 255)) str++;
					break;// skip a color number
				case 'F':
					font = (fltk3::Font)strtol(str,&str,10);
					break;
				case 'S':
					tsize = strtol(str,&str,10);
					break;
				case 0:
				case '@':
					str--;
				case '.':
					goto END_FORMAT;
				}
			}
END_FORMAT:
			char* ptr = str;
			if (ptr && *i++) str = strchr(str, column_char());
			else str = NULL;
			if((!str && *ptr) || (str && ptr < str)) {
				fltk3::font(font, tsize);
				int hh = fltk3::height();
				if (hh > hmax) hmax = hh;
			}
			if (!str || !*str) break;
		}
	}

	if (l->icon && (l->icon->h()+2)>hmax) {
		hmax = l->icon->h() + 2;	// leave 2px above/below
	}
	return hmax; // previous version returned hmax+2!
}

/**
  Returns width of \p item in pixels.
  This takes into account embedded \@ codes within the text() label.
  \param[in] item The item whose width is returned.
  \returns The width of the item in pixels.
  \see item_height(), item_width(),\n
       incr_height(), full_height()
*/
int fltk3::Browser::item_width(void *item) const
{
	//FLTK3_WIDGET_VCALLS_WRAPPER_RET(int, item_width(item), BrowserItemWidth)
	fltk3::BrowserLine_* l=(fltk3::BrowserLine_*)item;
	char* str = l->txt;
	const int* i = column_widths();
	int ww = 0;

	while (*i) { // add up all tab-separated fields
		char* e;
		e = strchr(str, column_char());
		if (!e) break; // last one occupied by text
		str = e+1;
		ww += *i++;
	}

	// OK, we gotta parse the string and find the string width...
	int tsize = textsize();
	fltk3::Font font = textfont();
	int done = 0;

	while (*str == format_char_ && str[1] && str[1] != format_char_) {
		str ++;
		switch (*str++) {
		case 'l':
		case 'L':
			tsize = 24;
			break;
		case 'm':
		case 'M':
			tsize = 18;
			break;
		case 's':
			tsize = 11;
			break;
		case 'b':
			font = (fltk3::Font)(font|fltk3::BOLD);
			break;
		case 'i':
			font = (fltk3::Font)(font|fltk3::ITALIC);
			break;
		case 'f':
		case 't':
			font = fltk3::COURIER;
			break;
		case 'B':
		case 'C':
			while (isdigit(*str & 255)) str++;
			break;// skip a color number
		case 'F':
			font = (fltk3::Font)strtol(str, &str, 10);
			break;
		case 'S':
			tsize = strtol(str, &str, 10);
			break;
		case '.':
			done = 1;
			break;
		case '@':
			str--;
			done = 1;
		}

		if (done)
			break;
	}

	if (*str == format_char_ && str[1])
		str ++;

	if (ww==0 && l->icon) ww = l->icon->w();

	fltk3::font(font, tsize);
	return ww + int(fltk3::width(str)) + 6;
}

/**
  The height of the entire list of all visible() items in pixels.
  This returns the accumulated height of *all* the items in the browser
  that are not hidden with hide(), including items scrolled off screen.
  \returns The accumulated size of all the visible items in pixels.
  \see item_height(), item_width(),\n
       incr_height(), full_height()
*/
int fltk3::Browser::full_height() const
{
	return full_height_;
}

/**
  The default 'average' item height (including inter-item spacing) in pixels.
  This currently returns textsize() + 2.
  \returns The value in pixels.
  \see item_height(), item_width(),\n
       incr_height(), full_height()
*/
int fltk3::Browser::incr_height() const
{
	return textsize()+2;
}

/**
  Draws \p item at the position specified by \p X \p Y \p W \p H.
  The \p W and \p H values are used for clipping.
  Should only be called within the context of an FLTK draw().
  \param[in] item The item to be drawn
  \param[in] X,Y,W,H position and size.
*/
void fltk3::Browser::item_draw(void* item, int X, int Y, int W, int H) const
{
	//FLTK3_WIDGET_VCALLS_WRAPPER(item_draw(item, X, Y, W, H), BrowserItemDraw)
	fltk3::BrowserLine_* l = (fltk3::BrowserLine_*)item;
	char* str = l->txt;
	const int* i = column_widths();

	bool first = true;	// for icon
	while (W > 6) {	// do each tab-separated field
		int w1 = W;	// width for this field
		char* e = 0; // pointer to end of field or null if none
		if (*i) { // find end of field and temporarily replace with 0
			e = strchr(str, column_char());
			if (e) {
				*e = 0;
				w1 = *i++;
			}
		}
		// Icon drawing code
		if (first) {
			first = false;
			if (l->icon) {
				l->icon->draw(X+2,Y+1);	// leave 2px left, 1px above
				int iconw = l->icon->w()+2;
				X += iconw;
				W -= iconw;
				w1 -= iconw;
			}
		}
		int tsize = textsize();
		fltk3::Font font = textfont();
		fltk3::Color lcol = textcolor();
		fltk3::Align talign = fltk3::ALIGN_LEFT;
		// check for all the @-lines recognized by XForms:
		//#if defined(__GNUC__)
		//#warning FIXME This maybe needs to be more UTF8 aware now...?
		//#endif /*__GNUC__*/
		while (*str == format_char() && *++str && *str != format_char()) {
			switch (*str++) {
			case 'l':
			case 'L':
				tsize = 24;
				break;
			case 'm':
			case 'M':
				tsize = 18;
				break;
			case 's':
				tsize = 11;
				break;
			case 'b':
				font = (fltk3::Font)(font|fltk3::BOLD);
				break;
			case 'i':
				font = (fltk3::Font)(font|fltk3::ITALIC);
				break;
			case 'f':
			case 't':
				font = fltk3::COURIER;
				break;
			case 'c':
				talign = fltk3::ALIGN_CENTER;
				break;
			case 'r':
				talign = fltk3::ALIGN_RIGHT;
				break;
			case 'B':
				if (!(l->flags & SELECTED)) {
					fltk3::color((fltk3::Color)strtol(str, &str, 10));
					fltk3::rectf(X, Y, w1, H);
				} else while (isdigit(*str & 255)) str++; // skip digits
				break;
			case 'C':
				lcol = (fltk3::Color)strtol(str, &str, 10);
				break;
			case 'F':
				font = (fltk3::Font)strtol(str, &str, 10);
				break;
			case 'N':
				lcol = fltk3::INACTIVE_COLOR;
				break;
			case 'S':
				tsize = strtol(str, &str, 10);
				break;
			case '-':
				fltk3::color(fltk3::DARK3);
				fltk3::line(X+3, Y+H/2, X+w1-3, Y+H/2);
				fltk3::color(fltk3::LIGHT3);
				fltk3::line(X+3, Y+H/2+1, X+w1-3, Y+H/2+1);
				break;
			case 'u':
			case '_':
				fltk3::color(lcol);
				fltk3::line(X+3, Y+H-1, X+w1-3, Y+H-1);
				break;
			case '.':
				goto BREAK;
			case '@':
				str--;
				goto BREAK;
			}
		}
BREAK:
		fltk3::font(font, tsize);
		if (l->flags & SELECTED)
			lcol = fltk3::contrast(lcol, selection_color());
		if (!active_r()) lcol = fltk3::inactive(lcol);
		fltk3::color(lcol);
		fltk3::draw(str, X+3, Y, w1-6, H, e ? fltk3::Align(talign|fltk3::ALIGN_CLIP) : talign, 0, 0);
		if (!e) break; // no more fields...
		*e = column_char(); // put the separator back
		X += w1;
		W -= w1;
		str = e+1;
	}
}

static const int no_columns[1] = {0};

/**
  The constructor makes an empty browser.
  \param[in] X,Y,W,H position and size.
  \param[in] L label string, may be NULL.
*/
fltk3::Browser::Browser(int X, int Y, int W, int H, const char *L)
	: fltk3::Browser_(X, Y, W, H, L)
{
	column_widths_ = no_columns;
	lines = 0;
	full_height_ = 0;
	cacheline = 0;
	format_char_ = '@';
	column_char_ = '\t';
	first = last = cache = 0;
}

/**
  Updates the browser so that \p line is shown at position \p pos.
  \param[in] line line number. (1 based)
  \param[in] pos position.
  \see topline(), middleline(), bottomline()
*/
void fltk3::Browser::lineposition(int line, LinePosition pos)
{
	if (line<1) line = 1;
	if (line>lines) line = lines;
	int p = 0;

	fltk3::BrowserLine_* l;
	for (l=first; l && line>1; l = l->next) {
		line--;
		p += item_height(l);
	}
	if (l && (pos == BOTTOM)) p += item_height (l);

	int final = p, X, Y, W, H;
	bbox(X, Y, W, H);

	switch(pos) {
	case TOP:
		break;
	case BOTTOM:
		final -= H;
		break;
	case MIDDLE:
		final -= H/2;
		break;
	}

	if (final > (full_height() - H)) final = full_height() -H;
	position(final);
}

/**
  Returns the line that is currently visible at the top of the browser.
  If there is no vertical scrollbar then this will always return 1.
  \returns The lineno() of the top() of the browser.
*/
int fltk3::Browser::topline() const
{
	return lineno(top());
}

/**
  Removes all the lines in the browser.
  \see add(), insert(), remove(), swap(int,int), clear()
*/
void fltk3::Browser::clear()
{
	for (fltk3::BrowserLine_* l = first; l;) {
		fltk3::BrowserLine_* n = l->next;
		free(l);
		l = n;
	}
	full_height_ = 0;
	first = 0;
	last = 0;
	lines = 0;
	new_list();
}

/**
  Adds a new line to the end of the browser.

  The text string \p newtext may contain format characters; see format_char() for details.
  \p newtext is copied using the strdup() function, and can be NULL to make a blank line.

  The optional void* argument \p d will be the data() for the new item.

  \param[in] newtext The label text used for the added item
  \param[in] d Optional user data() for the item (0 if unspecified)
  \see add(), insert(), remove(), swap(int,int), clear()
*/
void fltk3::Browser::add(const char* newtext, void* d)
{
	insert(lines+1, newtext, d);
	//fltk3::Browser_::display(last);
}

/**
  Returns the label text for the specified \p line.
  Return value can be NULL if \p line is out of range or unset.
  The parameter \p line is 1 based.
  \param[in] line The line number of the item whose text is returned. (1 based)
  \returns The text string (can be NULL)
*/
const char* fltk3::Browser::text(int line) const
{
	if (line < 1 || line > lines) return 0;
	return find_line(line)->txt;
}

/**
  Returns the user data() for specified \p line.
  Return value can be NULL if \p line is out of range or no user data() was defined.
  The parameter \p line is 1 based (1 will be the first item in the list).
  \param[in] line The line number of the item whose data() is returned. (1 based)
  \returns The user data pointer (can be NULL)

*/
void* fltk3::Browser::data(int line) const
{
	if (line < 1 || line > lines) return 0;
	return find_line(line)->data;
}

/**
  Sets the selection state of the item at \p line to the value \p val.
  If \p val is not specified, the default is 1 (selects the item).
  \param[in] line The line number of the item to be changed. (1 based)
  \param[in] val The new selection state (1=select, 0=de-select).
  \returns 1 if the state changed, 0 if not.
  \see select(), selected(), value(), item_select(), item_selected()
*/
int fltk3::Browser::select(int line, int val)
{
	if (line < 1 || line > lines) return 0;
	return Browser_::select(find_line(line), val);
}

/**
  Returns 1 if specified \p line is selected, 0 if not.
  \param[in] line The line being checked (1 based)
  \returns 1 if item selected, 0 if not.
  \see select(), selected(), value(), item_select(), item_selected()
  */
int fltk3::Browser::selected(int line) const
{
	if (line < 1 || line > lines) return 0;
	return find_line(line)->flags & SELECTED;
}

/**
  Makes \p line visible, and available for selection by user.
  Opposite of hide(int).
  This changes the full_height() if the state was changed.
  redraw() is called automatically if a change occurred.
  \param[in] line The line to be shown. (1 based)
  \see show(int), hide(int), display(), visible(), make_visible()
*/
void fltk3::Browser::show(int line)
{
	fltk3::BrowserLine_* t = find_line(line);
	if (t->flags & NOTDISPLAYED) {
		t->flags &= ~NOTDISPLAYED;
		full_height_ += item_height(t);
		if (Browser_::displayed(t)) redraw();
	}
}

/**
  Makes \p line invisible, preventing selection by the user.
  The line can still be selected under program control.
  This changes the full_height() if the state was changed.
  When a line is made invisible, lines below it are moved up in the display.
  redraw() is called automatically if a change occurred.
  \param[in] line The line to be hidden. (1 based)
  \see show(int), hide(int), display(), visible(), make_visible()
*/
void fltk3::Browser::hide(int line)
{
	fltk3::BrowserLine_* t = find_line(line);
	if (!(t->flags & NOTDISPLAYED)) {
		full_height_ -= item_height(t);
		t->flags |= NOTDISPLAYED;
		if (Browser_::displayed(t)) redraw();
	}
}

/**
  For back compatibility.
  This calls show(line) if \p val is true, and hide(line) otherwise.
  If \p val is not specified, the default is 1 (makes the line visible).
  \see show(int), hide(int), display(), visible(), make_visible()
*/
void fltk3::Browser::display(int line, int val)
{
	if (line < 1 || line > lines) return;
	if (val) show(line);
	else hide(line);
}

/**
  Returns non-zero if the specified \p line is visible, 0 if hidden.
  Use show(int), hide(int), or make_visible(int) to change an item's visible state.
  \param[in] line The line in the browser to be tested. (1 based)
  \see show(int), hide(int), display(), visible(), make_visible()
*/
int fltk3::Browser::visible(int line) const
{
	if (line < 1 || line > lines) return 0;
	return !(find_line(line)->flags&NOTDISPLAYED);
}

/**
  Returns the line number of the currently selected line, or 0 if none.
  \returns The line number of current selection, or 0 if none selected.
  \see select(), selected(), value(), item_select(), item_selected()
*/
int fltk3::Browser::value() const
{
	return lineno(selection());
}

/**
  Swap the two items \p a and \p b.
  Uses swapping() to ensure list updates correctly.
  \param[in] a,b The two items to be swapped.
  \see swap(int,int), item_swap()
*/
void fltk3::Browser::swap(fltk3::BrowserLine_ *a, fltk3::BrowserLine_ *b)
{

	if ( a == b || !a || !b) return;          // nothing to do
	swapping(a, b);
	fltk3::BrowserLine_ *aprev  = a->prev;
	fltk3::BrowserLine_ *anext  = a->next;
	fltk3::BrowserLine_ *bprev  = b->prev;
	fltk3::BrowserLine_ *bnext  = b->next;
	if ( b->prev == a ) { 		// A ADJACENT TO B
		if ( aprev ) aprev->next = b;
		else first = b;
		b->next = a;
		a->next = bnext;
		b->prev = aprev;
		a->prev = b;
		if ( bnext ) bnext->prev = a;
		else last = a;
	} else if ( a->prev == b ) {		// B ADJACENT TO A
		if ( bprev ) bprev->next = a;
		else first = a;
		a->next = b;
		b->next = anext;
		a->prev = bprev;
		b->prev = a;
		if ( anext ) anext->prev = b;
		else last = b;
	} else {				// A AND B NOT ADJACENT
		// handle prev's
		b->prev = aprev;
		if ( anext ) anext->prev = b;
		else last = b;
		a->prev = bprev;
		if ( bnext ) bnext->prev = a;
		else last = a;
		// handle next's
		if ( aprev ) aprev->next = b;
		else first = b;
		b->next = anext;
		if ( bprev ) bprev->next = a;
		else first = a;
		a->next = bnext;
	}
	// Disable cache -- we played around with positions
	cacheline = 0;
	cache = 0;
}

/**
  Swaps two browser lines \p a and \p b.
  You must call redraw() to make any changes visible.
  \param[in] a,b The two lines to be swapped. (both 1 based)
  \see swap(int,int), item_swap()
*/
void fltk3::Browser::swap(int a, int b)
{
	if (a < 1 || a > lines || b < 1 || b > lines) return;
	fltk3::BrowserLine_* ai = find_line(a);
	fltk3::BrowserLine_* bi = find_line(b);
	swap(ai,bi);
}

/**
  Set the image icon for \p line to the value \p icon.
  Caller is responsible for keeping the icon allocated.
  The \p line is automatically redrawn.
  \param[in] line The line to be modified. If out of range, nothing is done.
  \param[in] icon The image icon to be assigned to the \p line.
                  If NULL, any previous icon is removed.
*/
void fltk3::Browser::icon(int line, fltk3::Image* icon)
{

	if (line<1 || line > lines) return;

	fltk3::BrowserLine_* bl = find_line(line);

	int old_h = bl->icon ? bl->icon->h()+2 : 0;	// init with *old* icon height
	bl->icon = 0;					// remove icon, if any
	int th = item_height(bl);			// height of text only
	int new_h = icon ? icon->h()+2 : 0;		// init with *new* icon height
	if (th > old_h) old_h = th;
	if (th > new_h) new_h = th;
	int dh = new_h - old_h;
	full_height_ += dh;				// do this *always*

	bl->icon = icon;				// set new icon
	if (dh>0) {
		redraw();					// icon larger than item? must redraw widget
	} else {
		redraw_line(bl);				// icon same or smaller? can redraw just this line
	}
	replacing(bl,bl);				// recalc fltk3::Browser_::max_width et al
}

/**
  Returns the icon currently defined for \p line.
  If no icon is defined, NULL is returned.
  \param[in] line The line whose icon is returned.
  \returns The icon defined, or NULL if none.
*/
fltk3::Image* fltk3::Browser::icon(int line) const
{
	fltk3::BrowserLine_* l = find_line(line);
	return(l ? l->icon : NULL);
}

/**
  Removes the icon for \p line.
  It's ok to remove an icon if none has been defined.
  \param[in] line The line whose icon is to be removed.
*/
void fltk3::Browser::remove_icon(int line)
{
	icon(line,0);
}


fltk3::HoldBrowser::HoldBrowser(int X,int Y,int W,int H,const char *L)
	: fltk3::Browser(X,Y,W,H,L)
{
	type(fltk3::HOLD_BROWSER);
}


fltk3::MultiBrowser::MultiBrowser(int X,int Y,int W,int H,const char *L)
	: fltk3::Browser(X,Y,W,H,L)
{
	type(fltk3::MULTI_BROWSER);
}


fltk3::SelectBrowser::SelectBrowser(int X,int Y,int W,int H,const char *L)
	: fltk3::Browser(X,Y,W,H,L)
{
	type(fltk3::SELECT_BROWSER);
}


//
// End of "$Id: Browser.cxx 9638 2012-07-24 04:41:38Z matt $".
//
