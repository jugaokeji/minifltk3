//
// "$Id: Input_.cxx 9818 2013-02-09 14:16:47Z manolo $"
//
// Common input widget routines for the Fast Light Tool Kit (FLTK).
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
#include "Input_.h"
#include "Window.h"
#include "MenuItem.h"
#include "draw.h"
#include "ask.h"
#include <math.h>
#include "utf8.h"
#include "flstring.h"
#include <stdlib.h>
#include <ctype.h>

#define MAXBUF 1024
#if defined(USE_X11) && !USE_XFT
const int secret_char = '*'; // asterisk to hide secret input
#else
const int secret_char = 0x2022; // bullet to hide secret input
#endif
static int l_secret;

namespace fltk3
{
extern void draw(const char*, int, float, float);
}

////////////////////////////////////////////////////////////////

/** \internal
  Converts a given text segment into the text that will be rendered on screen.

  This copies the text from \p p to \p buf, replacing characters with <tt>^X</tt>
  and <tt>\\nnn</tt> as necessary.

  The destination buffer is limited to \c MAXBUF (currently at 1024). All
  following text is truncated.

  \param [in] p pointer to source buffer
  \param [in] buf pointer to destination buffer
  \return pointer to the end of the destination buffer
*/
const char* fltk3::Input_::expand(const char* p, char* buf) const
{
	char* o = buf;
	char* e = buf+(MAXBUF-4);
	const char* lastspace = p;
	char* lastspace_out = o;
	int width_to_lastspace = 0;
	int word_count = 0;
	int word_wrap;
//  const char *pe = p + strlen(p);

	if (input_type()==fltk3::SECRET_INPUT) {
		while (o<e && p < value_+size_) {
			if (fltk3::utf8len((char)p[0]) >= 1) {
				l_secret = fltk3::utf8encode(secret_char, o);
				o += l_secret;
			}
			p++;
		}

	} else while (o<e) {
			if (wrap() && (p >= value_+size_ || isspace(*p & 255))) {
				word_wrap = w() - fltk3::box_dw(box()) - 2;
				width_to_lastspace += (int)fltk3::width(lastspace_out, (int)(o-lastspace_out));
				if (p > lastspace+1) {
					if (word_count && width_to_lastspace > word_wrap) {
						p = lastspace;
						o = lastspace_out;
						break;
					}
					word_count++;
				}
				lastspace = p;
				lastspace_out = o;
			}

			if (p >= value_+size_) break;
			int c = *p++ & 255;
			if (c < ' ' || c == 127) {
				if (c=='\n' && input_type()==fltk3::MULTILINE_INPUT) {
					p--;
					break;
				}
				if (c == '\t' && input_type()==fltk3::MULTILINE_INPUT) {
					for (c = fltk3::utf_nb_char( (uchar*)buf, (int)(o-buf) )%8; c<8 && o<e; c++) {
						*o++ = ' ';
					}
				} else {
					*o++ = '^';
					*o++ = c ^ 0x40;
				}
			} else {
				*o++ = c;
			}
		}
	*o = 0;
	return p;
}

/** \internal
  Calculates the width in pixels of part of a text buffer.

  This call takes a string, usually created by expand, and calculates
  the width of the string when rendered with the given font.

  \param [in] p pointer to the start of the original string
  \param [in] e pointer to the end of the original string
  \param [in] buf pointer to the buffer as returned by expand()
  \return width of string in pixels
*/
double fltk3::Input_::expandpos(
        const char* p,	// real string
        const char* e,	// pointer into real string
        const char* buf,	// conversion of real string by expand()
        int* returnn		// return offset into buf here
) const
{
	int n = 0;
	int chr = 0;
	int l;
	if (input_type()==fltk3::SECRET_INPUT) {
		while (p<e) {
			l = fltk3::utf8len((char)p[0]);
			if (l >= 1) n += l_secret;
			p += l;
		}
	} else while (p<e) {
			int c = *p & 255;
			if (c < ' ' || c == 127) {
				if (c == '\t' && input_type()==fltk3::MULTILINE_INPUT) {
					n += 8-(chr%8);
					chr += 7-(chr%8);
				} else n += 2;
			} else {
				n++;
			}
			chr += fltk3::utf8len((char)p[0]) >= 1;
			p++;
		}
	if (returnn) *returnn = n;
	return fltk3::width(buf, n);
}

////////////////////////////////////////////////////////////////

/** \internal
  Marks a range of characters for update.

  This call marks all characters from \p to the end of the
  text buffer for update. At least these characters
  will be redrawn in the next update cycle.

  Characters from \p mu_p to end of widget are redrawn.
  If \p erase_cursor_only, small part at \p mu_p is redrawn.
  Right now minimal update just keeps unchanged characters from
  being erased, so they don't blink.

  \param [in] p start of update range
*/
void fltk3::Input_::minimal_update(int p)
{
	if (damage() & fltk3::DAMAGE_ALL) return; // don't waste time if it won't be done
	if (damage() & fltk3::DAMAGE_EXPOSE) {
		if (p < mu_p) mu_p = p;
	} else {
		mu_p = p;
	}

	damage(fltk3::DAMAGE_EXPOSE);
	erase_cursor_only = 0;
}

/** \internal
  Marks a range of characters for update.

  This call marks a text range for update. At least all characters
  from \p p to \p q will be redrawn in the next update cycle.

  \param [in] p start of update range
  \param [in] q end of update range
*/
void fltk3::Input_::minimal_update(int p, int q)
{
	if (q < p) p = q;
	minimal_update(p);
}

////////////////////////////////////////////////////////////////

