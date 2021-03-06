//
// "$Id: Dial.h 9300 2012-03-23 20:36:00Z ianmacarthur $"
//
// Dial header file for the Fast Light Tool Kit (FLTK).
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
 fltk3::Dial widget . */

#ifndef Fltk3_Dial_H
#define Fltk3_Dial_H

#include "Valuator.h"
#include "run.h"
#include "draw.h"
#include <stdlib.h>
#include "fltkmath.h"


namespace fltk3
{

// values for type():
const uchar NORMAL_DIAL = 0;	/**< type() for dial variant with dot */
const uchar LINE_DIAL   = 1;	/**< type() for dial variant with line */
const uchar FILL_DIAL   = 2;	/**< type() for dial variant with filled arc */

/**
 The fltk3::Dial widget provides a circular dial to control a
 single floating point value.
 <P ALIGN=CENTER>\image html dial.png
 \image latex dial.png "fltk3::Dial" width=4cm
 Use type() to set the type of the dial to:
 <UL>
 <LI>fltk3::NORMAL_DIAL - Draws a normal dial with a knob. </LI>
 <LI>fltk3::LINE_DIAL - Draws a dial with a line. </LI>
 <LI>fltk3::FILL_DIAL - Draws a dial with a filled arc. </LI>
 </UL>

 */
class FLTK3_EXPORT Dial : public fltk3::Valuator
{

	short a1,a2;

protected:

	// these allow subclasses to put the dial in a smaller area:
	void draw(int X, int Y, int W, int H) {
		if (damage()&fltk3::DAMAGE_ALL) draw_box(box(), X, Y, W, H, color());
		X += fltk3::box_dx(box());
		Y += fltk3::box_dy(box());
		W -= fltk3::box_dw(box());
		H -= fltk3::box_dh(box());
		double angle = (a2-a1)*(value()-minimum())/(maximum()-minimum()) + a1;
		if (type() == fltk3::FILL_DIAL) {
			// foo: draw this nicely in certain round box types
			int foo = (box() > fltk3::ROUND_UP_BOX && fltk3::box_dx(box()));
			if (foo) {
				X--;
				Y--;
				W+=2;
				H+=2;
			}
			if (active_r()) fltk3::color(color());
			else fltk3::color(fltk3::inactive(color()));
			fltk3::pie(X, Y, W, H, 270-a1, angle > a1 ? 360+270-angle : 270-360-angle);
			if (active_r()) fltk3::color(selection_color());
			else fltk3::color(fltk3::inactive(selection_color()));
			fltk3::pie(X, Y, W, H, 270-angle, 270-a1);
			if (foo) {
				if (active_r()) fltk3::color(fltk3::FOREGROUND_COLOR);
				else fltk3::color(fltk3::inactive(fltk3::FOREGROUND_COLOR));
				fltk3::arc(X, Y, W, H, 0, 360);
			}
			return;
		}
		if (!(damage()&fltk3::DAMAGE_ALL)) {
			if (active_r()) fltk3::color(color());
			else fltk3::color(fltk3::inactive(color()));
			fltk3::pie(X+1, Y+1, W-2, H-2, 0, 360);
		}
		fltk3::push_matrix();
		fltk3::translate(X+W/2-.5, Y+H/2-.5);
		fltk3::scale(W-1, H-1);
		fltk3::rotate(45-angle);
		if (active_r()) fltk3::color(selection_color());
		else fltk3::color(fltk3::inactive(selection_color()));
		if (type()) { // fltk3::LINE_DIAL
			fltk3::begin_polygon();
			fltk3::vertex(0.0,   0.0);
			fltk3::vertex(-0.04, 0.0);
			fltk3::vertex(-0.25, 0.25);
			fltk3::vertex(0.0,   0.04);
			fltk3::end_polygon();
			if (active_r()) fltk3::color(fltk3::FOREGROUND_COLOR);
			else fltk3::color(fltk3::inactive(fltk3::FOREGROUND_COLOR));
			fltk3::begin_loop();
			fltk3::vertex(0.0,   0.0);
			fltk3::vertex(-0.04, 0.0);
			fltk3::vertex(-0.25, 0.25);
			fltk3::vertex(0.0,   0.04);
			fltk3::end_loop();
		} else {
			fltk3::begin_polygon();
			fltk3::circle(-0.20, 0.20, 0.07);
			fltk3::end_polygon();
			if (active_r()) fltk3::color(fltk3::FOREGROUND_COLOR);
			else fltk3::color(fltk3::inactive(fltk3::FOREGROUND_COLOR));
			fltk3::begin_loop();
			fltk3::circle(-0.20, 0.20, 0.07);
			fltk3::end_loop();
		}
		fltk3::pop_matrix();
	}
	int handle(int event, int X, int Y, int W, int H) {
		switch (event) {
		case fltk3::PUSH: {
			fltk3::WidgetTracker wp(this);
			handle_push();
			if (wp.deleted()) return 1;
		}
		case fltk3::DRAG: {
			int mx = (fltk3::event_x()-X-W/2)*H;
			int my = (fltk3::event_y()-Y-H/2)*W;
			if (!mx && !my) return 1;
			double angle = 270-atan2((float)-my, (float)mx)*180/M_PI;
			double oldangle = (a2-a1)*(value()-minimum())/(maximum()-minimum()) + a1;
			while (angle < oldangle-180) angle += 360;
			while (angle > oldangle+180) angle -= 360;
			double val;
			if ((a1<a2) ? (angle <= a1) : (angle >= a1)) {
				val = minimum();
			} else if ((a1<a2) ? (angle >= a2) : (angle <= a2)) {
				val = maximum();
			} else {
				val = minimum() + (maximum()-minimum())*(angle-a1)/(a2-a1);
			}
			handle_drag(clamp(round(val)));
		}
		return 1;
		case fltk3::RELEASE:
			handle_release();
			return 1;
		case fltk3::ENTER : /* FALLTHROUGH */
		case fltk3::LEAVE :
			return 1;
		default:
			return 0;
		}
	}
	void draw() {
		draw(0, 0, w(), h());
		draw_label();
	}

public:
	int handle(int e) {
		return handle(e, 0, 0, w(), h());
	}
	/**
	 Creates a new fltk3::Dial widget using the given position, size,
	 and label string. The default type is fltk3::NORMAL_DIAL.
	 */
	Dial(int X, int Y, int W, int H, const char *l = 0) : fltk3::Valuator(X, Y, W, H, l) {
		box(fltk3::OVAL_BOX);
		selection_color(fltk3::INACTIVE_COLOR); // was 37
		a1 = 45;
		a2 = 315;
	}
	/**
	 Sets Or gets the angles used for the minimum and maximum values.  The default
	 values are 45 and 315 (0 degrees is straight down and the angles
	 progress clockwise).  Normally angle1 is less than angle2, but if you
	 reverse them the dial moves counter-clockwise.
	 */
	short angle1() const {
		return a1;
	}
	/** See short angle1() const */
	void angle1(short a) {
		a1 = a;
	}
	/** See short angle1() const */
	short angle2() const {
		return a2;
	}
	/** See short angle1() const */
	void angle2(short a) {
		a2 = a;
	}
	/** See short angle1() const */
	void angles(short a, short b) {
		a1 = a;
		a2 = b;
	}

};

}

#endif

//
// End of "$Id: Dial.h 9300 2012-03-23 20:36:00Z ianmacarthur $".
//
