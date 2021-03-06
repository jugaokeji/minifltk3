//
// "$Id: FileInput.cxx 9676 2012-08-18 11:23:23Z matt $"
//
// File_Input header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
// Original version Copyright 1998 by Curtis Edwards.
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
#include "FileInput.h"
#include "Window.h"
#include "draw.h"
#include "filename.h"
#include <stdio.h>
#include "flstring.h"


//
// Height of directory buttons...
//

#define DIR_HEIGHT	10


//
// Redraw bit for directory bar...
//

namespace fltk3
{
const unsigned int DAMAGE_BAR = 0x10;
}


/**
  Creates a new fltk3::FileInput widget using the given position,
  size, and label string. The default boxtype is fltk3::DOWN_BOX.
  \param[in] X, Y, W, H position and size of the widget
  \param[in] L widget label, default is no label
*/
fltk3::FileInput::FileInput(int X, int Y, int W, int H, const char *L)
	: fltk3::Input(X, Y, W, H, L)
{
	buttons_[0] = 0;
	errorcolor_ = fltk3::RED;
	ok_entry_   = 1;
	pressed_    = -1;

	down_box(fltk3::UP_BOX);
}

/**
  Draw directory buttons.
*/
void fltk3::FileInput::draw_buttons()
{
	int	i,					// Looping var
	        X;					// Current X position


	if (damage() & (fltk3::DAMAGE_BAR | fltk3::DAMAGE_ALL)) {
		update_buttons();
	}

	for (X = 0, i = 0; buttons_[i]; i ++) {
		if ((X + buttons_[i]) > xscroll()) {
			if (X < xscroll()) {
				//draw_box(pressed_ == i ? fltk3::down(down_box()) : down_box(), 0, 0, X + buttons_[i] - xscroll(), DIR_HEIGHT, fltk3::GRAY);
				draw_box(pressed_ == i ? down_box()->downbox() : down_box(), 0, 0, X + buttons_[i] - xscroll(), DIR_HEIGHT, fltk3::GRAY);
			} else if ((X + buttons_[i] - xscroll()) > w()) {
				draw_box(pressed_ == i ? down_box()->downbox() : down_box(), 0 + X - xscroll(), 0, w() - X + xscroll(), DIR_HEIGHT, fltk3::GRAY);
			} else {
				draw_box(pressed_ == i ? down_box()->downbox() : down_box(), 0 + X - xscroll(), 0, buttons_[i], DIR_HEIGHT, fltk3::GRAY);
			}
		}

		X += buttons_[i];
	}

	if (X < w()) {
		draw_box(pressed_ == i ? down_box()->downbox() : down_box(), X - xscroll(), 0, w() - X + xscroll(), DIR_HEIGHT, fltk3::GRAY);
	}
}

/**
  Update the sizes of the directory buttons.
*/
void fltk3::FileInput::update_buttons()
{
	int		i;				// Looping var
	const char	*start,				// Start of path component
	           *end;				// End of path component


//  puts("update_buttons()");

	// Set the current font & size...
	fltk3::font(textfont(), textsize());

	// Loop through the value string, setting widths...
	for (i = 0, start = value();
	     start && i < (int)(sizeof(buttons_) / sizeof(buttons_[0]) - 1);
	     start = end, i ++) {
//    printf("    start = \"%s\"\n", start);
		if ((end = strchr(start, '/')) == NULL)
#if defined(WIN32) || defined(__EMX__)
			if ((end = strchr(start, '\\')) == NULL)
#endif // WIN32 || __EMX__
				break;

		end ++;

		buttons_[i] = (short)fltk3::width(start, (int)(end - start));
		if (!i) buttons_[i] += fltk3::box_dx(box()) + 6;
	}

//  printf("    found %d components/buttons...\n", i);

	buttons_[i] = 0;
}


/**
  Sets the value of the widget given a new string value and its length.
  Returns non 0 on success.
  \param[in] str new string value
  \param[in] len lengh of value
*/
int						// O - TRUE on success
fltk3::FileInput::value(const char *str,		// I - New string value
                        int        len)  		// I - Length of value
{
	damage(fltk3::DAMAGE_BAR);
	return Input::value(str,len);
}