/* Horizontal cursor position in pixels while moving up or down. */
double fltk3::Input_::up_down_pos = 0;

/* Flag to remember last cursor move. */
int fltk3::Input_::was_up_down = 0;

/**
  Sets the current font and font size.
*/
void fltk3::Input_::setfont() const
{
	fltk3::font(textfont(), textsize());
}

/**
  Draws the text in the passed bounding box.

  If <tt>damage() & fltk3::DAMAGE_ALL</tt> is true, this assumes the
  area has already been erased to color(). Otherwise it does
  minimal update and erases the area itself.

  \param X, Y, W, H area that must be redrawn
*/
void fltk3::Input_::drawtext(int X, int Y, int W, int H)
{
	int do_mu = !(damage()&fltk3::DAMAGE_ALL);

	if (fltk3::focus()!=this && !size()) {
		if (do_mu) { // we have to erase it if cursor was there
			draw_box(box(), X-fltk3::box_dx(box()), Y-fltk3::box_dy(box()),
			         W+fltk3::box_dw(box()), H+fltk3::box_dh(box()), color());
		}
		return;
	}

	int selstart, selend;
	if (fltk3::focus()!=this && /*fltk3::selection_owner()!=this &&*/ fltk3::pushed()!=this)
		selstart = selend = 0;
	else if (position() <= mark()) {
		selstart = position();
		selend = mark();
	} else {
		selend = position();
		selstart = mark();
	}

	setfont();
	const char *p, *e;
	char buf[MAXBUF];

	// count how many lines and put the last one into the buffer:
	// And figure out where the cursor is:
	int height = fltk3::height();
	int threshold = height/2;
	int lines;
	int curx, cury;
	for (p=value(), curx=cury=lines=0; ;) {
		e = expand(p, buf);
		if (position() >= p-value() && position() <= e-value()) {
			curx = int(expandpos(p, value()+position(), buf, 0)+.5);
			if (fltk3::focus()==this && !was_up_down) up_down_pos = curx;
			cury = lines*height;
			int newscroll = xscroll_;
			if (curx > newscroll+W-threshold) {
				// figure out scrolling so there is space after the cursor:
				newscroll = curx+threshold-W;
				// figure out the furthest left we ever want to scroll:
				int ex = int(expandpos(p, e, buf, 0))+4-W;
				// use minimum of both amounts:
				if (ex < newscroll) newscroll = ex;
			} else if (curx < newscroll+threshold) {
				newscroll = curx-threshold;
			}
			if (newscroll < 0) newscroll = 0;
			if (newscroll != xscroll_) {
				xscroll_ = newscroll;
				mu_p = 0;
				erase_cursor_only = 0;
			}
		}
		lines++;
		if (e >= value_+size_) break;
		p = e+1;
	}

	// adjust the scrolling:
	if (input_type()==fltk3::MULTILINE_INPUT) {
		int newy = yscroll_;
		if (cury < newy) newy = cury;
		if (cury > newy+H-height) newy = cury-H+height;
		if (newy < -1) newy = -1;
		if (newy != yscroll_) {
			yscroll_ = newy;
			mu_p = 0;
			erase_cursor_only = 0;
		}
	} else {
		yscroll_ = -(H-height)/2;
	}

	fltk3::push_clip(X, Y, W, H);
	fltk3::Color tc = active_r() ? textcolor() : fltk3::inactive(textcolor());

	p = value();
	// visit each line and draw it:
	int desc = height-fltk3::descent();
	float xpos = (float)(X - xscroll_ + 1);
	int ypos = -yscroll_;
	for (; ypos < H;) {

		// re-expand line unless it is the last one calculated above:
		if (lines>1) e = expand(p, buf);

		if (ypos <= -height) goto CONTINUE; // clipped off top

		if (do_mu) {	// for minimal update:
			const char* pp = value()+mu_p; // pointer to where minimal update starts
			if (e < pp) goto CONTINUE2; // this line is before the changes
			if (readonly()) erase_cursor_only = 0; // this isn't the most efficient way
			if (erase_cursor_only && p > pp) goto CONTINUE2; // this line is after
			// calculate area to erase:
			float r = (float)(X+W);
			float xx;
			if (p >= pp) {
				xx = (float)X;
				if (erase_cursor_only) r = xpos+2;
				else if (readonly()) xx -= 3;
			} else {
				xx = xpos + (float)expandpos(p, pp, buf, 0);
				if (erase_cursor_only) r = xx+2;
				else if (readonly()) xx -= 3;
			}
			// clip to and erase it:
			fltk3::push_clip((int)xx-1-height/8, Y+ypos, (int)(r-xx+2+height/4), height);
			draw_box(box(), X-fltk3::box_dx(box()), Y-fltk3::box_dy(box()),
			         W+fltk3::box_dw(box()), H+fltk3::box_dh(box()), color());
			// it now draws entire line over it
			// this should not draw letters to left of erased area, but
			// that is nyi.
		}

		// Draw selection area if required:
		if (selstart < selend && selstart <= e-value() && selend > p-value()) {
			const char* pp = value()+selstart;
			float x1 = xpos;
			int offset1 = 0;
			if (pp > p) {
				fltk3::color(tc);
				x1 += (float)expandpos(p, pp, buf, &offset1);
				fltk3::draw(buf, offset1, xpos, (float)(Y+ypos+desc));
			}
			pp = value()+selend;
			float x2 = (float)(X+W);
			int offset2;
			if (pp <= e) x2 = xpos + (float)expandpos(p, pp, buf, &offset2);
			else offset2 = (int)strlen(buf);
#ifdef __APPLE__ // Mac OS: underline marked ( = selected + fltk3::compose_state != 0) text 
			if (fltk3::compose_state) {
				fltk3::color(textcolor());
			} else
#endif
			{
				fltk3::color(selection_color());
				fltk3::rectf((int)(x1+0.5), Y+ypos, (int)(x2-x1+0.5), height);
				fltk3::color(fltk3::contrast(textcolor(), selection_color()));
			}
			fltk3::draw(buf+offset1, offset2-offset1, x1, (float)(Y+ypos+desc));
#ifdef __APPLE__ // Mac OS: underline marked ( = selected + fltk3::compose_state != 0) text
			if (fltk3::compose_state) {
				fltk3::color( fltk3::color_average(textcolor(), color(), 0.6) );
				float width = fltk3::width(buf+offset1, offset2-offset1);
				fltk3::line(x1, Y+ypos+height-1, x1+width, Y+ypos+height-1);
			}
#endif
			if (pp < e) {
				fltk3::color(tc);
				fltk3::draw(buf+offset2, (int)strlen(buf+offset2), x2, (float)(Y+ypos+desc));
			}
		} else {
			// draw unselected text
			fltk3::color(tc);
			fltk3::draw(buf, (int)strlen(buf), xpos, (float)(Y+ypos+desc));
		}

		if (do_mu) fltk3::pop_clip();

CONTINUE2:
		// draw the cursor:
		if (fltk3::focus() == this && (
#ifdef __APPLE__
		            fltk3::compose_state ||
#endif
		            selstart == selend) &&
		    position() >= p-value() && position() <= e-value()) {
			fltk3::color(cursor_color());
			// cursor position may need to be recomputed (see STR #2486)
			curx = int(expandpos(p, value()+position(), buf, 0)+.5);
			if (readonly()) {
				fltk3::line((int)(xpos+curx-2.5f), Y+ypos+height-1,
				            (int)(xpos+curx+0.5f), Y+ypos+height-4,
				            (int)(xpos+curx+3.5f), Y+ypos+height-1);
			} else {
				fltk3::rectf((int)(xpos+curx+0.5), Y+ypos, 2, height);
			}
#ifdef __APPLE__
			fltk3::insertion_point_location(xpos+curx, Y+ypos+height, height);
#endif
		}

CONTINUE:
		ypos += height;
		if (e >= value_+size_) break;
		if (*e == '\n' || *e == ' ') e++;
		p = e;
	}

	// for minimal update, erase all lines below last one if necessary:
	if (input_type()==fltk3::MULTILINE_INPUT && do_mu && ypos<H
	    && (!erase_cursor_only || p <= value()+mu_p)) {
		if (ypos < 0) ypos = 0;
		fltk3::push_clip(X, Y+ypos, W, H-ypos);
		draw_box(box(), X-fltk3::box_dx(box()), Y-fltk3::box_dy(box()),
		         W+fltk3::box_dw(box()), H+fltk3::box_dh(box()), color());
		fltk3::pop_clip();
	}

	fltk3::pop_clip();
	if (fltk3::focus() == this) {
		fltk3::set_spot(textfont(), textsize(),
		                (int)xpos+curx, Y+ypos-fltk3::descent(), W, H, window());
	}
}

