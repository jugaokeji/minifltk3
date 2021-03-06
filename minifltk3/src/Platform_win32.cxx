//
// "$Id: win32.cxx 9626 2012-06-22 23:08:27Z ianmacarthur $"
//
// WIN32-specific code for the Fast Light Tool Kit (FLTK).
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

// This file contains win32-specific code for fltk which is always linked
// in.  Search other files for "WIN32" or filenames ending in _win32.cxx
// for other system-specific code.

// This file must be #include'd in Fl.cxx and not compiled separately.

#ifdef WIN32

#ifndef FLTK3_DOXYGEN
#include "run.h"
#include "utf8.h"
#include "Window.h"
#include "draw.h"
#include "enumerations.h"
#include "Tooltip.h"
#include "PagedDevice.h"
#include "flstring.h"
#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#ifdef __CYGWIN__
#  include <sys/time.h>
#  include <unistd.h>
#endif

#if !defined(NO_TRACK_MOUSE)
#  include <commctrl.h>	// TrackMouseEvent
// fabien: Ms Visual Studio >= 2003 permit embedded lib reference
// that makes fltk use easier as only fltk libs are now requested
// This idea could be extended to fltk libs themselves,
// implementer should then care about DLL linkage flags ...
#  if (_MSC_VER>=1310)
#    pragma comment (lib, "comctl32.lib")
#  endif
#endif

#if defined(__GNUC__)
# include <wchar.h>
#endif

#include <ole2.h>
#include <shellapi.h>

#include "aimm.h"

//
// USE_ASYNC_SELECT - define it if you have WSAAsyncSelect()...
// USE_ASYNC_SELECT is OBSOLETED in 1.3 for the following reasons:
/*
  This feature was supposed to provide an efficient alternative to the current
  polling method, but as it has been discussed (Thanks Albrecht!) :
  - the async mode would imply to change the socket to non blocking mode.
    This can have unexpected side effects for 3rd party apps, especially
    if it is set on-the-fly when socket service is really needed, as it is
    done today and on purpose, but still the 3rd party developer wouldn't easily
    control the sequencing of socket operations.
  - Finer granularity of events furthered by the async select is a plus only
    for socket 3rd party impl., it is simply not needed for the 'light' fltk
    use we make of wsock, so here it would also be a bad point, because of all
    the logic add-ons necessary for using this functionality, without a clear
    benefit.

  So async mode select would not add benefits to fltk, worse, it can slowdown
  fltk because of this finer granularity and instrumentation code to be added
  for async mode proper operation, not mentioning the side effects...
*/

static fltk3::GDIGraphicsDriver fl_gdi_driver;
static fltk3::DisplayDevice fl_gdi_display(&fl_gdi_driver);

// dynamic wsock dll handling api:
#if defined(__CYGWIN__) && !defined(SOCKET)
# define SOCKET int
#endif

// note: winsock2.h has been #include'd in Fl.cxx
#define WSCK_DLL_NAME "WS2_32.DLL"

typedef int (WINAPI* fl_wsk_select_f)(int, fd_set*, fd_set*, fd_set*, const struct timeval*);
typedef int (WINAPI* fl_wsk_fd_is_set_f)(SOCKET, fd_set *);

static HMODULE s_wsock_mod = 0;
static fl_wsk_select_f s_wsock_select = 0;
static fl_wsk_fd_is_set_f fl_wsk_fd_is_set = 0;

static HMODULE get_wsock_mod()
{
	if (!s_wsock_mod) {
		s_wsock_mod = LoadLibrary(WSCK_DLL_NAME);
		if (s_wsock_mod==NULL)
			fltk3::fatal("FLTK Lib Error: %s file not found! Please check your winsock dll accessibility.\n",WSCK_DLL_NAME);
		s_wsock_select = (fl_wsk_select_f) GetProcAddress(s_wsock_mod, "select");
		fl_wsk_fd_is_set = (fl_wsk_fd_is_set_f) GetProcAddress(s_wsock_mod, "__WSAFDIsSet");
	}
	return s_wsock_mod;
}

/*
 * Dynamic linking of imm32.dll
 * This library is only needed for a hand full (four ATM) functions relating to
 * international text rendering and locales. Dynamically loading reduces initial
 * size and link dependencies.
 */
static HMODULE s_imm_module = 0;
typedef HIMC (WINAPI* flTypeImmGetContext)(HWND);
static flTypeImmGetContext flImmGetContext = 0;
typedef BOOL (WINAPI* flTypeImmSetCompositionWindow)(HIMC, LPCOMPOSITIONFORM);
static flTypeImmSetCompositionWindow flImmSetCompositionWindow = 0;
typedef BOOL (WINAPI* flTypeImmReleaseContext)(HWND, HIMC);
static flTypeImmReleaseContext flImmReleaseContext = 0;
typedef BOOL (WINAPI* flTypeImmIsIME)(HKL);
static flTypeImmIsIME flImmIsIME = 0;

static HMODULE get_imm_module()
{
	if (!s_imm_module) {
		s_imm_module = LoadLibrary("IMM32.DLL");
		if (!s_imm_module)
			fltk3::fatal("FLTK Lib Error: IMM32.DLL file not found!\n\n"
			             "Please check your input method manager library accessibility.");
		flImmGetContext = (flTypeImmGetContext)GetProcAddress(s_imm_module, "ImmGetContext");
		flImmSetCompositionWindow = (flTypeImmSetCompositionWindow)GetProcAddress(s_imm_module, "ImmSetCompositionWindow");
		flImmReleaseContext = (flTypeImmReleaseContext)GetProcAddress(s_imm_module, "ImmReleaseContext");
		flImmIsIME = (flTypeImmIsIME)GetProcAddress(s_imm_module, "ImmIsIME");
	}
	return s_imm_module;
}

// USE_TRACK_MOUSE - define NO_TRACK_MOUSE if you don't have
// TrackMouseEvent()...
//
// Now (Dec. 2008) we can assume that current Cygwin/MinGW versions
// support the TrackMouseEvent() function, but WinCE obviously doesn't
// support it (STR 2095). Therefore, USE_TRACK_MOUSE is enabled by
// default, but you can disable it by defining NO_TRACK_MOUSE.
//
// TrackMouseEvent is only used to support window leave notifications
// under Windows. It can be used to get fltk3::LEAVE events, when the
// mouse leaves the _main_ application window (FLTK detects subwindow
// leave events by using normal move events).
//
// Implementation note: If the mouse cursor leaves one subwindow and
// enters another window, then Windows sends a WM_MOUSEMOVE message to
// the new window before it sends a WM_MOUSELEAVE message to the old
// (just left) window. We save the current mouse window in a static variable,
// and if we get a WM_MOUSELEAVE event for the current mouse window, this
// means that the top level window has been left (otherwise we would have
// got another WM_MOUSEMOVE message before).

// #define NO_TRACK_MOUSE

#if !defined(NO_TRACK_MOUSE)
# define USE_TRACK_MOUSE
#endif // NO_TRACK_MOUSE

static fltk3::Window *track_mouse_win=0;	// current TrackMouseEvent() window

// USE_CAPTURE_MOUSE_WIN - this must be defined for TrackMouseEvent to work
// correctly with subwindows - otherwise a single mouse click and release
// (without a move) would generate phantom leave events.
// This defines, if the current mouse window (maybe a subwindow) or the
// main window should get mouse events after pushing (and holding) a mouse
// button, i.e. when dragging the mouse. This is done by calling SetCapture
// (see below).

#ifdef USE_TRACK_MOUSE
#define USE_CAPTURE_MOUSE_WIN
#endif // USE_TRACK_MOUSE

//
// WM_SYNCPAINT is an "undocumented" message, which is finally defined in
// VC++ 6.0.
//

#ifndef WM_SYNCPAINT
#  define WM_SYNCPAINT 0x0088
#endif

#ifndef WM_MOUSELEAVE
#  define WM_MOUSELEAVE 0x02a3
#endif

#ifndef WM_MOUSEWHEEL
#  define WM_MOUSEWHEEL 0x020a
#endif

// This is only defined on Vista and upwards...
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

#ifndef WHEEL_DELTA
#  define WHEEL_DELTA 120	// according to MSDN.
#endif


//
// WM_FLSELECT is the user-defined message that we get when one of
// the sockets has pending data, etc.
//

#define WM_FLSELECT	(WM_APP+1)	// WM_APP is used for hide-window


////////////////////////////////////////////////////////////////
// interface to poll/select call:

// fd's are only implemented for sockets.  Microsoft Windows does not
// have a unified IO system, so it doesn't support select() on files,
// devices, or pipes...
//
// Microsoft provides the Berkeley select() call and an asynchronous
// select function that sends a WIN32 message when the select condition
// exists. However, we don't try to use the asynchronous WSAAsyncSelect()
// any more for good reasons (see above).
//
// A.S. Dec 2009: We got reports that current winsock2.h files define
// POLLIN, POLLOUT, and POLLERR with conflicting values WRT what we
// used before (STR #2301).  Therefore we must not use these values
// for our internal purposes, but use fltk3::READ, fltk3::WRITE, and
// fltk3::EXCEPT, as defined for use in fltk3::add_fd().
//
static int maxfd = 0;
static fd_set fdsets[3];

extern IDropTarget *flIDropTarget;

static int nfds = 0;
static int fd_array_size = 0;
static struct FD {
	int fd;
	short events;
	void (*cb)(fltk3::SOCKET, void*);
	void* arg;
} *fd = 0;

extern unsigned int fl_codepage;

void fltk3::reset_spot()
{
}

void fltk3::set_spot(int font, int size, int X, int Y, int W, int H, fltk3::Window *win)
{
	if (!win) return;
	fltk3::Window* tw = win;
	while (tw->parent()) tw = tw->window(); // find top level window

	get_imm_module();
	HIMC himc = flImmGetContext(fl_xid(tw));

	if (himc) {
		COMPOSITIONFORM cfs;
		cfs.dwStyle = CFS_POINT;
		cfs.ptCurrentPos.x = X;
		cfs.ptCurrentPos.y = Y - tw->labelsize();
		MapWindowPoints(fl_xid(win), fl_xid(tw), &cfs.ptCurrentPos, 1);
		flImmSetCompositionWindow(himc, &cfs);
		flImmReleaseContext(fl_xid(tw), himc);
	}
}

void fltk3::set_status(int x, int y, int w, int h)
{
}

