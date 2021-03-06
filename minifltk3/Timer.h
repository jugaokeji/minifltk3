//
// "$Id: Timer.h 9300 2012-03-23 20:36:00Z ianmacarthur $"
//
// Timer header file for the Fast Light Tool Kit (FLTK).
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
 fltk3::Timer widget . */

#ifndef Fltk3_Timer_H
#define Fltk3_Timer_H

#include "Widget.h"

namespace fltk3
{

// values for type():
const uchar NORMAL_TIMER		= 0;
const uchar VALUE_TIMER		= 1;
const uchar HIDDEN_TIMER		= 2;

/**
 This is provided only to emulate the Forms Timer widget.  It works by
 making a timeout callback every 1/5 second.  This is wasteful and
 inaccurate if you just want something to happen a fixed time in the
 future.  You should directly call
 fltk3::add_timeout() instead.
 */
class FLTK3_EXPORT Timer : public fltk3::Widget
{
	static void stepcb(void *);
	void step();
	char on, direction_;
	double delay, total;
	long lastsec,lastusec;
protected:
	void draw();
public:
	int handle(int);
	Timer(uchar t,int x,int y,int w,int h, const char *l);
	~Timer();
	void value(double);
	/** See void fltk3::Timer::value(double)  */
	double value() const {
		return delay>0.0?delay:0.0;
	}
	/**
	 Gets or sets the direction of the timer.  If the direction is zero
	 then the timer will count up, otherwise it will count down from the
	 initial value().
	 */
	char direction() const {
		return direction_;
	}
	/**
	 Gets or sets the direction of the timer.  If the direction is zero
	 then the timer will count up, otherwise it will count down from the
	 initial value().
	 */
	void direction(char d) {
		direction_ = d;
	}
	/**    Gets or sets whether the timer is suspended.  */
	char suspended() const {
		return !on;
	}
	void suspended(char d);
};

}

#endif

//
// End of "$Id: Timer.h 9300 2012-03-23 20:36:00Z ianmacarthur $".
//