/** \internal
  Simple function that determines if a character could be part of a word.
  \todo This function is not ucs4-aware.
*/
static int isword(char c)
{
	return (c&128 || isalnum(c) || strchr("#%&-/@\\_~", c));
}

/**
  Finds the end of a word.

  This call calculates the end of a word based on the given
  index \p i. Calling this function repeatedly will move
  forwards to the end of the text.

  \param [in] i starting index for the search
  \return end of the word
*/
int fltk3::Input_::word_end(int i) const
{
	if (input_type() == fltk3::SECRET_INPUT) return size();
	//while (i < size() && !isword(index(i))) i++;
	while (i < size() && !isword(index(i))) i++;
	while (i < size() && isword(index(i))) i++;
	return i;
}

/**
  Finds the start of a word.

  This call calculates the start of a word based on the given
  index \p i. Calling this function repeatedly will move
  backwards to the beginning of the text.

  \param [in] i starting index for the search
  \return start of the word
*/
int fltk3::Input_::word_start(int i) const
{
	if (input_type() == fltk3::SECRET_INPUT) return 0;
//   if (i >= size() || !isword(index(i)))
//     while (i > 0 && !isword(index(i-1))) i--;
	while (i > 0 && !isword(index(i-1))) i--;
	while (i > 0 && isword(index(i-1))) i--;
	return i;
}

/**
  Finds the end of a line.

  This call calculates the end of a line based on the given
  index \p i.

  \param [in] i starting index for the search
  \return end of the line
*/
int fltk3::Input_::line_end(int i) const
{
	if (input_type() != fltk3::MULTILINE_INPUT) return size();

	if (wrap()) {
		// go to the start of the paragraph:
		int j = i;
		while (j > 0 && index(j-1) != '\n') j--;
		// now measure lines until we get past i, end of that line is real eol:
		setfont();
		for (const char* p=value()+j; ;) {
			char buf[MAXBUF];
			p = expand(p, buf);
			int k = (int)(p - value());
			if (k >= i) return k;
			p++;
		}
	} else {
		while (i < size() && index(i) != '\n') i++;
		return i;
	}
}

