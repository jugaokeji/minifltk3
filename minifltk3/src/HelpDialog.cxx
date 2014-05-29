//
// "$Id: HelpDialog.cxx 9557 2012-05-29 21:17:12Z matt $"
//
// fltk3::HelpDialog dialog for the Fast Light Tool Kit (FLTK).
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

// generated by Fast Light User Interface Designer (fluid) version 1.0108

#include "HelpDialog.h"
#include "flstring.h"
#include "ask.h"

void fltk3::HelpDialog::cb_back__i(fltk3::Button*, void*) {
  if (index_ > 0)
    index_ --;
  
  if (index_ == 0)
    back_->deactivate();
  
  forward_->activate();
  
  int l = line_[index_];
  
  if (strcmp(view_->filename(), file_[index_]) != 0)
    view_->load(file_[index_]);
  
  view_->topline(l);
}
void fltk3::HelpDialog::cb_back_(fltk3::Button* o, void* v) {
  ((fltk3::HelpDialog*)(o->parent()->parent()->user_data()))->cb_back__i(o,v);
}

void fltk3::HelpDialog::cb_forward__i(fltk3::Button*, void*) {
  if (index_ < max_)
    index_ ++;
  
  if (index_ >= max_)
    forward_->deactivate();
  
  back_->activate();
  
  int l = view_->topline();
  
  if (strcmp(view_->filename(), file_[index_]) != 0)
    view_->load(file_[index_]);
  
  view_->topline(l);
}
void fltk3::HelpDialog::cb_forward_(fltk3::Button* o, void* v) {
  ((fltk3::HelpDialog*)(o->parent()->parent()->user_data()))->cb_forward__i(o,v);
}

void fltk3::HelpDialog::cb_smaller__i(fltk3::Button*, void*) {
  if (view_->textsize() > 8)
    view_->textsize(view_->textsize() - 2);
  
  if (view_->textsize() <= 8)
    smaller_->deactivate();
  larger_->activate();
}
void fltk3::HelpDialog::cb_smaller_(fltk3::Button* o, void* v) {
  ((fltk3::HelpDialog*)(o->parent()->parent()->user_data()))->cb_smaller__i(o,v);
}

void fltk3::HelpDialog::cb_larger__i(fltk3::Button*, void*) {
  if (view_->textsize() < 18)
    view_->textsize(view_->textsize() + 2);
  
  if (view_->textsize() >= 18)
    larger_->deactivate();
  smaller_->activate();
}
void fltk3::HelpDialog::cb_larger_(fltk3::Button* o, void* v) {
  ((fltk3::HelpDialog*)(o->parent()->parent()->user_data()))->cb_larger__i(o,v);
}

void fltk3::HelpDialog::cb_find__i(fltk3::Input*, void*) {
  find_pos_ = view_->find(find_->value(), find_pos_);
}
void fltk3::HelpDialog::cb_find_(fltk3::Input* o, void* v) {
  ((fltk3::HelpDialog*)(o->parent()->parent()->parent()->user_data()))->cb_find__i(o,v);
}

void fltk3::HelpDialog::cb_view__i(fltk3::HelpView*, void*) {
  if (view_->filename())
  {
    if (view_->changed())
    {
      index_ ++;
      
      if (index_ >= 100)
      {
        memmove(line_, line_ + 10, sizeof(line_[0]) * 90);
        memmove(file_, file_ + 10, sizeof(file_[0]) * 90);
        index_ -= 10;
      }
      
      max_ = index_;
      
      strlcpy(file_[index_], view_->filename(),sizeof(file_[0]));
      line_[index_] = view_->topline();
      
      if (index_ > 0)
        back_->activate();
      else
        back_->deactivate();
      
      forward_->deactivate();
      window_->label(view_->title());
    }
    else // if ! view_->changed()
    {
      strlcpy(file_[index_], view_->filename(), sizeof(file_[0]));
      line_[index_] = view_->topline();
    }
  } else { // if ! view_->filename()
    index_ = 0; // hitting an internal page will disable the back/fwd buffer
    file_[index_][0] = 0; // unnamed internal page
    line_[index_] = view_->topline();
    back_->deactivate();
    forward_->deactivate();
  };
}
void fltk3::HelpDialog::cb_view_(fltk3::HelpView* o, void* v) {
  ((fltk3::HelpDialog*)(o->parent()->user_data()))->cb_view__i(o,v);
}