void fltk3::add_fd(int n, int events, void (*cb)(fltk3::SOCKET, void*), void *v)
{
	remove_fd(n,events);
	int i = nfds++;
	if (i >= fd_array_size) {
		fd_array_size = 2*fd_array_size+1;
		fd = (FD*)realloc(fd, fd_array_size*sizeof(FD));
	}
	fd[i].fd = n;
	fd[i].events = (short)events;
	fd[i].cb = cb;
	fd[i].arg = v;

	if (events & fltk3::READ) FD_SET((unsigned)n, &fdsets[0]);
	if (events & fltk3::WRITE) FD_SET((unsigned)n, &fdsets[1]);
	if (events & fltk3::EXCEPT) FD_SET((unsigned)n, &fdsets[2]);
	if (n > maxfd) maxfd = n;
}

void fltk3::add_fd(int fd, void (*cb)(fltk3::SOCKET, void*), void* v)
{
	fltk3::add_fd(fd, fltk3::READ, cb, v);
}

void fltk3::remove_fd(int n, int events)
{
	int i,j;
	for (i=j=0; i<nfds; i++) {
		if (fd[i].fd == n) {
			short e = fd[i].events & ~events;
			if (!e) continue; // if no events left, delete this fd
			fd[i].events = e;
		}
		// move it down in the array if necessary:
		if (j<i) {
			fd[j]=fd[i];
		}
		j++;
	}
	nfds = j;

	if (events & fltk3::READ) FD_CLR(unsigned(n), &fdsets[0]);
	if (events & fltk3::WRITE) FD_CLR(unsigned(n), &fdsets[1]);
	if (events & fltk3::EXCEPT) FD_CLR(unsigned(n), &fdsets[2]);
}

void fltk3::remove_fd(int n)
{
	remove_fd(n, -1);
}

// these pointers are set by the fltk3::lock() function:
static void nothing() {}
void (*fl_lock_function)() = nothing;
void (*fl_unlock_function)() = nothing;

static void* thread_message_;
void* fltk3::thread_message()
{
	void* r = thread_message_;
	thread_message_ = 0;
	return r;
}

IActiveIMMApp *fl_aimm = NULL;
MSG fl_msg;

// This is never called with time_to_wait < 0.0.
// It *should* return negative on error, 0 if nothing happens before
// timeout, and >0 if any callbacks were done.  This version only
// returns zero if nothing happens during a 0.0 timeout, otherwise
// it returns 1.
int fl_wait(double time_to_wait)
{
	int have_message = 0;

	run_checks();

	// idle processing
	static char in_idle;
	if (fltk3::idle && !in_idle) {
		in_idle = 1;
		fltk3::idle();
		in_idle = 0;
	}

	if (nfds) {
		// For WIN32 we need to poll for socket input FIRST, since
		// the event queue is not something we can select() on...
		timeval t;
		t.tv_sec = 0;
		t.tv_usec = 0;

		fd_set fdt[3];
		memcpy(fdt, fdsets, sizeof fdt); // one shot faster fdt init
		if (get_wsock_mod()&& s_wsock_select(maxfd+1,&fdt[0],&fdt[1],&fdt[2],&t)) {
			// We got something - do the callback!
			for (int i = 0; i < nfds; i ++) {
				SOCKET f = fd[i].fd;
				short revents = 0;
				if (fl_wsk_fd_is_set(f, &fdt[0])) revents |= fltk3::READ;
				if (fl_wsk_fd_is_set(f, &fdt[1])) revents |= fltk3::WRITE;
				if (fl_wsk_fd_is_set(f, &fdt[2])) revents |= fltk3::EXCEPT;
				if (fd[i].events & revents) fd[i].cb(f, fd[i].arg);
			}
			time_to_wait = 0.0; // just peek for any messages
		} else {
			// we need to check them periodically, so set a short timeout:
			if (time_to_wait > .001) time_to_wait = .001;
		}
	}

	if (fltk3::idle || fltk3::damage())
		time_to_wait = 0.0;

	// if there are no more windows and this timer is set
	// to FOREVER, continue through or look up indefinitely
	if (!fltk3::first_window() && time_to_wait==1e20)
		time_to_wait = 0.0;

	fl_unlock_function();

	time_to_wait = (time_to_wait > 10000 ? 10000 : time_to_wait);
	int t_msec = (int) (time_to_wait * 1000.0 + 0.5);
	MsgWaitForMultipleObjects(0, NULL, FALSE, t_msec, QS_ALLINPUT);

	fl_lock_function();

	// Execute the message we got, and all other pending messages:
	// have_message = PeekMessage(&fl_msg, NULL, 0, 0, PM_REMOVE);
	have_message = PeekMessageW(&fl_msg, NULL, 0, 0, PM_REMOVE);
	if (have_message > 0) {
		while (have_message != 0 && have_message != -1) {
			if (fl_msg.message == fl_wake_msg) {
				// Used for awaking wait() from another thread
				thread_message_ = (void*)fl_msg.wParam;
				fltk3::AwakeHandler func;
				void *data;
				while (fltk3::get_awake_handler_(func, data)==0) {
					func(data);
				}
			}

			TranslateMessage(&fl_msg);
			DispatchMessageW(&fl_msg);
			have_message = PeekMessageW(&fl_msg, NULL, 0, 0, PM_REMOVE);
		}
	}
	fltk3::flush();

	// This should return 0 if only timer events were handled:
	return 1;
}

// fl_ready() is just like fl_wait(0.0) except no callbacks are done:
int fl_ready()
{
	if (PeekMessage(&fl_msg, NULL, 0, 0, PM_NOREMOVE)) return 1;
	if (!nfds) return 0;
	timeval t;
	t.tv_sec = 0;
	t.tv_usec = 0;
	fd_set fdt[3];
	memcpy(fdt, fdsets, sizeof fdt);
	return get_wsock_mod() ? s_wsock_select(0,&fdt[0],&fdt[1],&fdt[2],&t) : 0;
}

////////////////////////////////////////////////////////////////

int fltk3::x()
{
	RECT r;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
	return r.left;
}

int fltk3::y()
{
	RECT r;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
	return r.top;
}

int fltk3::h()
{
	RECT r;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
	return r.bottom - r.top;
}

int fltk3::w()
{
	RECT r;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
	return r.right - r.left;
}

void fltk3::get_mouse(int &x, int &y)
{
	POINT p;
	GetCursorPos(&p);
	x = p.x;
	y = p.y;
}

////////////////////////////////////////////////////////////////
// code used for selections:

char *fl_selection_buffer[2];
int fl_selection_length[2];
int fl_selection_buffer_length[2];
char fl_i_own_selection[2];

UINT fl_get_lcid_codepage(LCID id)
{
	char buf[8];
	buf[GetLocaleInfo(id, LOCALE_IDEFAULTANSICODEPAGE, buf, 8)] = 0;
	return atol(buf);
}

// Convert \n -> \r\n
class Lf2CrlfConvert
{
	char *out;
	int outlen;
public:
	Lf2CrlfConvert(const char *in, int inlen) {
		outlen = 0;
		const char *i;
		char *o;
		int lencount;
		// Predict size of \r\n conversion buffer
		for ( i=in, lencount = inlen; lencount--; ) {
			if ( *i == '\r' && *(i+1) == '\n' ) {	// leave \r\n untranslated
				i+=2;
				outlen+=2;
			} else if ( *i == '\n' ) {		// \n by itself? leave room to insert \r
				i++;
				outlen+=2;
			} else {
				++i;
				++outlen;
			}
		}
		// Alloc conversion buffer + NULL
		out = new char[outlen+1];
		// Handle \n -> \r\n conversion
		for ( i=in, o=out, lencount = inlen; lencount--; ) {
			if ( *i == '\r' && *(i+1) == '\n' ) {	// leave \r\n untranslated
				*o++ = *i++;
				*o++ = *i++;
			} else if ( *i == '\n' ) {		// \n by itself? insert \r
				*o++ = '\r';
				*o++ = *i++;
			} else {
				*o++ = *i++;
			}
		}
		*o++ = 0;
	}
	~Lf2CrlfConvert() {
		delete[] out;
	}
	int GetLength() const {
		return(outlen);
	}
	const char* GetValue() const {
		return(out);
	}
};

// call this when you create a selection:
void fltk3::copy(const char *stuff, int len, int clipboard)
{
	if (!stuff || len<0) return;

	// Convert \n -> \r\n (for old apps like Notepad, DOS)
	Lf2CrlfConvert buf(stuff, len);
	len = buf.GetLength();
	stuff = buf.GetValue();

	if (len+1 > fl_selection_buffer_length[clipboard]) {
		delete[] fl_selection_buffer[clipboard];
		fl_selection_buffer[clipboard] = new char[len+100];
		fl_selection_buffer_length[clipboard] = len+100;
	}
	memcpy(fl_selection_buffer[clipboard], stuff, len);
	fl_selection_buffer[clipboard][len] = 0; // needed for direct paste
	fl_selection_length[clipboard] = len;
	if (clipboard) {
		// set up for "delayed rendering":
		if (OpenClipboard(NULL)) {
			// if the system clipboard works, use it
			int utf16_len = fltk3::utf8toUtf16(fl_selection_buffer[clipboard], fl_selection_length[clipboard], 0, 0);
			EmptyClipboard();
			HGLOBAL hMem = GlobalAlloc(GHND, utf16_len * 2 + 2); // moveable and zero'ed mem alloc.
			LPVOID memLock = GlobalLock(hMem);
			fltk3::utf8toUtf16(fl_selection_buffer[clipboard], fl_selection_length[clipboard], (unsigned short*) memLock, utf16_len + 1);
			GlobalUnlock(hMem);
			SetClipboardData(CF_UNICODETEXT, hMem);
			CloseClipboard();
			GlobalFree(hMem);
			fl_i_own_selection[clipboard] = 0;
		} else {
			// only if it fails, instruct paste() to use the internal buffers
			fl_i_own_selection[clipboard] = 1;
		}
	}
}