/**
  Finds the start of a line.

  This call calculates the start of a line based on the given
  index \p i.

  \param [in] i starting index for the search
  \return start of the line
*/
int fltk3::Input_::line_start(int i) const
{
	if (input_type() != fltk3::MULTILINE_INPUT) return 0;
	int j = i;
	while (j > 0 && index(j-1) != '\n') j--;
	if (wrap()) {
		// now measure lines until we get past i, start of that line is real eol:
		setfont();
		for (const char* p=value()+j; ;) {
			char buf[MAXBUF];
			const char* e = expand(p, buf);
			if ( (int)(e-value()) >= i) return (int)(p-value());
			p = e+1;
		}
	} else return j;
}


static fltk3::MenuItem ccp_menu[] = {
	{ "Cut", fltk3::COMMAND|'x', 0, (void*)1 },
	{ "Copy", fltk3::COMMAND|'c', 0, (void*)2 },
	{ "Paste", fltk3::COMMAND|'v', 0, (void*)3 },
	{ 0 }
};

/**
 Handles right mouse button clicks.
 */
void fltk3::Input_::handle_menu_event()
{

	if (fltk3::focus() != this) {
		fltk3::focus(this);
		handle(fltk3::FOCUS);
	}

	int m = mark(), p = position();
	if (m<p) {
		int x=p;
		p=m;
		m=x;
	}
	handle_mouse(x()+fltk3::box_dx(box()), y()+fltk3::box_dy(box()), 0, 0, 0);
	if ( ( (p==m && p!=position()) || position()<p || position()>m ) && (input_type()!=fltk3::SECRET_INPUT)) {
		handle_mouse(x()+fltk3::box_dx(box()), y()+fltk3::box_dy(box()), 0, 0, 0);
		p = word_start(position());
		m = word_end(position());
	}
	position(p, m);
	if (p!=m && (input_type()!=fltk3::SECRET_INPUT) && !readonly())
		ccp_menu[0].activate();
	else
		ccp_menu[0].deactivate();
	if (p!=m && (input_type()!=fltk3::SECRET_INPUT))
		ccp_menu[1].activate();
	else
		ccp_menu[1].deactivate();
	if (!readonly() /*&& paste_buffer && *paste_buffer*/ ) // TODO: provide a function that can check if data is in the paste buffer
		ccp_menu[2].activate();
	else
		ccp_menu[2].deactivate();
	redraw();
	fltk3::flush();
	const fltk3::MenuItem *mi = ccp_menu->popup(fltk3::event_x(), fltk3::event_y());
	if (mi) {
		switch (mi->argument()) {
		case 1:
			copy(1);
			cut();
			break;
		case 2:
			copy(1);
			break;
		case 3:
			paste(*this, 1);
			break;
		}
	}
	return;
}

/**
  Handles mouse clicks and mouse moves.
  \todo Add comment and parameters
*/
void fltk3::Input_::handle_mouse(int X, int Y, int /*W*/, int /*H*/, int drag)
{

	was_up_down = 0;
	if (!size()) return;
	setfont();

	const char *p, *e;
	char buf[MAXBUF];

	int theline = (input_type()==fltk3::MULTILINE_INPUT) ?
	              (fltk3::event_y()-Y+yscroll_)/fltk3::height() : 0;

	int newpos = 0;
	for (p=value();; ) {
		e = expand(p, buf);
		theline--;
		if (theline < 0) break;
		if (e >= value_+size_) break;
		p = e+1;
	}
	const char *l, *r, *t;
	double f0 = fltk3::event_x()-X+xscroll_;
	for (l = p, r = e; l<r; ) {
		double f;
		int cw = fltk3::utf8len((char)l[0]);
		if (cw < 1) cw = 1;
		t = l+cw;
		f = X-xscroll_+expandpos(p, t, buf, 0);
		if (f <= fltk3::event_x()) {
			l = t;
			f0 = fltk3::event_x()-f;
		} else r = t-cw;
	}
	if (l < e) { // see if closer to character on right:
		double f1;
		int cw = fltk3::utf8len((char)l[0]);
		if (cw > 0) {
			f1 = X-xscroll_+expandpos(p, l + cw, buf, 0) - fltk3::event_x();
			if (f1 < f0) l = l+cw;
		}
	}
	newpos = (int)(l-value());

	int newmark = drag ? mark() : newpos;
	if (fltk3::event_clicks()) {
		if (newpos >= newmark) {
			if (newpos == newmark) {
				if (newpos < size()) newpos++;
				else newmark--;
			}
			if (fltk3::event_clicks() > 1) {
				newpos = line_end(newpos);
				newmark = line_start(newmark);
			} else {
				newpos = word_end(newpos);
				newmark = word_start(newmark);
			}
		} else {
			if (fltk3::event_clicks() > 1) {
				newpos = line_start(newpos);
				newmark = line_end(newmark);
			} else {
				newpos = word_start(newpos);
				newmark = word_end(newmark);
			}
		}
		// if the multiple click does not increase the selection, revert
		// to single-click behavior:
		if (!drag && (mark() > position() ?
		              (newmark >= position() && newpos <= mark()) :
		              (newmark >= mark() && newpos <= position()))) {
			fltk3::event_clicks(0);
			newmark = newpos = (int)(l-value());
		}
	}
	position(newpos, newmark);
}

