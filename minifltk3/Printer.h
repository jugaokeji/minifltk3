//
// "$Id: Printer.h 9300 2012-03-23 20:36:00Z ianmacarthur $"
//
// Printing support for the Fast Light Tool Kit (FLTK).
//
// Copyright 2010-2011 by Bill Spitzak and others.
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
/** \file Printer.h
 \brief declaration of classes fltk3::Printer, fltk3::SystemPrinter and fltk3::PostScriptPrinter.
 */

#ifndef Fltk3_Printer_H
#define Fltk3_Printer_H

#include "x.h"
#include "PagedDevice.h"
#include "draw.h"
#include "Pixmap.h"
#include "Image.h"
#include "Bitmap.h"
#include <stdio.h>
#if !(defined(__APPLE__) || defined(WIN32))
#include "PostScript.h"
#elif defined(WIN32)
#include <commdlg.h>
#endif

namespace fltk3
{
class Printer;

#if defined(__APPLE__) || defined(WIN32) || defined(FLTK3_DOXYGEN)
/**
 Print support under MSWindows and Mac OS.

 Class fltk3::SystemPrinter is implemented only on the MSWindows and Mac OS platforms.
 It has no public constructor.
 Use fltk3::Printer instead that is cross-platform and has the same API.
 */
class SystemPrinter : public fltk3::PagedDevice
{
	friend class fltk3::Printer;
private:
	/** \brief the printer's graphics context, if there's one, NULL otherwise */
	void *gc;
	void set_current(void);
#ifdef __APPLE__
	float scale_x;
	float scale_y;
	float angle; // rotation angle in radians
	PMPrintSession  printSession;
	PMPageFormat    pageFormat;
	PMPrintSettings printSettings;
#elif defined(WIN32)
	int   abortPrint;
	PRINTDLG      pd;
	HDC           hPr;
	int           prerr;
	int left_margin;
	int top_margin;
	void absolute_printable_rect(int *x, int *y, int *w, int *h);
#endif
protected:
	/** \brief The constructor */
	SystemPrinter(void);
public:
	int start_job(int pagecount, int *frompage = NULL, int *topage = NULL);
	int start_page (void);
	int printable_rect(int *w, int *h);
	void margins(int *left, int *top, int *right, int *bottom);
	void origin(int *x, int *y);
	void origin(int x, int y);
	void scale (float scale_x, float scale_y = 0.);
	void rotate(float angle);
	void translate(int x, int y);
	void untranslate(void);
	int end_page (void);
	void end_job (void);
	/** \brief The destructor */
	~SystemPrinter(void);
}; // class fltk3::SystemPrinter

#endif

#if !(defined(__APPLE__) || defined(WIN32) )
/**
 Print support under Unix/Linux.

 Class fltk3::PostScriptPrinter is implemented only on the Unix/Linux platform.
 It has no public constructor.
 Use fltk3::Printer instead that is cross-platform and has the same API.
 */
class PostScriptPrinter : public fltk3::PostScriptFileDevice
{
	friend class fltk3::Printer;
protected:
	/** The constructor */
	PostScriptPrinter(void) {};
public:
	int start_job(int pages, int *firstpage = NULL, int *lastpage = NULL);
};

#endif


/**
 * \brief OS-independent print support.
 *
 fltk3::Printer allows to use all FLTK drawing, color, text, and clip functions, and to have them operate
 on printed page(s). There are two main, non exclusive, ways to use it.
 <ul><li>Print any widget (standard, custom, fltk3::Window, fltk3::GLWindow) as it appears
 on screen, with optional translation, scaling and rotation. This is done by calling print_widget(),
 print_window() or print_window_part().
 <li>Use a series of FLTK graphics commands (e.g., font, text, lines, colors, clip, image) to
 compose a page appropriately shaped for printing.
 </ul>
 In both cases, begin by start_job(), start_page(), printable_rect() and origin() calls
 and finish by end_page() and end_job() calls.
 <p><b>Platform specifics</b>
 <ul>
 <li>Unix/Linux platforms:
 Class fltk3::ImageRGB prints but loses its transparency if it has one.
 See class fltk3::PostScriptGraphicsDriver for a description of how UTF-8 strings appear in print.
 Use the static public attributes of this class to set the print dialog to other languages
 than English. For example, the "Printer:" dialog item fltk3::Printer::dialog_printer can be set to French with:
 \code
 fltk3::Printer::dialog_printer = "Imprimante:";
 \endcode
 before creation of the fltk3::Printer object.
 Use fltk3::PostScriptFileDevice::file_chooser_title to customize the title of the file chooser dialog that opens
 when using the "Print To File" option of the print dialog.
 <li>MSWindows platform: Transparent fltk3::ImageRGB 's don't print with exact transparency on most printers.
 fltk3::ImageRGB 's don't rotate() well.
 A workaround is to use the print_window_part() call.
 <li>Mac OS X platform: all graphics requests print as on display.
 </ul>
 */
class FLTK3_EXPORT Printer : public fltk3::PagedDevice
{
public:
	/** \brief The constructor */
	Printer(void);
	int start_job(int pagecount, int *frompage = NULL, int *topage = NULL);
	int start_page(void);
	int printable_rect(int *w, int *h);
	void margins(int *left, int *top, int *right, int *bottom);
	void origin(int *x, int *y);
	void origin(int x, int y);
	void scale(float scale_x, float scale_y = 0.);
	void rotate(float angle);
	void translate(int x, int y);
	void untranslate(void);
	int end_page (void);
	void end_job (void);
	void set_current(void);
	fltk3::GraphicsDriver* driver(void);
	/** \brief The destructor */
	~Printer(void);

	/** \name These attributes are effective under the Xlib platform only.
	 \{
	 */
	static const char *dialog_title;
	static const char *dialog_printer;
	static const char *dialog_range;
	static const char *dialog_copies;
	static const char *dialog_all;
	static const char *dialog_pages;
	static const char *dialog_from;
	static const char *dialog_to;
	static const char *dialog_properties;
	static const char *dialog_copyNo;
	static const char *dialog_print_button;
	static const char *dialog_cancel_button;
	static const char *dialog_print_to_file;
	static const char *property_title;
	static const char *property_pagesize;
	static const char *property_mode;
	static const char *property_use;
	static const char *property_save;
	static const char *property_cancel;
	/** \} */
private:
#if defined(WIN32) || defined(__APPLE__)
	fltk3::SystemPrinter *printer;
#else
	fltk3::PostScriptPrinter *printer;
#endif
};

} // namespace

#endif // Fltk3_Printer_H

//
// End of "$Id: Printer.h 9300 2012-03-23 20:36:00Z ianmacarthur $"
//
