//
// "$Id: ToggleButton.h 9638 2012-07-24 04:41:38Z matt $"
//
// Toggle button header file for the Fast Light Tool Kit (FLTK).
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
 fltk3::ToggleButton widget . */

#ifndef Fltk3_Toggle_Button_H
#define Fltk3_Toggle_Button_H

#include "Button.h"

namespace fltk3
{
/**
 The toggle button is a push button that needs to be clicked once
 to toggle on, and one more time to toggle off.
 The fltk3::ToggleButton subclass displays the "on" state by
 drawing a pushed-in button.</P>
 <P>Buttons generate callbacks when they are clicked by the user.  You
 control exactly when and how by changing the values for type()
 and when().
 */
class FLTK3_EXPORT ToggleButton : public Button
{
public:
	/**
	 Creates a new fltk3::ToggleButton widget using the given
	 position, size, and label string.
	 <P>The inherited destructor deletes the toggle button.
	 */
	ToggleButton(int X,int Y,int W,int H,const char *l=0) : Button(X,Y,W,H,l) { type(TOGGLE_BUTTON); }
};

}

#endif

//
// End of "$Id: ToggleButton.h 9638 2012-07-24 04:41:38Z matt $".
//