/**
  Sets the index for the cursor and mark.

  The input widget maintains two pointers into the string. The
  \e position (\c p) is where the cursor is. The
  \e mark (\c m) is the other end of the selected text. If they
  are equal then there is no selection. Changing this does not
  affect the clipboard (use copy() to do that).

  Changing these values causes a redraw(). The new
  values are bounds checked.

  \param p index for the cursor position
  \param m index for the mark
  \return 0 if no positions changed
  \see position(int), position(), mark(int)
*/
int fltk3::Input_::position(int p, int m)
{
	int is_same = 0;
	was_up_down = 0;
	if (p<0) p = 0;
	if (p>size()) p = size();
	if (m<0) m = 0;
	if (m>size()) m = size();
	if (p == m) is_same = 1;

	while (p < position_ && p > 0 && (size() - p) > 0 &&
	       (fltk3::utf8len((char)(value() + p)[0]) < 1)) {
		p--;
	}
	int ul = fltk3::utf8len((char)(value() + p)[0]);
	while (p < size() && p > position_ && ul < 0) {
		p++;
		ul = fltk3::utf8len((char)(value() + p)[0]);
	}

	while (m < mark_ && m > 0 && (size() - m) > 0 &&
	       (fltk3::utf8len((char)(value() + m)[0]) < 1)) {
		m--;
	}
	ul = fltk3::utf8len((char)(value() + m)[0]);
	while (m < size() && m > mark_ && ul < 0) {
		m++;
		ul = fltk3::utf8len((char)(value() + m)[0]);
	}
	if (is_same) m = p;
	if (p == position_ && m == mark_) return 0;


	//if (fltk3::selection_owner() == this) fltk3::selection_owner(0);
	if (p != m) {
		if (p != position_) minimal_update(position_, p);
		if (m != mark_) minimal_update(mark_, m);
	} else {
		// new position is a cursor
		if (position_ == mark_) {
			// old position was just a cursor
			if (fltk3::focus() == this && !(damage()&fltk3::DAMAGE_EXPOSE)) {
				minimal_update(position_);
				erase_cursor_only = 1;
			}
		} else { // old position was a selection
			minimal_update(position_, mark_);
		}
	}
	position_ = p;
	mark_ = m;
	return 1;
}

/**
  Moves the cursor to the column given by \p up_down_pos.

  This function is helpful when implementing up and down
  cursor movement. It moves the cursor from the beginning
  of a line to the column indicated by the global variable
  \p up_down_pos in pixel units.

  \param [in] i index into the beginning of a line of text
  \param [in] keepmark if set, move only the cursor, but not the mark
  \return index to new cursor position
*/
int fltk3::Input_::up_down_position(int i, int keepmark)
{
	// unlike before, i must be at the start of the line already!

	setfont();
	char buf[MAXBUF];
	const char* p = value()+i;
	const char* e = expand(p, buf);
	const char *l, *r, *t;
	for (l = p, r = e; l<r; ) {
		t = l+(r-l+1)/2;
		int f = (int)expandpos(p, t, buf, 0);
		if (f <= up_down_pos) l = t;
		else r = t-1;
	}
	int j = (int)(l-value());
	j = position(j, keepmark ? mark_ : j);
	was_up_down = 1;
	return j;
}

/**
  Put the current selection into the clipboard.

  This function copies the current selection between mark() and
  position() into the specified \c clipboard. This does not
  replace the old clipboard contents if position() and
  mark() are equal. Clipboard 0 maps to the current text
  selection and clipboard 1 maps to the cut/paste clipboard.

  \param clipboard the clipboard destination 0 or 1
  \return 0 if no text is selected, 1 if the selection was copied
  \see fltk3::copy(const char *, int, int)
*/
int fltk3::Input_::copy(int clipboard)
{
	int b = position();
	int e = mark();
	if (b != e) {
		if (b > e) {
			b = mark();
			e = position();
		}
		if (input_type() == fltk3::SECRET_INPUT) e = b;
		fltk3::copy(value()+b, e-b, clipboard);
		return 1;
	}
	return 0;
}

#define MAXFLOATSIZE 40

static char* undobuffer;
static int undobufferlength;
static fltk3::Input_* undowidget;
static int undoat;	// points after insertion
static int undocut;	// number of characters deleted there
static int undoinsert;	// number of characters inserted
static int yankcut;	// length of valid contents of buffer, even if undocut=0

static void undobuffersize(int n)
{
	if (n > undobufferlength) {
		if (undobuffer) {
			do {
				undobufferlength *= 2;
			} while (undobufferlength < n);
			undobuffer = (char*)realloc(undobuffer, undobufferlength);
		} else {
			undobufferlength = n+9;
			undobuffer = (char*)malloc(undobufferlength);
		}
	}
}

