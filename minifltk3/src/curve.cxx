//
// "$Id: curve.cxx 9011 2011-08-25 21:50:59Z matt $"
//
// Bezier curve functions for the Fast Light Tool Kit (FLTK).
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

/**
  \file fl_curve.cxx
  \brief Utility for drawing Bezier curves, adding the points to the
         current fl_begin/fltk3::vertex/fl_end path.

  Incremental math implementation:
  I very much doubt this is optimal!  From Foley/vanDam page 511.
  If anybody has a better algorithm, please send it!
*/

#include "draw.h"
#include <math.h>

void fltk3::GraphicsDriver::curve(double X0, double Y0,
                                  double X1, double Y1,
                                  double X2, double Y2,
                                  double X3, double Y3)
{

	double x = fltk3::transform_x(X0,Y0);
	double y = fltk3::transform_y(X0,Y0);

	// draw point 0:
	fltk3::transformed_vertex(x,y);

	double x1 = fltk3::transform_x(X1,Y1);
	double yy1 = fltk3::transform_y(X1,Y1);
	double x2 = fltk3::transform_x(X2,Y2);
	double y2 = fltk3::transform_y(X2,Y2);
	double x3 = fltk3::transform_x(X3,Y3);
	double y3 = fltk3::transform_y(X3,Y3);

	// find the area:
	double a = fabs((x-x2)*(y3-yy1)-(y-y2)*(x3-x1));
	double b = fabs((x-x3)*(y2-yy1)-(y-y3)*(x2-x1));
	if (b > a) a = b;

	// use that to guess at the number of segments:
	int n = int(sqrt(a)/4);
	if (n > 1) {
		if (n > 100) n = 100; // make huge curves not hang forever

		double e = 1.0/n;

		// calculate the coefficients of 3rd order equation:
		double xa = (x3-3*x2+3*x1-x);
		double xb = 3*(x2-2*x1+x);
		double xc = 3*(x1-x);
		// calculate the forward differences:
		double dx1 = ((xa*e+xb)*e+xc)*e;
		double dx3 = 6*xa*e*e*e;
		double dx2 = dx3 + 2*xb*e*e;

		// calculate the coefficients of 3rd order equation:
		double ya = (y3-3*y2+3*yy1-y);
		double yb = 3*(y2-2*yy1+y);
		double yc = 3*(yy1-y);
		// calculate the forward differences:
		double dy1 = ((ya*e+yb)*e+yc)*e;
		double dy3 = 6*ya*e*e*e;
		double dy2 = dy3 + 2*yb*e*e;

		// draw points 1 .. n-2:
		for (int m=2; m<n; m++) {
			x += dx1;
			dx1 += dx2;
			dx2 += dx3;
			y += dy1;
			dy1 += dy2;
			dy2 += dy3;
			fltk3::transformed_vertex(x,y);
		}

		// draw point n-1:
		fltk3::transformed_vertex(x+dx1, y+dy1);
	}

	// draw point n:
	fltk3::transformed_vertex(x3,y3);
}

//
// End of "$Id: curve.cxx 9011 2011-08-25 21:50:59Z matt $".
//