// Call this when a "paste" operation happens:
void fltk3::paste(fltk3::Widget &receiver, int clipboard)
{
	if (!clipboard || fl_i_own_selection[clipboard]) {
		// We already have it, do it quickly without window server.
		// Notice that the text is clobbered if set_selection is
		// called in response to fltk3::PASTE!

		// Convert \r\n -> \n
		char *i = fl_selection_buffer[clipboard];
		if (i==0L) {
			fltk3::e_text = 0;
			return;
		}
		fltk3::e_text = new char[fl_selection_length[clipboard]+1];
		char *o = fltk3::e_text;
		while (*i) {
			if ( *i == '\r' && *(i+1) == '\n') i++;
			else *o++ = *i++;
		}
		*o = 0;
		fltk3::e_length = (int)(o - fltk3::e_text);
		receiver.handle(fltk3::PASTE);
		delete [] fltk3::e_text;
		fltk3::e_text = 0;
	} else {
		if (!OpenClipboard(NULL)) return;
		HANDLE h = GetClipboardData(CF_UNICODETEXT);
		if (h) {
			wchar_t *memLock = (wchar_t*) GlobalLock(h);
			size_t utf16_len = wcslen(memLock);
			fltk3::e_text = (char*) malloc (utf16_len * 4 + 1);
			unsigned utf8_len = fltk3::utf8fromwc(fltk3::e_text, utf16_len * 4, memLock, utf16_len);
			*(fltk3::e_text + utf8_len) = 0;
			LPSTR a,b;
			a = b = fltk3::e_text;
			while (*a) { // strip the CRLF pairs ($%$#@^)
				if (*a == '\r' && a[1] == '\n') a++;
				else *b++ = *a++;
			}
			*b = 0;
			fltk3::e_length = (int)(b - fltk3::e_text);
			receiver.handle(fltk3::PASTE);
			GlobalUnlock(h);
			free(fltk3::e_text);
			fltk3::e_text = 0;
		}
		CloseClipboard();
	}
}

////////////////////////////////////////////////////////////////
char fl_is_ime = 0;
void fl_get_codepage()
{
	HKL hkl = GetKeyboardLayout(0);
	TCHAR ld[8];

	GetLocaleInfo (LOWORD(hkl), LOCALE_IDEFAULTANSICODEPAGE, ld, 6);
	DWORD ccp = atol(ld);
	fl_is_ime = 0;

	fl_codepage = ccp;
	if (fl_aimm) {
		fl_aimm->GetCodePageA(GetKeyboardLayout(0), &fl_codepage);
	} else if (get_imm_module() && flImmIsIME(hkl)) {
		fl_is_ime = 1;
	}
}

HWND fl_capture;

static int mouse_event(fltk3::Window *window, int what, int button,
                       WPARAM wParam, LPARAM lParam)
{
	static int px, py, pmx, pmy;
	POINT pt;
	fltk3::e_x = pt.x = (signed short)LOWORD(lParam);
	fltk3::e_y = pt.y = (signed short)HIWORD(lParam);
	ClientToScreen(fl_xid(window), &pt);
	fltk3::e_x_root = pt.x;
	fltk3::e_y_root = pt.y;
#ifdef USE_CAPTURE_MOUSE_WIN
	fltk3::Window *mouse_window = window;	// save "mouse window"
#endif
	while (window->parent()) {
		fltk3::e_x += window->x();
		fltk3::e_y += window->y();
		window = window->window();
	}

	ulong state = fltk3::e_state & 0xff0000; // keep shift key states
#if 0
	// mouse event reports some shift flags, perhaps save them?
	if (wParam & MK_SHIFT) state |= fltk3::SHIFT;
	if (wParam & MK_CONTROL) state |= fltk3::CTRL;
#endif
	if (wParam & MK_LBUTTON) state |= fltk3::BUTTON1;
	if (wParam & MK_MBUTTON) state |= fltk3::BUTTON2;
	if (wParam & MK_RBUTTON) state |= fltk3::BUTTON3;
	fltk3::e_state = state;

	switch (what) {
	case 1: // double-click
		if (fltk3::e_is_click) {
			fltk3::e_clicks++;
			goto J1;
		}
	case 0: // single-click
		fltk3::e_clicks = 0;
J1:
#ifdef USE_CAPTURE_MOUSE_WIN
		if (!fl_capture) SetCapture(fl_xid(mouse_window));  // use mouse window
#else
		if (!fl_capture) SetCapture(fl_xid(window));	// use main window
#endif
		fltk3::e_keysym = fltk3::MouseButton + button;
		fltk3::e_is_click = 1;
		px = pmx = fltk3::e_x_root;
		py = pmy = fltk3::e_y_root;
		return fltk3::handle(fltk3::PUSH,window);

	case 2: // release:
		if (!fl_capture) ReleaseCapture();
		fltk3::e_keysym = fltk3::MouseButton + button;
		return fltk3::handle(fltk3::RELEASE,window);

	case 3: // move:
	default: // avoid compiler warning
		// MSWindows produces extra events even if mouse does not move, ignore em:
		if (fltk3::e_x_root == pmx && fltk3::e_y_root == pmy) return 1;
		pmx = fltk3::e_x_root;
		pmy = fltk3::e_y_root;
		if (abs(fltk3::e_x_root-px)>5 || abs(fltk3::e_y_root-py)>5) fltk3::e_is_click = 0;
		return fltk3::handle(fltk3::MOVE,window);

	}
}

// convert a MSWindows VK_x to an Fltk (X) Keysym:
// See also the inverse converter in Fl_get_key_win32.cxx
// This table is in numeric order by VK:
static const struct {
	unsigned short vk, fltk, extended;
} vktab[] = {
	{VK_BACK,	fltk3::BackSpaceKey},
	{VK_TAB,	fltk3::TabKey},
	{VK_CLEAR,	fltk3::KPKey+'5',	0xff0b/*XK_Clear*/},
	{VK_RETURN,	fltk3::EnterKey,	fltk3::KPEnterKey},
	{VK_SHIFT,	fltk3::ShiftLKey,	fltk3::ShiftRKey},
	{VK_CONTROL,	fltk3::ControlLKey,	fltk3::ControlRKey},
	{VK_MENU,	fltk3::AltLKey,	fltk3::AltRKey},
	{VK_PAUSE,	fltk3::PauseKey},
	{VK_CAPITAL,	fltk3::CapsLockKey},
	{VK_ESCAPE,	fltk3::EscapeKey},
	{VK_SPACE,	' '},
	{VK_PRIOR,	fltk3::KPKey+'9',	fltk3::PageUpKey},
	{VK_NEXT,	fltk3::KPKey+'3',	fltk3::PageDownKey},
	{VK_END,	fltk3::KPKey+'1',	fltk3::EndKey},
	{VK_HOME,	fltk3::KPKey+'7',	fltk3::HomeKey},
	{VK_LEFT,	fltk3::KPKey+'4',	fltk3::LeftKey},
	{VK_UP,	fltk3::KPKey+'8',	fltk3::UpKey},
	{VK_RIGHT,	fltk3::KPKey+'6',	fltk3::RightKey},
	{VK_DOWN,	fltk3::KPKey+'2',	fltk3::DownKey},
	{VK_SNAPSHOT,	fltk3::PrintKey},	// does not work on NT
	{VK_INSERT,	fltk3::KPKey+'0',	fltk3::InsertKey},
	{VK_DELETE,	fltk3::KPKey+'.',	fltk3::DeleteKey},
	{VK_LWIN,	fltk3::MetaLKey},
	{VK_RWIN,	fltk3::MetaRKey},
	{VK_APPS,	fltk3::MenuKey},
# if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
	{VK_SLEEP,    fltk3::SleepKey},
#endif
	{VK_MULTIPLY,	fltk3::KPKey+'*'},
	{VK_ADD,	fltk3::KPKey+'+'},
	{VK_SUBTRACT,	fltk3::KPKey+'-'},
	{VK_DECIMAL,	fltk3::KPKey+'.'},
	{VK_DIVIDE,	fltk3::KPKey+'/'},
	{VK_NUMLOCK,	fltk3::NumLockKey},
	{VK_SCROLL,	fltk3::ScrollLockKey},
# if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
	{VK_BROWSER_BACK, fltk3::BackKey},
	{VK_BROWSER_FORWARD, fltk3::ForwardKey},
	{VK_BROWSER_REFRESH, fltk3::RefreshKey},
	{VK_BROWSER_STOP, fltk3::StopKey},
	{VK_BROWSER_SEARCH, fltk3::SearchKey},
	{VK_BROWSER_FAVORITES, fltk3::FavoritesKey},
	{VK_BROWSER_HOME, fltk3::HomePageKey},
	{VK_VOLUME_MUTE, fltk3::VolumeMuteKey},
	{VK_VOLUME_DOWN, fltk3::VolumeDownKey},
	{VK_VOLUME_UP, fltk3::VolumeUpKey},
	{VK_MEDIA_NEXT_TRACK, fltk3::MediaNextKey},
	{VK_MEDIA_PREV_TRACK, fltk3::MediaPrevKey},
	{VK_MEDIA_STOP, fltk3::MediaStopKey},
	{VK_MEDIA_PLAY_PAUSE, fltk3::MediaPlayKey},
	{VK_LAUNCH_MAIL, fltk3::MailKey},
#endif
	{0xba,	';'},
	{0xbb,	'='},
	{0xbc,	','},
	{0xbd,	'-'},
	{0xbe,	'.'},
	{0xbf,	'/'},
	{0xc0,	'`'},
	{0xdb,	'['},
	{0xdc,	'\\'},
	{0xdd,	']'},
	{0xde,	'\''},
	{226 /*VK_OEM_102*/, fltk3::IsoKey}
};
static int ms2fltk(WPARAM vk, int extended)
{
	static unsigned short vklut[256];
	static unsigned short extendedlut[256];
	if (!vklut[1]) { // init the table
		unsigned int i;
		for (i = 0; i < 256; i++) vklut[i] = tolower(i);
		for (i=VK_F1; i<=VK_F16; i++) vklut[i] = i+(fltk3::FKey-(VK_F1-1));
		for (i=VK_NUMPAD0; i<=VK_NUMPAD9; i++) vklut[i] = i+(fltk3::KPKey+'0'-VK_NUMPAD0);
		for (i = 0; i < sizeof(vktab)/sizeof(*vktab); i++) {
			vklut[vktab[i].vk] = vktab[i].fltk;
			extendedlut[vktab[i].vk] = vktab[i].extended;
		}
		for (i = 0; i < 256; i++) if (!extendedlut[i]) extendedlut[i] = vklut[i];
	}
	return extended ? extendedlut[vk] : vklut[vk];
}

#if USE_COLORMAP
extern HPALETTE fl_select_palette(void); // in fl_color_win32.cxx
#endif


/////////////////////////////////////////////////////////////////////////////
/// Win32 timers
///

struct Win32Timer {
	UINT_PTR handle;
	fltk3::TimeoutHandler callback;
	void *data;
};
static Win32Timer* win32_timers;
static int win32_timer_alloc;
static int win32_timer_used;
static HWND s_TimerWnd;

static void realloc_timers()
{
	if (win32_timer_alloc == 0) {
		win32_timer_alloc = 8;
	}
	win32_timer_alloc *= 2;
	Win32Timer* new_timers = new Win32Timer[win32_timer_alloc];
	memset(new_timers, 0, sizeof(Win32Timer) * win32_timer_used);
	memcpy(new_timers, win32_timers, sizeof(Win32Timer) * win32_timer_used);
	Win32Timer* delete_me = win32_timers;
	win32_timers = new_timers;
	delete [] delete_me;
}

