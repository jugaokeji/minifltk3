//
// "$Id: SingleWindow.h 9638 2012-07-24 04:41:38Z matt $"
//
// Single-buffered window header file for the Fast Light Tool Kit (FLTK).
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
 fltk3::SingleWindow class . */

#ifndef Fltk3_Single_Window_H
#define Fltk3_Single_Window_H

#include "Window.h"

namespace fltk3
{

/**
 This is the same as fltk3::Window.  However, it is possible that
 some implementations will provide double-buffered windows by default.
 This subcan be used to force single-buffering.  This may be
 useful for modifying existing programs that use incremental update, or
 for some types of image data, such as a movie flipbook.
 */
class FLTK3_EXPORT SingleWindow : public fltk3::Window
{
public:
	void show() {
		Window::show();
	}
	void show(int a, char **b) {
		Window::show(a,b);
	}
	void flush() {
		Window::flush();
	}
	/**
	 Creates a new fltk3::SingleWindow widget using the given
	 size, and label (title) string.
	 */
	SingleWindow(int W, int H, const char *l=0) : fltk3::Window(W,H,l) {}
	/**
	 Creates a new fltk3::SingleWindow widget using the given
	 position, size, and label (title) string.
	 */
	SingleWindow(int X, int Y, int W, int H, const char *l=0): fltk3::Window(X,Y,W,H,l) {}
	int make_current();
};

}

#endif

//
// End of "$Id: SingleWindow.h 9638 2012-07-24 04:41:38Z matt $".
//