/**
  Deletes text from \p b to \p e and inserts the new string \p text.

  All changes to the text buffer go through this function.
  It deletes the region between \p a and \p b (either one may be less or
  equal to the other), and then inserts the string \p text
  at that point and moves the mark() and
  position() to the end of the insertion. Does the callback if
  <tt>when() & fltk3::WHEN_CHANGED</tt> and there is a change.

  Set \p b and \p e equal to not delete anything.
  Set \p text to \c NULL to not insert anything.

  \p ilen can be zero or <tt>strlen(text)</tt>, which
  saves a tiny bit of time if you happen to already know the
  length of the insertion, or can be used to insert a portion of a
  string.

  \p b and \p e are clamped to the
  <tt>0..size()</tt> range, so it is safe to pass any values.

  cut() and insert() are just inline functions that call replace().

  \param [in] b beginning index of text to be deleted
  \param [in] e ending index of text to be deleted and insertion position
  \param [in] text string that will be inserted
  \param [in] ilen length of \p text or 0 for \c nul terminated strings
  \return 0 if nothing changed
*/
int fltk3::Input_::replace(int b, int e, const char* text, int ilen)
{
	int ul, om, op;
	was_up_down = 0;

	if (b<0) b = 0;
	if (e<0) e = 0;
	if (b>size_) b = size_;
	if (e>size_) e = size_;
	if (e<b) {
		int t=b;
		b=e;
		e=t;
	}
	while (b != e && b > 0 && (size_ - b) > 0 &&
	       (fltk3::utf8len((value_ + b)[0]) < 1)) {
		b--;
	}
	ul = fltk3::utf8len((char)(value_ + e)[0]);
	while (e < size_ && e > 0 && ul < 0) {
		e++;
		ul = fltk3::utf8len((char)(value_ + e)[0]);
	}
	if (text && !ilen) ilen = (int)strlen(text);
	if (e<=b && !ilen) return 0; // don't clobber undo for a null operation
	if (size_+ilen-(e-b) > maximum_size_) {
		ilen = maximum_size_-size_+(e-b);
		if (ilen < 0) ilen = 0;
	}

	put_in_buffer(size_+ilen);

	if (e>b) {
		if (undowidget == this && b == undoat) {
			undobuffersize(undocut+(e-b));
			memcpy(undobuffer+undocut, value_+b, e-b);
			undocut += e-b;
		} else if (undowidget == this && e == undoat && !undoinsert) {
			undobuffersize(undocut+(e-b));
			memmove(undobuffer+(e-b), undobuffer, undocut);
			memcpy(undobuffer, value_+b, e-b);
			undocut += e-b;
		} else if (undowidget == this && e == undoat && (e-b)<undoinsert) {
			undoinsert -= e-b;
		} else {
			undobuffersize(e-b);
			memcpy(undobuffer, value_+b, e-b);
			undocut = e-b;
			undoinsert = 0;
		}
		memmove(buffer+b, buffer+e, size_-e+1);
		size_ -= e-b;
		undowidget = this;
		undoat = b;
		if (input_type() == fltk3::SECRET_INPUT) yankcut = 0;
		else yankcut = undocut;
	}

	if (ilen) {
		if (undowidget == this && b == undoat)
			undoinsert += ilen;
		else {
			undocut = 0;
			undoinsert = ilen;
		}
		memmove(buffer+b+ilen, buffer+b, size_-b+1);
		memcpy(buffer+b, text, ilen);
		size_ += ilen;
	}
	undowidget = this;
	om = mark_;
	op = position_;
	mark_ = position_ = undoat = b+ilen;

	// Insertions into the word at the end of the line will cause it to
	// wrap to the next line, so we must indicate that the changes may start
	// right after the whitespace before the current word.  This will
	// result in sub-optimal update when such wrapping does not happen
	// but it is too hard to figure out for now...
	if (wrap()) {
		// if there is a space in the pasted text, the whole line may have rewrapped
		int i;
		for (i=0; i<ilen; i++)
			if (text[i]==' ') break;
		if (i==ilen)
			while (b > 0 && !isspace(index(b) & 255) && index(b)!='\n') b--;
		else
			while (b > 0 && index(b)!='\n') b--;
	}

	// make sure we redraw the old selection or cursor:
	if (om < b) b = om;
	if (op < b) b = op;

	minimal_update(b);

	mark_ = position_ = undoat;

	set_changed();
	if (when()&fltk3::WHEN_CHANGED) do_callback();
	return 1;
}

/**
  Undoes previous changes to the text buffer.

  This call undoes a number of previous calls to replace().

  \return non-zero if any change was made.
*/
int fltk3::Input_::undo()
{
	was_up_down = 0;
	if ( undowidget != this || (!undocut && !undoinsert) ) return 0;

	int ilen = undocut;
	int xlen = undoinsert;
	int b = undoat-xlen;
	int b1 = b;

	put_in_buffer(size_+ilen);

	if (ilen) {
		memmove(buffer+b+ilen, buffer+b, size_-b+1);
		memcpy(buffer+b, undobuffer, ilen);
		size_ += ilen;
		b += ilen;
	}

	if (xlen) {
		undobuffersize(xlen);
		memcpy(undobuffer, buffer+b, xlen);
		memmove(buffer+b, buffer+b+xlen, size_-xlen-b+1);
		size_ -= xlen;
	}

	undocut = xlen;
	if (xlen) yankcut = xlen;
	undoinsert = ilen;
	undoat = b;
	mark_ = b /* -ilen */;
	position_ = b;

	if (wrap())
		while (b1 > 0 && index(b1)!='\n') b1--;
	minimal_update(b1);
	set_changed();
	if (when()&fltk3::WHEN_CHANGED) do_callback();
	return 1;
}

/**
  Copies the \e yank buffer to the clipboard.

  This method copies all the previous contiguous cuts from the undo
  information to the clipboard. This function implements
  the \c ^K shortcut key.

  \return 0 if the operation did not change the clipboard
  \see copy(int), cut()
*/
int fltk3::Input_::copy_cuts()
{
	// put the yank buffer into the X clipboard
	if (!yankcut || input_type()==fltk3::SECRET_INPUT) return 0;
	fltk3::copy(undobuffer, yankcut, 1);
	return 1;
}

/** \internal
  Checks the when() field and does a callback if indicated.
*/
void fltk3::Input_::maybe_do_callback()
{
	if (changed() || (when()&fltk3::WHEN_NOT_CHANGED)) {
		do_callback();
	}
}