static void delete_timer(Win32Timer& t)
{
	KillTimer(s_TimerWnd, t.handle);
	memset(&t, 0, sizeof(Win32Timer));
}

/// END TIMERS
/////////////////////////////////////////////////////////////////////////////

static fltk3::Window* resize_bug_fix;

extern void fl_save_pen(void);
extern void fl_restore_pen(void);

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Copy the message to fl_msg so add_handler code can see it, it is
	// already there if this is called by DispatchMessage, but not if
	// Windows calls this directly.
	fl_msg.hwnd = hWnd;
	fl_msg.message = uMsg;
	fl_msg.wParam = wParam;
	fl_msg.lParam = lParam;
	//fl_msg.time = ???
	//fl_msg.pt = ???
	//fl_msg.lPrivate = ???

	fltk3::Window *window = fl_find(hWnd);

	if (window) switch (uMsg) {

		case WM_QUIT: // this should not happen?
			fltk3::fatal("WM_QUIT message");

		case WM_CLOSE: // user clicked close box
			fltk3::handle(fltk3::CLOSE, window);
			PostQuitMessage(0);
			return 0;

		case WM_SYNCPAINT :
		case WM_NCPAINT :
		case WM_ERASEBKGND :
			// Andreas Weitl - WM_SYNCPAINT needs to be passed to DefWindowProc
			// so that Windows can generate the proper paint messages...
			// Similarly, WM_NCPAINT and WM_ERASEBKGND need this, too...
			break;

		case WM_PAINT: {
			fltk3::Region R;
			Fl_X *i = Fl_X::i(window);
			i->wait_for_expose = 0;
			char redraw_whole_window = false;
			if (!i->region && window->damage()) {
				// Redraw the whole window...
				i->region = CreateRectRgn(0, 0, window->w(), window->h());
				redraw_whole_window = true;
			}

			// We need to merge WIN32's damage into FLTK's damage.
			R = CreateRectRgn(0,0,0,0);
			int r = GetUpdateRgn(hWnd,R,0);
			if (r==NULLREGION && !redraw_whole_window) {
				break;
			}

			if (i->region) {
				// Also tell WIN32 that we are drawing someplace else as well...
				CombineRgn(i->region, i->region, R, RGN_OR);
				XDestroyRegion(R);
			} else {
				i->region = R;
			}
			if (window->type() == fltk3::DOUBLE_WINDOW) ValidateRgn(hWnd,0);
			else ValidateRgn(hWnd,i->region);

			window->set_damage((uchar)(window->damage()|fltk3::DAMAGE_EXPOSE));
			// These next two statements should not be here, so that all update
			// is deferred until fltk3::flush() is called during idle.  However WIN32
			// apparently is very unhappy if we don't obey it and draw right now.
			// Very annoying!
			fl_GetDC(hWnd); // Make sure we have a DC for this window...
			fl_save_pen();
			i->flush();
			fl_restore_pen();
			window->clear_damage();
		}
		return 0;

		case WM_LBUTTONDOWN:
			mouse_event(window, 0, 1, wParam, lParam);
			return 0;
		case WM_LBUTTONDBLCLK:
			mouse_event(window, 1, 1, wParam, lParam);
			return 0;
		case WM_LBUTTONUP:
			mouse_event(window, 2, 1, wParam, lParam);
			return 0;
		case WM_MBUTTONDOWN:
			mouse_event(window, 0, 2, wParam, lParam);
			return 0;
		case WM_MBUTTONDBLCLK:
			mouse_event(window, 1, 2, wParam, lParam);
			return 0;
		case WM_MBUTTONUP:
			mouse_event(window, 2, 2, wParam, lParam);
			return 0;
		case WM_RBUTTONDOWN:
			mouse_event(window, 0, 3, wParam, lParam);
			return 0;
		case WM_RBUTTONDBLCLK:
			mouse_event(window, 1, 3, wParam, lParam);
			return 0;
		case WM_RBUTTONUP:
			mouse_event(window, 2, 3, wParam, lParam);
			return 0;

		case WM_MOUSEMOVE:
#ifdef USE_TRACK_MOUSE
			if (track_mouse_win != window) {
				TRACKMOUSEEVENT tme;
				tme.cbSize    = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags   = TME_LEAVE;
				tme.hwndTrack = hWnd;
				_TrackMouseEvent(&tme);
				track_mouse_win = window;
			}
#endif // USE_TRACK_MOUSE
			mouse_event(window, 3, 0, wParam, lParam);
			return 0;

		case WM_MOUSELEAVE:
			if (track_mouse_win == window) { // we left the top level window !
				fltk3::Window *tw = window;
				while (tw->parent()) tw = tw->window(); // find top level window
				fltk3::belowmouse(0);
				fltk3::handle(fltk3::LEAVE, tw);
			}
			track_mouse_win = 0; // force TrackMouseEvent() restart
			break;

		case WM_SETFOCUS:
			fltk3::handle(fltk3::FOCUS, window);
			break;

		case WM_KILLFOCUS:
			fltk3::handle(fltk3::UNFOCUS, window);
			fltk3::flush(); // it never returns to main loop when deactivated...
			break;

		case WM_SHOWWINDOW:
			if (!window->parent()) {
				fltk3::handle(wParam ? fltk3::SHOW : fltk3::HIDE, window);
			}
			break;

		case WM_ACTIVATEAPP:
			// From eric@vfx.sel.sony.com, we should process WM_ACTIVATEAPP
			// messages to restore the correct state of the shift/ctrl/alt/lock
			// keys...  Added control, shift, alt, and meta keys, and changed
			// to use GetAsyncKeyState and do it when wParam is 1
			// (that means we have focus...)
			if (wParam) {
				ulong state = 0;
				if (GetAsyncKeyState(VK_CAPITAL)) state |= fltk3::CAPS_LOCK;
				if (GetAsyncKeyState(VK_NUMLOCK)) state |= fltk3::NUM_LOCK;
				if (GetAsyncKeyState(VK_SCROLL)) state |= fltk3::SCROLL_LOCK;
				if (GetAsyncKeyState(VK_CONTROL)&~1) state |= fltk3::CTRL;
				if (GetAsyncKeyState(VK_SHIFT)&~1) state |= fltk3::SHIFT;
				if (GetAsyncKeyState(VK_MENU)) state |= fltk3::ALT;
				if ((GetAsyncKeyState(VK_LWIN)|GetAsyncKeyState(VK_RWIN))&~1) state |= fltk3::META;
				fltk3::e_state = state;
				return 0;
			}
			break;

		case WM_INPUTLANGCHANGE:
			fl_get_codepage();
			break;
		case WM_IME_COMPOSITION:
//	if (!fl_is_nt4() && lParam & GCS_RESULTCLAUSE) {
//		HIMC himc = ImmGetContext(hWnd);
//		wlen = ImmGetCompositionStringW(himc, GCS_RESULTSTR,
//			wbuf, sizeof(wbuf)) / sizeof(short);
//		if (wlen < 0) wlen = 0;
//		wbuf[wlen] = 0;
//		ImmReleaseContext(hWnd, himc);
//	}
			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
			// save the keysym until we figure out the characters:
			fltk3::e_keysym = fltk3::e_original_keysym = ms2fltk(wParam,lParam&(1<<24));
			// See if TranslateMessage turned it into a WM_*CHAR message:
			if (PeekMessageW(&fl_msg, hWnd, WM_CHAR, WM_SYSDEADCHAR, PM_REMOVE)) {
				uMsg = fl_msg.message;
				wParam = fl_msg.wParam;
				lParam = fl_msg.lParam;
			}
		case WM_DEADCHAR:
		case WM_SYSDEADCHAR:
		case WM_CHAR:
		case WM_SYSCHAR: {
			ulong state = fltk3::e_state & 0xff000000; // keep the mouse button state
			// if GetKeyState is expensive we might want to comment some of these out:
			if (GetKeyState(VK_SHIFT)&~1) state |= fltk3::SHIFT;
			if (GetKeyState(VK_CAPITAL)) state |= fltk3::CAPS_LOCK;
			if (GetKeyState(VK_CONTROL)&~1) state |= fltk3::CTRL;
			// Alt gets reported for the Alt-GR switch on foreign keyboards.
			// so we need to check the event as well to get it right:
			if ((lParam&(1<<29)) //same as GetKeyState(VK_MENU)
			    && uMsg != WM_CHAR) state |= fltk3::ALT;
			if (GetKeyState(VK_NUMLOCK)) state |= fltk3::NUM_LOCK;
			if ((GetKeyState(VK_LWIN)|GetKeyState(VK_RWIN))&~1) {
				// WIN32 bug?  GetKeyState returns garbage if the user hit the
				// meta key to pop up start menu.  Sigh.
				if ((GetAsyncKeyState(VK_LWIN)|GetAsyncKeyState(VK_RWIN))&~1)
					state |= fltk3::META;
			}
			if (GetKeyState(VK_SCROLL)) state |= fltk3::SCROLL_LOCK;
			fltk3::e_state = state;
			static char buffer[1024];
			if (uMsg == WM_CHAR || uMsg == WM_SYSCHAR) {

				xchar u = (xchar) wParam;
//    fltk3::e_length = fl_unicode2utf(&u, 1, buffer);
				fltk3::e_length = fltk3::utf8fromwc(buffer, 1024, &u, 1);
				buffer[fltk3::e_length] = 0;


			} else if (fltk3::e_keysym >= fltk3::KPKey && fltk3::e_keysym <= fltk3::KPLastKey) {
				if (state & fltk3::NUM_LOCK) {
					// Convert to regular keypress...
					buffer[0] = fltk3::e_keysym-fltk3::KPKey;
					fltk3::e_length = 1;
				} else {
					// Convert to special keypress...
					buffer[0] = 0;
					fltk3::e_length = 0;
					switch (fltk3::e_keysym) {
					case fltk3::KPKey + '0' :
						fltk3::e_keysym = fltk3::InsertKey;
						break;
					case fltk3::KPKey + '1' :
						fltk3::e_keysym = fltk3::EndKey;
						break;
					case fltk3::KPKey + '2' :
						fltk3::e_keysym = fltk3::DownKey;
						break;
					case fltk3::KPKey + '3' :
						fltk3::e_keysym = fltk3::PageDownKey;
						break;
					case fltk3::KPKey + '4' :
						fltk3::e_keysym = fltk3::LeftKey;
						break;
					case fltk3::KPKey + '6' :
						fltk3::e_keysym = fltk3::RightKey;
						break;
					case fltk3::KPKey + '7' :
						fltk3::e_keysym = fltk3::HomeKey;
						break;
					case fltk3::KPKey + '8' :
						fltk3::e_keysym = fltk3::UpKey;
						break;
					case fltk3::KPKey + '9' :
						fltk3::e_keysym = fltk3::PageUpKey;
						break;
					case fltk3::KPKey + '.' :
						fltk3::e_keysym = fltk3::DeleteKey;
						break;
					case fltk3::KPKey + '/' :
					case fltk3::KPKey + '*' :
					case fltk3::KPKey + '-' :
					case fltk3::KPKey + '+' :
						buffer[0] = fltk3::e_keysym-fltk3::KPKey;
						fltk3::e_length = 1;
						break;
					}
				}
			} else if ((lParam & (1<<31))==0) {
#ifdef FLTK_PREVIEW_DEAD_KEYS
				if ((lParam & (1<<24))==0) { // clear if dead key (always?)
					xchar u = (xchar) wParam;
					fltk3::e_length = fltk3::utf8fromwc(buffer, 1024, &u, 1);
					buffer[fltk3::e_length] = 0;
				} else { // set if "extended key" (never printable?)
					buffer[0] = 0;
					fltk3::e_length = 0;
				}
#else
				buffer[0] = 0;
				fltk3::e_length = 0;
#endif
			}
			fltk3::e_text = buffer;
			if (lParam & (1<<31)) { // key up events.
				if (fltk3::handle(fltk3::KEYUP, window)) return 0;
				break;
			}
			// for (int i = lParam&0xff; i--;)
			while (window->parent()) window = window->window();
			if (fltk3::handle(fltk3::KEYBOARD,window)) {
				if (uMsg==WM_DEADCHAR || uMsg==WM_SYSDEADCHAR)
					fltk3::compose_state = 1;
				return 0;
			}
			break;
		}

		case WM_MOUSEWHEEL: {
			static int delta = 0; // running total of all motion
			delta += (SHORT)(HIWORD(wParam));
			fltk3::e_dx = 0;
			fltk3::e_dy = -delta / WHEEL_DELTA;
			delta += fltk3::e_dy * WHEEL_DELTA;
			if (fltk3::e_dy) fltk3::handle(fltk3::MOUSEWHEEL, window);
			return 0;
		}

		case WM_MOUSEHWHEEL: {
			static int delta = 0; // running total of all motion
			delta += (SHORT)(HIWORD(wParam));
			fltk3::e_dy = 0;
			fltk3::e_dx = delta / WHEEL_DELTA;
			delta -= fltk3::e_dx * WHEEL_DELTA;
			if (fltk3::e_dx) fltk3::handle(fltk3::MOUSEWHEEL, window);
			return 0;
		}

		case WM_GETMINMAXINFO:
			Fl_X::i(window)->set_minmax((LPMINMAXINFO)lParam);
			break;

		case WM_SIZE:
			if (!window->parent()) {
				if (wParam == SIZE_MINIMIZED || wParam == SIZE_MAXHIDE) {
					fltk3::handle(fltk3::HIDE, window);
				} else {
					fltk3::handle(fltk3::SHOW, window);
					resize_bug_fix = window;
					window->size(LOWORD(lParam), HIWORD(lParam));
				}
			}
			break;

		case WM_MOVE: {
			resize_bug_fix = window;
			int nx = LOWORD(lParam);
			int ny = HIWORD(lParam);
			if (nx & 0x8000) nx -= 65536;
			if (ny & 0x8000) ny -= 65536;
			window->position(nx, ny);
		}
		break;

		case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT) {
				while (window->parent()) window = window->window();
				SetCursor(Fl_X::i(window)->cursor);
				return 0;
			}
			break;