/**
  Sets the value of the widget given a new string value.
  Returns non 0 on success.
  \param[in] str new string value
*/
int						// O - TRUE on success
fltk3::FileInput::value(const char *str)  		// I - New string value
{
	damage(fltk3::DAMAGE_BAR);
	return Input::value(str);
}


/**
  Draws the file input widget
*/
void fltk3::FileInput::draw()
{
	fltk3::Box* b = box();
	if (damage() & (fltk3::DAMAGE_BAR | fltk3::DAMAGE_ALL)) draw_buttons();
	// this flag keeps fltk3::Input_::drawtext from drawing a bogus box!
	char must_trick_fl_input_ =
	        fltk3::focus()!=this && !size() && !(damage()&fltk3::DAMAGE_ALL);
	if ((damage() & fltk3::DAMAGE_ALL) || must_trick_fl_input_)
		draw_box(b,0,DIR_HEIGHT,w(),h()-DIR_HEIGHT,color());
	if (!must_trick_fl_input_)
		Input_::drawtext(fltk3::box_dx(b)+3, fltk3::box_dy(b)+DIR_HEIGHT,
		                 w()-fltk3::box_dw(b)-6, h()-fltk3::box_dh(b)-DIR_HEIGHT);
}



/**
  Handle events in the widget.
  Return non zero if event is handled.
  \param[in] event
*/
int						// O - TRUE if we handled event
fltk3::FileInput::handle(int event) 		// I - Event
{
//  printf("handle(event = %d)\n", event);
	static char inButtonBar = 0;

	switch (event) {
	case fltk3::MOVE :
	case fltk3::ENTER :
		if (active_r()) {
			if (fltk3::event_y() < (DIR_HEIGHT))
				window()->cursor(fltk3::CURSOR_DEFAULT);
			else
				window()->cursor(fltk3::CURSOR_INSERT);
		}

		return 1;

	case fltk3::PUSH :
		inButtonBar = (fltk3::event_y() < (DIR_HEIGHT));
	case fltk3::RELEASE :
	case fltk3::DRAG :
		if (inButtonBar)
			return handle_button(event);
		else
			return Input::handle(event);

	default : {
		fltk3::WidgetTracker wp(this);
		if (Input::handle(event)) {
			if (wp.exists())
				damage(fltk3::DAMAGE_BAR);
			return 1;
		}
	}
	return 0;
	}
}



/**
  Handles button events in the widget.
  Return non zero if event is handled.
  \param[in] event
*/
int						// O - TRUE if we handled event
fltk3::FileInput::handle_button(int event)		// I - Event
{
	int		i,				// Looping var
	             X;				// Current X position
	char		*start,				// Start of path component
	             *end;				// End of path component
	char		newvalue[FLTK3_PATH_MAX];		// New value


	// Figure out which button is being pressed...
	for (X = 0, i = 0; buttons_[i]; i ++) {
		X += buttons_[i];

		if (X > xscroll() && fltk3::event_x() < (X - xscroll())) break;
	}

//  printf("handle_button(event = %d), button = %d\n", event, i);

	// Redraw the directory bar...
	if (event == fltk3::RELEASE) pressed_ = -1;
	else pressed_ = (short)i;

	//window()->make_current();
	//draw_buttons();
	damage(fltk3::DAMAGE_BAR);

	// Return immediately if the user is clicking on the last button or
	// has not released the mouse button...
	if (!buttons_[i] || event != fltk3::RELEASE) return 1;

	// Figure out where to truncate the path...
	strlcpy(newvalue, value(), sizeof(newvalue));

	for (start = newvalue, end = start; start && i >= 0; start = end, i --) {
//    printf("    start = \"%s\"\n", start);
		if ((end = strchr(start, '/')) == NULL)
#if defined(WIN32) || defined(__EMX__)
			if ((end = strchr(start, '\\')) == NULL)
#endif // WIN32 || __EMX__
				break;

		end ++;
	}

	if (i < 0) {
		// Found the end; truncate the value and update the buttons...
		*start = '\0';
		value(newvalue, (int)(start - newvalue));

		// Then do the callbacks, if necessary...
		set_changed();
		if (when() & (fltk3::WHEN_CHANGED|fltk3::WHEN_RELEASE) ) do_callback();
	}

	return 1;
}


//
// End of "$Id: FileInput.cxx 9676 2012-08-18 11:23:23Z matt $".
//