/**
  Handles all kinds of text field related events.

  This is called by derived classes.
  \todo Add comment and parameters
*/
int fltk3::Input_::handletext(int event, int X, int Y, int W, int H)
{
	switch (event) {

	case fltk3::ENTER:
	case fltk3::MOVE:
		if (active_r() && window()) window()->cursor(fltk3::CURSOR_INSERT);
		return 1;

	case fltk3::LEAVE:
		if (active_r() && window()) window()->cursor(fltk3::CURSOR_DEFAULT);
		return 1;

	case fltk3::FOCUS:
		fltk3::set_spot(textfont(), textsize(), x(), y(), w(), h(), window());
		if (mark_ == position_) {
			minimal_update(size()+1);
		} else //if (fltk3::selection_owner() != this)
			minimal_update(mark_, position_);
		return 1;

	case fltk3::UNFOCUS:
		if (active_r() && window()) window()->cursor(fltk3::CURSOR_DEFAULT);
		if (mark_ == position_) {
			if (!(damage()&fltk3::DAMAGE_EXPOSE)) {
				minimal_update(position_);
				erase_cursor_only = 1;
			}
		} else //if (fltk3::selection_owner() != this)
			minimal_update(mark_, position_);
	case fltk3::HIDE:
		fltk3::reset_spot();
		if (!readonly() && (when() & fltk3::WHEN_RELEASE))
			maybe_do_callback();
		return 1;

	case fltk3::PUSH:

		if (active_r() && window()) window()->cursor(fltk3::CURSOR_INSERT);

		handle_mouse(X, Y, W, H, fltk3::event_state(fltk3::SHIFT));

		if (fltk3::focus() != this) {
			fltk3::focus(this);
			handle(fltk3::FOCUS);
		}
		return 1;

	case fltk3::DRAG:
		handle_mouse(X, Y, W, H, 1);
		return 1;

	case fltk3::RELEASE:
		copy(0);
		return 1;

	case fltk3::PASTE: {
		// Don't allow pastes into readonly widgets...
		if (readonly()) {
			fltk3::beep(fltk3::BEEP_ERROR);
			return 1;
		}

		// See if we have anything to paste...
		if (!fltk3::event_text() || !fltk3::event_length()) return 1;

		// strip trailing control characters and spaces before pasting:
		const char* t = fltk3::event_text();
		const char* e = t+fltk3::event_length();
		if (input_type() != fltk3::MULTILINE_INPUT) while (e > t && isspace(*(e-1) & 255)) e--;
		if (!t || e <= t) return 1; // Int/float stuff will crash without this test
		if (input_type() == fltk3::INT_INPUT) {
			while (isspace(*t & 255) && t < e) t ++;
			const char *p = t;
			if (*p == '+' || *p == '-') p ++;
			if (strncmp(p, "0x", 2) == 0) {
				p += 2;
				while (isxdigit(*p & 255) && p < e) p ++;
			} else {
				while (isdigit(*p & 255) && p < e) p ++;
			}
			if (p < e) {
				fltk3::beep(fltk3::BEEP_ERROR);
				return 1;
			} else return replace(0, size(), t, (int)(e - t));
		} else if (input_type() == fltk3::FLOAT_INPUT) {
			while (isspace(*t & 255) && t < e) t ++;
			const char *p = t;
			if (*p == '+' || *p == '-') p ++;
			while (isdigit(*p & 255) && p < e) p ++;
			if (*p == '.') {
				p ++;
				while (isdigit(*p & 255) && p < e) p ++;
				if (*p == 'e' || *p == 'E') {
					p ++;
					if (*p == '+' || *p == '-') p ++;
					while (isdigit(*p & 255) && p < e) p ++;
				}
			}
			if (p < e) {
				fltk3::beep(fltk3::BEEP_ERROR);
				return 1;
			} else return replace(0, size(), t, (int)(e - t));
		}
		return replace(position(), mark(), t, (int)(e-t));
	}

	case fltk3::SHORTCUT:
		if (!(shortcut() ? fltk3::test_shortcut(shortcut()) : test_shortcut()))
			return 0;
		if (fltk3::visible_focus() && handle(fltk3::FOCUS)) {
			fltk3::focus(this);
			return 1;
		} // else fall through

	default:
		return 0;
	}
}

/*------------------------------*/

/**
  Creates a new fltk3::Input_ widget.

  This function creates a new fltk3::Input_ widget and adds it to the current
  fltk3::Group. The value() is set to \c NULL.
  The default boxtype is \c fltk3::DOWN_BOX.

  \param X, Y, W, H the dimensions of the new widget
  \param l an optional label text
*/
fltk3::Input_::Input_(int X, int Y, int W, int H, const char* l)
	: fltk3::Widget(X, Y, W, H, l)
{
	box(fltk3::DOWN_BOX);
	color(fltk3::BACKGROUND2_COLOR, fltk3::SELECTION_COLOR);
	align(fltk3::ALIGN_LEFT);
	textsize(fltk3::NORMAL_SIZE);
	textfont(fltk3::HELVETICA);
	textcolor(fltk3::FOREGROUND_COLOR);
	cursor_color_ = fltk3::FOREGROUND_COLOR; // was fltk3::BLUE
	mark_ = position_ = size_ = 0;
	bufsize = 0;
	buffer  = 0;
	value_ = "";
	xscroll_ = yscroll_ = 0;
	maximum_size_ = 32767;
	shortcut_ = 0;
	set_flag(SHORTCUT_LABEL);
	set_flag(MAC_USE_ACCENTS_MENU);
	tab_nav(1);
}