#if USE_COLORMAP
		case WM_QUERYNEWPALETTE :
			fl_GetDC(hWnd);
			if (fl_select_palette()) InvalidateRect(hWnd, NULL, FALSE);
			break;

		case WM_PALETTECHANGED:
			fl_GetDC(hWnd);
			if ((HWND)wParam != hWnd && fl_select_palette()) UpdateColors(fl_gc);
			break;

		case WM_CREATE :
			fl_GetDC(hWnd);
			fl_select_palette();
			break;
#endif

		case WM_DESTROYCLIPBOARD:
			fl_i_own_selection[1] = 0;
			return 1;

		case WM_RENDERALLFORMATS:
			fl_i_own_selection[1] = 0;
			// Windoze seems unhappy unless I do these two steps. Documentation
			// seems to vary on whether opening the clipboard is necessary or
			// is in fact wrong:
			CloseClipboard();
			OpenClipboard(NULL);
			// fall through...
		case WM_RENDERFORMAT: {
			HANDLE h;

//  int l = fltk3::utf_nb_char((unsigned char*)fl_selection_buffer[1], fl_selection_length[1]);
			int l = fltk3::utf8toUtf16(fl_selection_buffer[1], fl_selection_length[1], NULL, 0); // Pass NULL buffer to query length required
			h = GlobalAlloc(GHND, (l+1) * sizeof(unsigned short));
			if (h) {
				unsigned short *g = (unsigned short*) GlobalLock(h);
//    fl_utf2unicode((unsigned char *)fl_selection_buffer[1], fl_selection_length[1], (xchar*)g);
				l = fltk3::utf8toUtf16(fl_selection_buffer[1], fl_selection_length[1], g, (l+1));
				g[l] = 0;
				GlobalUnlock(h);
				SetClipboardData(CF_UNICODETEXT, h);
			}

			// Windoze also seems unhappy if I don't do this. Documentation very
			// unclear on what is correct:
			if (fl_msg.message == WM_RENDERALLFORMATS) CloseClipboard();
			return 1;
		}
		case WM_DISPLAYCHANGE: // occurs when screen configuration (number, position) changes
			fltk3::call_screen_init();
			fltk3::handle(fltk3::SCREEN_CONFIGURATION_CHANGED, NULL);
			return 0;

		default:
			if (fltk3::handle(0,0)) return 0;
			break;
		}


	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

////////////////////////////////////////////////////////////////
// This function gets the dimensions of the top/left borders and
// the title bar, if there is one, based on the FL_BORDER, FL_MODAL
// and FL_NONMODAL flags, and on the window's size range.
// It returns the following values:
//
// value | border | title bar
//   0   |  none  |   no
//   1   |  fix   |   yes
//   2   |  size  |   yes

int Fl_X::fake_X_wm(const fltk3::Window* w,int &X,int &Y, int &bt,int &bx, int &by)
{
	int W, H, xoff, yoff, dx, dy;
	int ret = bx = by = bt = 0;

	int fallback = 1;
	if (!w->parent()) {
		HWND hwnd = fl_xid(w);
		if (hwnd) {
			// The block below calculates the window borders by requesting the
			// required decorated window rectangle for a desired client rectangle.
			// If any part of the function above fails, we will drop to a
			// fallback to get the best guess which is always available.
			HWND hwnd = fl_xid(w);
			// request the style flags of this window, as WIN32 sees them
			LONG style = GetWindowLong(hwnd, GWL_STYLE);
			LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
			RECT r;
			r.left = w->x();
			r.top = w->y();
			r.right = w->x()+w->w();
			r.bottom = w->y()+w->h();
			// get the decoration rectangle for the desired client rectangle
			BOOL ok = AdjustWindowRectEx(&r, style, FALSE, exstyle);
			if (ok) {
				X = r.left;
				Y = r.top;
				W = r.right - r.left;
				H = r.bottom - r.top;
				bx = w->x() - r.left;
				by = r.bottom - w->y() - w->h(); // height of the bootm frame
				bt = w->y() - r.top - by; // height of top caption bar
				xoff = bx;
				yoff = by + bt;
				dx = W - w->w();
				dy = H - w->h();
				if (w->size_range_set && (w->maxw != w->minw || w->maxh != w->minh))
					ret = 2;
				else
					ret = 1;
				fallback = 0;
			}
		}
	}
	// This is the original (pre 1.1.7) routine to calculate window border sizes.
	if (fallback) {
		if (w->border() && !w->parent()) {
			if (w->size_range_set && (w->maxw != w->minw || w->maxh != w->minh)) {
				ret = 2;
				bx = GetSystemMetrics(SM_CXSIZEFRAME);
				by = GetSystemMetrics(SM_CYSIZEFRAME);
			} else {
				ret = 1;
				bx = GetSystemMetrics(SM_CXFIXEDFRAME);
				by = GetSystemMetrics(SM_CYFIXEDFRAME);
			}
			bt = GetSystemMetrics(SM_CYCAPTION);
		}
		//The coordinates of the whole window, including non-client area
		xoff = bx;
		yoff = by + bt;
		dx = 2*bx;
		dy = 2*by + bt;
		X = w->x()-xoff;
		Y = w->y()-yoff;
		W = w->w()+dx;
		H = w->h()+dy;
	}

	//Proceed to positioning the window fully inside the screen, if possible
	//Find screen that contains most of the window
	//FIXME: this ought to be the "work area" instead of the entire screen !
	int scr_x, scr_y, scr_w, scr_h;
	fltk3::screen_xywh(scr_x, scr_y, scr_w, scr_h, X, Y, W, H);
	//Make border's lower right corner visible
	if (scr_x+scr_w < X+W) X = scr_x+scr_w - W;
	if (scr_y+scr_h < Y+H) Y = scr_y+scr_h - H;
	//Make border's upper left corner visible
	if (X<scr_x) X = scr_x;
	if (Y<scr_y) Y = scr_y;
	//Make client area's lower right corner visible
	if (scr_x+scr_w < X+dx+ w->w()) X = scr_x+scr_w - w->w() - dx;
	if (scr_y+scr_h < Y+dy+ w->h()) Y = scr_y+scr_h - w->h() - dy;
	//Make client area's upper left corner visible
	if (X+xoff < scr_x) X = scr_x-xoff;
	if (Y+yoff < scr_y) Y = scr_y-yoff;
	//Return the client area's top left corner in (X,Y)
	X+=xoff;
	Y+=yoff;

	if (w->fullscreen_active()) {
		X = Y = 0;
		bx = by = bt = 0;
	}

	return ret;
}

////////////////////////////////////////////////////////////////