fltk3::HelpDialog::HelpDialog() {
  { window_ = new fltk3::DoubleWindow(530, 385, "Help Dialog");
    window_->user_data((void*)(this));
    { fltk3::Group* o = new fltk3::Group(10, 10, 511, 25);
      { back_ = new fltk3::Button(0, 0, 25, 25, "@<-");
        back_->tooltip("Show the previous help page.");
        back_->shortcut(0xff51);
        back_->labelcolor((fltk3::Color)2);
        back_->callback((fltk3::Callback*)cb_back_);
      } // fltk3::Button* back_
      { forward_ = new fltk3::Button(35, 0, 25, 25, "@->");
        forward_->tooltip("Show the next help page.");
        forward_->shortcut(0xff53);
        forward_->labelcolor((fltk3::Color)2);
        forward_->callback((fltk3::Callback*)cb_forward_);
      } // fltk3::Button* forward_
      { smaller_ = new fltk3::Button(70, 0, 25, 25, "F");
        smaller_->tooltip("Make the help text smaller.");
        smaller_->labelfont(1);
        smaller_->labelsize(10);
        smaller_->callback((fltk3::Callback*)cb_smaller_);
      } // fltk3::Button* smaller_
      { larger_ = new fltk3::Button(105, 0, 25, 25, "F");
        larger_->tooltip("Make the help text larger.");
        larger_->labelfont(1);
        larger_->labelsize(16);
        larger_->callback((fltk3::Callback*)cb_larger_);
      } // fltk3::Button* larger_
      { fltk3::Group* o = new fltk3::Group(340, 0, 171, 25);
        o->box(fltk3::DOWN_BOX);
        o->color(fltk3::BACKGROUND2_COLOR);
        { find_ = new fltk3::Input(25, 2, 143, 21, "@search");
          find_->tooltip("find text in document");
          find_->box(fltk3::FLAT_BOX);
          find_->labelsize(13);
          find_->textfont(4);
          find_->callback((fltk3::Callback*)cb_find_);
          find_->when(fltk3::WHEN_ENTER_KEY_ALWAYS);
        } // fltk3::Input* find_
        o->end();
      } // fltk3::Group* o
      { fltk3::Widget* o = new fltk3::Widget(140, 0, 190, 25);
        fltk3::Group::current()->resizable(o);
      } // fltk3::Widget* o
      o->end();
    } // fltk3::Group* o
    { view_ = new fltk3::HelpView(10, 45, 510, 330);
      view_->box(fltk3::DOWN_BOX);
      view_->callback((fltk3::Callback*)cb_view_);
      fltk3::Group::current()->resizable(view_);
    } // fltk3::HelpView* view_
    window_->size_range(260, 150);
    window_->end();
  } // fltk3::DoubleWindow* window_
  back_->deactivate();
  forward_->deactivate();
  
  index_    = -1;
  max_      = 0;
  find_pos_ = 0;
  
  fltk3::register_images();
}

fltk3::HelpDialog::~HelpDialog() {
  delete window_;
}

int fltk3::HelpDialog::h() {
  return (window_->h());
}

void fltk3::HelpDialog::hide() {
  window_->hide();
}

void fltk3::HelpDialog::load(const char *f) {
  view_->set_changed();
  view_->load(f);
  window_->label(view_->title());
}

void fltk3::HelpDialog::position(int xx, int yy) {
  window_->position(xx, yy);
}

void fltk3::HelpDialog::resize(int xx, int yy, int ww, int hh) {
  window_->resize(xx, yy, ww, hh);
}

void fltk3::HelpDialog::show() {
  window_->show();
}

void fltk3::HelpDialog::show(int argc, char **argv) {
  window_->show(argc, argv);
}

void fltk3::HelpDialog::textsize(fltk3::Fontsize s) {
  view_->textsize(s);
  
  if (s <= 8)
    smaller_->deactivate();
  else
    smaller_->activate();
  
  if (s >= 18)
    larger_->deactivate();
  else
    larger_->activate();
}

fltk3::Fontsize fltk3::HelpDialog::textsize() {
  return (view_->textsize());
}

void fltk3::HelpDialog::topline(const char *n) {
  view_->topline(n);
}

void fltk3::HelpDialog::topline(int n) {
  view_->topline(n);
}

void fltk3::HelpDialog::value(const char *f) {
  view_->set_changed();
  view_->value(f);
  window_->label(view_->title());
}

const char * fltk3::HelpDialog::value() const {
  return view_->value();
}

int fltk3::HelpDialog::visible() {
  return (window_->visible());
}

int fltk3::HelpDialog::w() {
  return (window_->w());
}

int fltk3::HelpDialog::x() {
  return (window_->x());
}

int fltk3::HelpDialog::y() {
  return (window_->y());
}

//
// End of "$Id: HelpDialog.cxx 9557 2012-05-29 21:17:12Z matt $".
//