/**
 Copies the value from a possibly static entry into the internal buffer.

 \param [in] len size of the current text
*/
void fltk3::Input_::put_in_buffer(int len)
{
	if (value_ == buffer && bufsize > len) {
		buffer[size_] = 0;
		return;
	}
	if (!bufsize) {
		if (len > size_) len += 9; // let a few characters insert before realloc
		bufsize = len+1;
		buffer = (char*)malloc(bufsize);
	} else if (bufsize <= len) {
		// we may need to move old value in case it points into buffer:
		int moveit = (value_ >= buffer && value_ < buffer+bufsize);
		// enlarge current buffer
		if (len > size_) {
			do {
				bufsize *= 2;
			} while (bufsize <= len);
		} else {
			bufsize = len+1;
		}
		// Note: the following code is equivalent to:
		//
		//   if (moveit) value_ = value_ - buffer;
		//   char* nbuffer = (char*)realloc(buffer, bufsize);
		//   if (moveit) value_ = value_ + nbuffer;
		//   buffer = nbuffer;
		//
		// We just optimized the pointer arithmetic for value_...
		//
		char* nbuffer = (char*)realloc(buffer, bufsize);
		if (moveit) value_ += (nbuffer-buffer);
		buffer = nbuffer;
	}
	memmove(buffer, value_, size_);
	buffer[size_] = 0;
	value_ = buffer;
}

/**
  Changes the widget text.

  This function changes the text and sets the mark and the point to
  the end of it. The string is \e not copied. If the user edits the
  string it is copied to the internal buffer then. This can save a
  great deal of time and memory if your program is rapidly
  changing the values of text fields, but this will only work if
  the passed string remains unchanged until either the
  fltk3::Input is destroyed or value() is called again.

  You can use the \p len parameter to directly set the length
  if you know it already or want to put \c nul characters in the text.

  \param [in] str the new text
  \param [in] len the length of the new text
  \return non-zero if the new value is different than the current one
*/
int fltk3::Input_::static_value(const char* str, int len)
{
	clear_changed();
	if (undowidget == this) undowidget = 0;
	if (str == value_ && len == size_) return 0;
	if (len) { // non-empty new value:
		if (xscroll_ || yscroll_) {
			xscroll_ = yscroll_ = 0;
			minimal_update(0);
		} else {
			int i = 0;
			// find first different character:
			if (value_) {
				for (; i<size_ && i<len && str[i]==value_[i]; i++);
				if (i==size_ && i==len) return 0;
			}
			minimal_update(i);
		}
		value_ = str;
		size_ = len;
	} else { // empty new value:
		if (!size_) return 0; // both old and new are empty.
		size_ = 0;
		value_ = "";
		xscroll_ = yscroll_ = 0;
		minimal_update(0);
	}
	position(readonly() ? 0 : size());
	return 1;
}

/**
  Changes the widget text.

  This function changes the text and sets the mark and the point to
  the end of it. The string is \e not copied. If the user edits the
  string it is copied to the internal buffer then. This can save a
  great deal of time and memory if your program is rapidly
  changing the values of text fields, but this will only work if
  the passed string remains unchanged until either the
  fltk3::Input is destroyed or value() is called again.

  \param [in] str the new text
  \return non-zero if the new value is different than the current one
*/
int fltk3::Input_::static_value(const char* str)
{
	return static_value(str, str ? (int)strlen(str) : 0);
}

/**
  Changes the widget text.

  This function changes the text and sets the mark and the
  point to the end of it. The string is copied to the internal
  buffer. Passing \c NULL is the same as "".

  You can use the \p length parameter to directly set the length
  if you know it already or want to put \c nul characters in the text.

  \param [in] str the new text
  \param [in] len the length of the new text
  \return non-zero if the new value is different than the current one
  \see fltk3::Input_::value(const char* str), fltk3::Input_::value()
*/
int fltk3::Input_::value(const char* str, int len)
{
	int r = static_value(str, len);
	if (len) put_in_buffer(len);
	return r;
}

/**
  Changes the widget text.

  This function changes the text and sets the mark and the
  point to the end of it. The string is copied to the internal
  buffer. Passing \c NULL is the same as \c "".

  \param [in] str the new text
  \return non-zero if the new value is different than the current one
  \see fltk3::Input_::value(const char* str, int len), fltk3::Input_::value()
*/
int fltk3::Input_::value(const char* str)
{
	return value(str, str ? (int)strlen(str) : 0);
}

/**
  Changes the size of the widget.
  This call updates the text layout so that the cursor is visible.
  \param [in] X, Y, W, H new size of the widget
  \see fltk3::Widget::resize(int, int, int, int)
*/
void fltk3::Input_::resize(int X, int Y, int W, int H)
{
	if (W != w()) xscroll_ = 0;
	if (H != h()) yscroll_ = 0;
	Widget::resize(X, Y, W, H);
}

/**
  Destroys the widget.

  The destructor clears all allocated buffers and removes the widget
  from the parent fltk3::Group.
*/
fltk3::Input_::~Input_()
{
	if (undowidget == this) undowidget = 0;
	if (bufsize) free((void*)buffer);
}

/** \internal
  Returns the number of lines displayed on a single page.
  \return widget height divided by the font height
*/
int fltk3::Input_::linesPerPage()
{
	int n = 1;
	if (input_type() == fltk3::MULTILINE_INPUT) {
		fltk3::font(textfont(),textsize()); //ensure current font is set to ours
		n = h()/fltk3::height(); // number of lines to scroll
		if (n<=0) n = 1;
	}
	return n;
}

/**
  Returns the character at index \p i.

  This function returns the UTF-8 character at \p i
  as a ucs4 character code.

  \param [in] i index into the value field
  \return the character at index \p i
*/
unsigned int fltk3::Input_::index(int i) const
{
	int len = 0;
	return fltk3::utf8decode(value_+i, value_+size_, &len);
}

//
// End of "$Id: Input_.cxx 9818 2013-02-09 14:16:47Z manolo $".
//