void fltk3::Window::resize(int X,int Y,int W,int H)
{
	UINT flags = SWP_NOSENDCHANGING | SWP_NOZORDER
	             | SWP_NOACTIVATE | SWP_NOOWNERZORDER;
	int is_a_resize = (W != w() || H != h());
	int resize_from_program = (this != resize_bug_fix);
	if (!resize_from_program) resize_bug_fix = 0;
	if (X != x() || Y != y()) {
		force_position(1);
	} else {
		if (!is_a_resize) return;
		flags |= SWP_NOMOVE;
	}
	if (is_a_resize) {
		Group::resize(X,Y,W,H);
		if (visible_r()) {
			redraw();
			// only wait for exposure if this window has a size - a window
			// with no width or height will never get an exposure event
			if (i && W>0 && H>0)
				i->wait_for_expose = 1;
		}
	} else {
		x(X);
		y(Y);
		flags |= SWP_NOSIZE;
	}
	if (!border()) flags |= SWP_NOACTIVATE;
	if (resize_from_program && shown()) {
		if (!resizable()) size_range(w(),h(),w(),h());
		int dummy_x, dummy_y, bt, bx, by;
		//Ignore window managing when resizing, so that windows (and more
		//specifically menus) can be moved offscreen.
		if (Fl_X::fake_X_wm(this, dummy_x, dummy_y, bt, bx, by)) {
			X -= bx;
			Y -= by+bt;
			W += 2*bx;
			H += 2*by+bt;
		}
		// avoid zero size windows. A zero sized window on Win32
		// will cause continouly  new redraw events.
		if (W<=0) W = 1;
		if (H<=0) H = 1;
		SetWindowPos(i->xid, 0, X, Y, W, H, flags);
	}
}

static void make_fullscreen(fltk3::Window *w, Window xid, int X, int Y, int W, int H)
{
	int sx, sy, sw, sh;
	fltk3::screen_xywh(sx, sy, sw, sh, X, Y, W, H);
	DWORD flags = GetWindowLong(xid, GWL_STYLE);
	flags = flags & ~(WS_THICKFRAME|WS_CAPTION);
	SetWindowLong(xid, GWL_STYLE, flags);
	// SWP_NOSENDCHANGING is so that we can override size limits
	SetWindowPos(xid, HWND_TOP, sx, sy, sw, sh, SWP_NOSENDCHANGING | SWP_FRAMECHANGED);
}

void fltk3::Window::fullscreen_x()
{
	_set_fullscreen();
	make_fullscreen(this, fl_xid(this), x(), y(), w(), h());
	fltk3::handle(fltk3::FULLSCREEN, this);
}

void fltk3::Window::fullscreen_off_x(int X, int Y, int W, int H)
{
	_clear_fullscreen();
	DWORD style = GetWindowLong(fl_xid(this), GWL_STYLE);
	// Remove the xid temporarily so that Fl_X::fake_X_wm() behaves like it
	// does in Fl_X::make().
	HWND xid = fl_xid(this);
	Fl_X::i(this)->xid = NULL;
	int wx, wy, bt, bx, by;
	switch (Fl_X::fake_X_wm(this, wx, wy, bt, bx, by)) {
	case 0:
		break;
	case 1:
		style |= WS_CAPTION;
		break;
	case 2:
		if (border()) {
			style |= WS_THICKFRAME | WS_CAPTION;
		}
		break;
	}
	Fl_X::i(this)->xid = xid;
	// Adjust for decorations (but not if that puts the decorations
	// outside the screen)
	if ((X != x()) || (Y != y())) {
		X -= bx;
		Y -= by+bt;
	}
	W += bx*2;
	H += by*2+bt;
	SetWindowLong(fl_xid(this), GWL_STYLE, style);
	SetWindowPos(fl_xid(this), 0, X, Y, W, H,
	             SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
	fltk3::handle(fltk3::FULLSCREEN, this);
}

////////////////////////////////////////////////////////////////

/*
 * This silly little class remembers the name of all window classes
 * we register to avoid double registration. It has the added bonus
 * of freeing everything on application close as well.
 */
class NameList
{
public:
	NameList() {
		name = (char**)malloc(sizeof(char**));
		NName = 1;
		nName = 0;
	}
	~NameList() {
		int i;
		for (i=0; i<nName; i++) free(name[i]);
		if (name) free(name);
	}
	void add_name(const char *n) {
		if (NName==nName) {
			NName += 5;
			name = (char**)realloc(name, NName * sizeof(char*));
		}
		name[nName++] = strdup(n);
	}
	char has_name(const char *n) {
		int i;
		for (i=0; i<nName; i++) {
			if (strcmp(name[i], n)==0) return 1;
		}
		return 0;
	}
private:
	char **name;
	int nName, NName;
};

void fl_fix_focus(); // in Fl.cxx

char fl_show_iconic;	// hack for fltk3::Window::iconic()
// int fl_background_pixel = -1; // color to use for background
HCURSOR fl_default_cursor;
UINT fl_wake_msg = 0;
int fl_disable_transient_for; // secret method of removing TRANSIENT_FOR

Fl_X* Fl_X::make(fltk3::Window* w)
{
	fltk3::Group::current(0); // get rid of very common user bug: forgot end()

	// if the window is a subwindow and our parent is not mapped yet, we
	// mark this window visible, so that mapping the parent at a later
	// point in time will call this function again to finally map the subwindow.
	if (w->parent() && !Fl_X::i(w->window())) {
		w->set_visible();
		return 0L;
	}

	static NameList class_name_list;
	static const char *first_class_name = 0L;
	const char *class_name = w->xclass();
	if (!class_name) class_name = first_class_name; // reuse first class name used
	if (!class_name) class_name = "FLTK"; // default to create a "FLTK" WNDCLASS
	if (!first_class_name) {
		first_class_name = class_name;
	}

	wchar_t class_namew[100]; // (limited) buffer for Windows class name

	// convert UTF-8 class_name to wchar_t for RegisterClassExW and CreateWindowExW

	fltk3::utf8toUtf16(class_name,
	                   (unsigned)strlen(class_name),		// in
	                   (unsigned short*)class_namew,		// out
	                   (unsigned)(sizeof(class_namew)/sizeof(wchar_t)));	// max. size

	if (!class_name_list.has_name(class_name)) {
		WNDCLASSEXW wcw;
		memset(&wcw, 0, sizeof(wcw));
		wcw.cbSize = sizeof(WNDCLASSEXW);

		// Documentation states a device context consumes about 800 bytes
		// of memory... so who cares? If 800 bytes per window is what it
		// takes to speed things up, I'm game.
		//wc.style = CS_HREDRAW | CS_VREDRAW | CS_CLASSDC | CS_DBLCLKS;
		wcw.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
		wcw.lpfnWndProc = (WNDPROC)WndProc;
		wcw.cbClsExtra = wcw.cbWndExtra = 0;
		wcw.hInstance = fl_display;
		if (!w->icon())
			w->icon((void *)LoadIcon(NULL, IDI_APPLICATION));
		wcw.hIcon = wcw.hIconSm = (HICON)w->icon();
		wcw.hCursor = fl_default_cursor = LoadCursor(NULL, IDC_ARROW);
		//uchar r,g,b; fltk3::get_color(fltk3::GRAY,r,g,b);
		//wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(r,g,b));
		wcw.hbrBackground = NULL;
		wcw.lpszMenuName = NULL;
		wcw.lpszClassName = class_namew;
		RegisterClassExW(&wcw);
		class_name_list.add_name(class_name);
	}

	const wchar_t* message_namew = L"FLTK::ThreadWakeup";
	if (!fl_wake_msg) fl_wake_msg = RegisterWindowMessageW(message_namew);

	HWND parent;
	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	DWORD styleEx = WS_EX_LEFT;

	int xp = w->x();
	int yp = w->y();
	int wp = w->w();
	int hp = w->h();

	int showit = 1;

	if (w->parent()) {
		xp += w->parent()->dx_window();
		yp += w->parent()->dy_window();
		style |= WS_CHILD;
		styleEx |= WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT;
		parent = fl_xid(w->window());
	} else {
		if (!w->size_range_set) {
			if (w->resizable()) {
				fltk3::Widget *o = w->resizable();
				int minw = o->w();
				if (minw > 100) minw = 100;
				int minh = o->h();
				if (minh > 100) minh = 100;
				w->size_range(w->w() - o->w() + minw, w->h() - o->h() + minh, 0, 0);
			} else {
				w->size_range(w->w(), w->h(), w->w(), w->h());
			}
		}
		styleEx |= WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT;
		int xwm = xp , ywm = yp , bt, bx, by;
		switch (fake_X_wm(w, xwm, ywm, bt, bx, by)) {
			// No border (used for menus)
		case 0:
			style |= WS_POPUP;
			styleEx |= WS_EX_TOOLWINDOW;
			break;

			// Thin border and title bar
		case 1:
			style |= WS_DLGFRAME | WS_CAPTION;
			if (!w->modal())
				style |= WS_SYSMENU | WS_MINIMIZEBOX;
			break;

			// Thick, resizable border and title bar, with maximize button
		case 2:
			style |= WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_CAPTION;
			if (!w->modal())
				style |= WS_MINIMIZEBOX;
			break;
		}
		if (by+bt) {
			wp += 2*bx;
			hp += 2*by+bt;
		}
		if (!w->force_position()) {
			xp = yp = CW_USEDEFAULT;
		} else {
			if (!fltk3::grab()) {
				xp = xwm;
				yp = ywm;
				w->x(xp);
				w->y(yp);
			}
			xp -= bx;
			yp -= by+bt;
		}

		parent = 0;
		if (w->non_modal() && Fl_X::first && !fl_disable_transient_for) {
			// find some other window to be "transient for":
			fltk3::Window* w = Fl_X::first->w;
			while (w->parent()) w = w->window();
			parent = fl_xid(w);
			if (!w->visible()) showit = 0;
		} else if (fltk3::grab()) parent = fl_xid(fltk3::grab());
	}

	Fl_X* x = new Fl_X;
	x->other_xid = 0;
	x->setwindow(w);
	x->region = 0;
	x->private_dc = 0;
	x->cursor = fl_default_cursor;
	if (!fl_codepage) fl_get_codepage();

	WCHAR *lab = NULL;
	if (w->label()) {
		int l = (int)strlen(w->label());
//  lab = (WCHAR*) malloc((l + 1) * sizeof(short));
//  l = fl_utf2unicode((unsigned char*)w->label(), l, (xchar*)lab);
//  lab[l] = 0;
		unsigned wlen = fltk3::utf8toUtf16(w->label(), (unsigned)l, NULL, 0); // Pass NULL to query length
		wlen++;
		lab = (WCHAR *) malloc(sizeof(WCHAR)*wlen);
		wlen = fltk3::utf8toUtf16(w->label(), (unsigned)l, (unsigned short*)lab, wlen);
		lab[wlen] = 0;
	}
	x->xid = CreateWindowExW(
	                 styleEx,
	                 class_namew, lab, style,
	                 xp, yp, wp, hp,
	                 parent,
	                 NULL, // menu
	                 fl_display,
	                 NULL // creation parameters
	         );
	if (lab) free(lab);

	if (w->fullscreen_active()) {
		/* We need to make sure that the fullscreen is created on the
		 default monitor, ie the desktop where the shortcut is located
		etc. This requires that CreateWindow is called with CW_USEDEFAULT
		for x and y. We can then use GetWindowRect to determine which
		monitor the window was placed on. */
		RECT rect;
		GetWindowRect(x->xid, &rect);
		make_fullscreen(w, x->xid, rect.left, rect.top,
		                rect.right - rect.left, rect.bottom - rect.top);
	}

	x->next = Fl_X::first;
	Fl_X::first = x;

	x->wait_for_expose = 1;
	if (fl_show_iconic) {
		showit = 0;
		fl_show_iconic = 0;
	}
	if (showit) {
		w->set_visible();
		int old_event = fltk3::e_number;
		w->handle(fltk3::e_number = fltk3::SHOW); // get child windows to appear
		fltk3::e_number = old_event;
		w->redraw(); // force draw to happen
	}
	// If we've captured the mouse, we dont want to activate any
	// other windows from the code, or we lose the capture.
	ShowWindow(x->xid, !showit ? SW_SHOWMINNOACTIVE :
	           (fltk3::grab() || (styleEx & WS_EX_TOOLWINDOW)) ? SW_SHOWNOACTIVATE : SW_SHOWNORMAL);

	// Register all windows for potential drag'n'drop operations
	fl_OleInitialize();
	RegisterDragDrop(x->xid, flIDropTarget);

	if (!fl_aimm) {
		CoCreateInstance(CLSID_CActiveIMM, NULL, CLSCTX_INPROC_SERVER,
		                 IID_IActiveIMMApp, (void**) &fl_aimm);
		if (fl_aimm) {
			fl_aimm->Activate(TRUE);
		}
	}

	if (w->modal()) {
		fltk3::modal_ = w;
		fl_fix_focus();
	}
	return x;
}




/////////////////////////////////////////////////////////////////////////////
/// Win32 timers
///


static LRESULT CALLBACK s_TimerProc(HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_TIMER: {
		unsigned int id = (unsigned)(wParam - 1);
		if (id < (unsigned int)win32_timer_used && win32_timers[id].handle) {
			fltk3::TimeoutHandler cb   = win32_timers[id].callback;
			void*              data = win32_timers[id].data;
			delete_timer(win32_timers[id]);
			if (cb) {
				(*cb)(data);
			}
		}
	}
	return 0;

	default:
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void fltk3::add_timeout(double time, fltk3::TimeoutHandler cb, void* data)
{
	repeat_timeout(time, cb, data);
}

void fltk3::repeat_timeout(double time, fltk3::TimeoutHandler cb, void* data)
{
	int timer_id = -1;
	for (int i = 0;  i < win32_timer_used;  ++i) {
		if ( !win32_timers[i].handle ) {
			timer_id = i;
			break;
		}
	}
	if (timer_id == -1) {
		if (win32_timer_used == win32_timer_alloc) {
			realloc_timers();
		}
		timer_id = win32_timer_used++;
	}
	unsigned int elapsed = (unsigned int)(time * 1000);

	if ( !s_TimerWnd ) {
		const char* timer_class = "FLTimer";
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(wc));
		wc.cbSize = sizeof (wc);
		wc.style = CS_CLASSDC;
		wc.lpfnWndProc = (WNDPROC)s_TimerProc;
		wc.hInstance = fl_display;
		wc.lpszClassName = timer_class;
		/*ATOM atom =*/
		RegisterClassEx(&wc);
		// create a zero size window to handle timer events
		s_TimerWnd = CreateWindowEx(WS_EX_LEFT | WS_EX_TOOLWINDOW,
		                            timer_class, "",
		                            WS_POPUP,
		                            0, 0, 0, 0,
		                            NULL, NULL, fl_display, NULL);
		// just in case this OS won't let us create a 0x0 size window:
		if (!s_TimerWnd)
			s_TimerWnd = CreateWindowEx(WS_EX_LEFT | WS_EX_TOOLWINDOW,
			                            timer_class, "",
			                            WS_POPUP,
			                            0, 0, 1, 1,
			                            NULL, NULL, fl_display, NULL);
		ShowWindow(s_TimerWnd, SW_SHOWNOACTIVATE);
	}

	win32_timers[timer_id].callback = cb;
	win32_timers[timer_id].data     = data;

	win32_timers[timer_id].handle =
	        SetTimer(s_TimerWnd, timer_id + 1, elapsed, NULL);
}

int fltk3::has_timeout(fltk3::TimeoutHandler cb, void* data)
{
	for (int i = 0;  i < win32_timer_used;  ++i) {
		Win32Timer& t = win32_timers[i];
		if (t.handle  &&  t.callback == cb  &&  t.data == data) {
			return 1;
		}
	}
	return 0;
}

void fltk3::remove_timeout(fltk3::TimeoutHandler cb, void* data)
{
	int i;
	for (i = 0;  i < win32_timer_used;  ++i) {
		Win32Timer& t = win32_timers[i];
		if (t.handle  &&  t.callback == cb  &&
		    (t.data == data  ||  data == NULL)) {
			delete_timer(t);
		}
	}
}

/// END TIMERS
/////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////

HINSTANCE fl_display = GetModuleHandle(NULL);

void fltk3::Window::size_range_()
{
	size_range_set = 1;
}

void Fl_X::set_minmax(LPMINMAXINFO minmax)
{
	int td, wd, hd, dummy_x, dummy_y;

	fake_X_wm(w, dummy_x, dummy_y, td, wd, hd);
	wd *= 2;
	hd *= 2;
	hd += td;

	minmax->ptMinTrackSize.x = w->minw + wd;
	minmax->ptMinTrackSize.y = w->minh + hd;
	if (w->maxw) {
		minmax->ptMaxTrackSize.x = w->maxw + wd;
		minmax->ptMaxSize.x = w->maxw + wd;
	}
	if (w->maxh) {
		minmax->ptMaxTrackSize.y = w->maxh + hd;
		minmax->ptMaxSize.y = w->maxh + hd;
	}
}

////////////////////////////////////////////////////////////////

#include "filename.h" // need so FLTK3_EXPORT fltk3::filename_name works

// returns pointer to the filename, or null if name ends with '/'
const char *fltk3::filename_name(const char *name)
{
	const char *p,*q;
	if (!name) return (0);
	q = name;
	if (q[0] && q[1]==':') q += 2; // skip leading drive letter
	for (p = q; *p; p++) if (*p == '/' || *p == '\\') q = p+1;
	return q;
}

void fltk3::Window::label(const char *name,const char *iname)
{
	Widget::label(name);
	iconlabel_ = iname;
	if (shown() && !parent()) {
		if (!name) name = "";
		int l = (int)strlen(name);
//  WCHAR *lab = (WCHAR*) malloc((l + 1) * sizeof(short));
//  l = fl_utf2unicode((unsigned char*)name, l, (xchar*)lab);
		unsigned wlen = fltk3::utf8toUtf16(name, (unsigned)l, NULL, 0); // Pass NULL to query length
		wlen++;
		unsigned short * lab = (unsigned short*)malloc(sizeof(unsigned short)*wlen);
		wlen = fltk3::utf8toUtf16(name, (unsigned)l, lab, wlen);
		lab[wlen] = 0;
		SetWindowTextW(i->xid, (WCHAR *)lab);
		free(lab);
	}
}

////////////////////////////////////////////////////////////////
// Implement the virtual functions for the base fltk3::Window class:

// If the box is a filled rectangle, we can make the redisplay *look*
// faster by using X's background pixel erasing.  We can make it
// actually *be* faster by drawing the frame only, this is done by
// setting fl_boxcheat, which is seen by code in fl_drawbox.cxx:
// For WIN32 it looks like all windows share a background color, so
// I use fltk3::GRAY for this and only do this cheat for windows that are
// that color.
// Actually it is totally disabled.
// fltk3::Widget *fl_boxcheat;
//static inline int can_boxcheat(uchar b) {return (b==1 || (b&2) && b<=15);}

void fltk3::Window::show()
{
	image(fltk3::scheme_bg_);
	if (fltk3::scheme_bg_) {
		labeltype(fltk3::NORMAL_LABEL);
		align(fltk3::ALIGN_CENTER | fltk3::ALIGN_INSIDE | fltk3::ALIGN_CLIP);
	} else {
		labeltype(fltk3::NO_LABEL);
	}
	fltk3::Tooltip::exit(this);
	if (!shown()) {
		// if (can_boxcheat(box())) fl_background_pixel = fl_xpixel(color());
		Fl_X::make(this);
	} else {
		// Once again, we would lose the capture if we activated the window.
		if (IsIconic(i->xid)) OpenIcon(i->xid);
		if (!fl_capture) BringWindowToTop(i->xid);
		//ShowWindow(i->xid,fl_capture?SW_SHOWNOACTIVATE:SW_RESTORE);
	}
#ifdef USE_PRINT_BUTTON
	void preparePrintFront(void);
	preparePrintFront();
#endif
}

fltk3::Window *fltk3::Window::current_;
// the current context
HDC fl_gc = 0;
// the current window handle, initially set to -1 so we can correctly
// allocate fl_GetDC(0)
HWND fl_window = NULL;

// Here we ensure only one GetDC is ever in place.
HDC fl_GetDC(HWND w)
{
	if (fl_gc) {
		if (w == fl_window  &&  fl_window != NULL) return fl_gc;
		if (fl_window) fl_release_dc(fl_window, fl_gc); // ReleaseDC
	}
	fl_gc = GetDC(w);
	fl_save_dc(w, fl_gc);
	fl_window = w;
	// calling GetDC seems to always reset these: (?)
	SetTextAlign(fl_gc, TA_BASELINE|TA_LEFT);
	SetBkMode(fl_gc, TRANSPARENT);

	return fl_gc;
}

// make X drawing go into this window (called by subclass flush() impl.)
void fltk3::Window::make_current()
{
	fl_GetDC(fl_xid(this));

#if USE_COLORMAP
	// Windows maintains a hardware and software color palette; the
	// SelectPalette() call updates the current soft->hard mapping
	// for all drawing calls, so we must select it here before any
	// code does any drawing...

	fl_select_palette();
#endif // USE_COLORMAP

	current_ = this;
	fltk3::clip_region(0);


}

/* Make sure that all allocated fonts are released. This works only if
   fltk3::run() is allowed to exit by closing all windows. Calling 'exit(int)'
   will not automatically free any fonts. */
void fl_free_fonts(void)
{
// remove the Fl_Font_Descriptor chains
	int i;
	Fl_Fontdesc * s;
	Fl_Font_Descriptor * f;
	Fl_Font_Descriptor * ff;
	for (i=0; i<fltk3::FREE_FONT; i++) {
		s = fltk3::fonts + i;
		for (f=s->first; f; f=ff) {
			ff = f->next;
			delete f;
			s->first = ff;
		}
	}
}


///////////////////////////////////////////////////////////////////////
//
//  The following routines help fix a problem with the leaking of Windows
//  Device Context (DC) objects. The 'proper' protocol is for a program to
//  acquire a DC, save its state, do the modifications needed for drawing,
//  perform the drawing, restore the initial state, and release the DC. In
//  FLTK, the save and restore steps have previously been omitted and DCs are
//  not properly released, leading to a great number of DC leaks. As some
//  Windows "OSs" will hang when any process exceeds roughly 10,000 GDI objects,
//  it is important to control GDI leaks, which are much more important than memory
//  leaks. The following struct, global variable, and routines help implement
//  the above protocol for those cases where the GetDC and RestoreDC are not in
//  the same routine. For each GetDC, fl_save_dc is used to create an entry in
//  a linked list that saves the window handle, the DC handle, and the initial
//  state. When the DC is to be released, 'fl_release_dc' is called. It restores
//  the initial state and releases the DC. When the program exits, 'fl_cleanup_dc_list'
//  frees any remaining nodes in the list.

struct Win_DC_List {      // linked list
	HWND    window;         // window handle
	HDC     dc;             // device context handle
	int     saved_dc;       // initial state of DC
	Win_DC_List * next;     // pointer to next item
};

static Win_DC_List * win_DC_list = 0;

void fl_save_dc( HWND w, HDC dc)
{
	Win_DC_List * t;
	t = new Win_DC_List;
	t->window = w;
	t->dc = dc;
	t->saved_dc = SaveDC(dc);
	if (win_DC_list)
		t->next = win_DC_list;
	else
		t->next = NULL;
	win_DC_list = t;
}

void fl_release_dc(HWND w, HDC dc)
{
	Win_DC_List * t= win_DC_list;
	Win_DC_List * prev = 0;
	if (!t)
		return;
	do {
		if (t->dc == dc) {
			RestoreDC(dc, t->saved_dc);
			ReleaseDC(w, dc);
			if (!prev) {
				win_DC_list = t->next;   // delete first item
			} else {
				prev->next = t->next;       // one in the middle
			}
			delete (t);
			return;
		}
		prev = t;
		t = t->next;
	} while (t);
}

void fl_cleanup_dc_list(void)            // clean up the list
{
	Win_DC_List * t = win_DC_list;
	if (!t)return;
	do {
		RestoreDC(t->dc, t->saved_dc);
		ReleaseDC(t->window, t->dc);
		win_DC_list = t->next;
		delete (t);
		t = win_DC_list;
	} while(t);
}

fltk3::Region XRectangleRegion(int x, int y, int w, int h)
{
	if (fltk3::SurfaceDevice::surface() == fltk3::DisplayDevice::display_device()) return CreateRectRgn(x,y,x+w,y+h);
	// because rotation may apply, the rectangle becomes a polygon in device coords
	POINT pt[4] = { {x, y}, {x + w, y}, {x + w, y + h}, {x, y + h} };
	LPtoDP(fl_gc, pt, 4);
	return CreatePolygonRgn(pt, 4, ALTERNATE);
}

Window fl_xid_(const fltk3::Window *w)
{
	Fl_X *temp = Fl_X::i(w);
	return temp ? temp->xid : 0;
}

int fltk3::Window::decorated_w()
{
	if (!shown() || parent() || !border() || !visible()) return w();
	int X, Y, bt, bx, by;
	Fl_X::fake_X_wm(this, X, Y, bt, bx, by);
	return w() + 2 * bx;
}

int fltk3::Window::decorated_h()
{
	if (!shown() || parent() || !border() || !visible()) return h();
	int X, Y, bt, bx, by;
	Fl_X::fake_X_wm(this, X, Y, bt, bx, by);
	return h() + bt + 2 * by;
}

void fltk3::PagedDevice::print_window(fltk3::Window *win, int x_offset, int y_offset)
{
	if (!win->shown() || win->parent() || !win->border() || !win->visible()) {
		this->print_widget(win, x_offset, y_offset);
		return;
	}
	int X, Y, bt, bx, by, ww, wh; // compute the window border sizes
	Fl_X::fake_X_wm(win, X, Y, bt, bx, by);
	ww = win->w() + 2 * bx;
	wh = win->h() + bt + 2 * by;
	fltk3::DisplayDevice::display_device()->set_current(); // make window current
	win->show();
	fltk3::check();
	win->make_current();
	HDC save_gc = fl_gc;
	fl_gc = GetDC(NULL); // get the screen device context
	// capture the 4 window sides from screen
	RECT r;
	GetWindowRect(fl_window, &r);
	uchar *top_image = fltk3::read_image(NULL, r.left, r.top, ww, bt + by);
	uchar *left_image = fltk3::read_image(NULL, r.left, r.top, bx, wh);
	uchar *right_image = fltk3::read_image(NULL, r.right - bx, r.top, bx, wh);
	uchar *bottom_image = fltk3::read_image(NULL, r.left, r.bottom-by, ww, by);
	ReleaseDC(NULL, fl_gc);
	fl_gc = save_gc;
	this->set_current();
	// print the 4 window sides
	fltk3::draw_image(top_image, x_offset, y_offset, ww, bt + by, 3);
	fltk3::draw_image(left_image, x_offset, y_offset, bx, wh, 3);
	fltk3::draw_image(right_image, x_offset + win->w() + bx, y_offset, bx, wh, 3);
	fltk3::draw_image(bottom_image, x_offset, y_offset + win->h() + bt + by, ww, by, 3);
	delete[] top_image;
	delete[] left_image;
	delete[] right_image;
	delete[] bottom_image;
	// print the window inner part
	this->print_widget(win, x_offset + bx, y_offset + bt + by);
	fl_gc = GetDC(fl_xid(win));
	ReleaseDC(fl_xid(win), fl_gc);
}

#ifdef USE_PRINT_BUTTON
// to test the fltk3::Printer class creating a "Print front window" button in a separate window
// contains also preparePrintFront call above
#include "Printer.h"
#include "Button.h"
void printFront(fltk3::Widget *o, void *data)
{
	fltk3::Printer printer;
	o->window()->hide();
	fltk3::Window *win = fltk3::first_window();
	if(!win) return;
	int w, h;
	if( printer.start_job(1) ) {
		o->window()->show();
		return;
	}
	if( printer.start_page() ) {
		o->window()->show();
		return;
	}
	printer.printable_rect(&w,&h);
	int  wh, ww;
	wh = win->decorated_h();
	ww = win->decorated_w();
	// scale the printer device so that the window fits on the page
	float scale = 1;
	if (ww > w || wh > h) {
		scale = (float)w/ww;
		if ((float)h/wh < scale) scale = (float)h/wh;
		printer.scale(scale, scale);
	}
// #define ROTATE 20.0
#ifdef ROTATE
	printer.scale(scale * 0.8, scale * 0.8);
	printer.printable_rect(&w, &h);
	printer.origin(w/2, h/2 );
	printer.rotate(ROTATE);
	printer.print_widget( win, - win->w()/2, - win->h()/2 );
	//printer.print_window_part( win, 0,0, win->w(), win->h(), - win->w()/2, - win->h()/2 );
#else
	printer.print_window(win);
#endif
	printer.end_page();
	printer.end_job();
	o->window()->show();
}

void preparePrintFront(void)
{
	static BOOL first=TRUE;
	if(!first) return;
	first=FALSE;
	static fltk3::Window w(0,0,120,30);
	static fltk3::Button b(0,0,w.w(),w.h(), "Print front window");
	b.callback(printFront);
	w.end();
	w.show();
}
#endif // USE_PRINT_BUTTON

// call main
#  include <windows.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include "utf8.h"

extern int main(int, char *[]);

#  ifdef BORLAND5
#    define __argc _argc
#    define __argv _argv
#  endif /* BORLAND5 */

/* static int mbcs2utf(const char *s, int l, char *dst, unsigned dstlen) */
static int mbcs2utf(const char *s, int l, char *dst)
{
	static xchar *mbwbuf;
	unsigned dstlen = 0;
	if (!s) return 0;
	dstlen = (l * 6) + 6;
	mbwbuf = (xchar*)malloc(dstlen * sizeof(xchar));
	l = (int)mbstowcs(mbwbuf, s, l);
	/* l = fl_unicode2utf(mbwbuf, l, dst); */
	l = fltk3::utf8fromwc(dst, dstlen, mbwbuf, l);
	dst[l] = 0;
	free(mbwbuf);
	return l;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
LPSTR lpCmdLine, int nCmdShow)
{
	int rc, i;
	char **ar;

#  ifdef _DEBUG
	/*
	 * If we are using compiling in debug mode, open a console window so
	 * we can see any printf's, etc...
	 *
	 * While we can detect if the program was run from the command-line -
	 * look at the CMDLINE environment variable, it will be "WIN" for
	 * programs started from the GUI - the shell seems to run all WIN32
	 * applications in the background anyways...
	 */

	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);
#  endif /* _DEBUG */

	ar = (char**) malloc(sizeof(char*) * (__argc + 1));
	i = 0;
	while (i < __argc) {
		int l;
		unsigned dstlen;
		if (__wargv ) {
			for (l = 0; __wargv[i] && __wargv[i][l]; l++) {}; /* is this just wstrlen??? */
			dstlen = (l * 5) + 1;
			ar[i] = (char*) malloc(dstlen);
			/*    ar[i][fl_unicode2utf(__wargv[i], l, ar[i])] = 0; */
			dstlen = fltk3::utf8fromwc(ar[i], dstlen, __wargv[i], l);
			ar[i][dstlen] = 0;
		} else {
			for (l = 0; __argv[i] && __argv[i][l]; l++) {};
			dstlen = (l * 5) + 1;
			ar[i] = (char*) malloc(dstlen);
			/*      ar[i][mbcs2utf(__argv[i], l, ar[i], dstlen)] = 0; */
			ar[i][mbcs2utf(__argv[i], l, ar[i])] = 0;
		}
		i++;
	}
	ar[__argc] = 0;
	/* Run the standard main entry point function... */
	rc = main(__argc, ar);

#  ifdef _DEBUG
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
#  endif /* _DEBUG */

	return rc;
}

#endif // FLTK3_DOXYGEN

#endif // #ifdef WIN32

//
// End of "$Id: win32.cxx 9626 2012-06-22 23:08:27Z ianmacarthur $".
//
