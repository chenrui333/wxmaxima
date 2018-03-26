﻿// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//  Copyright (C) 2017-2018 Gunter Königsmann <wxMaxima@physikbuch.de>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  SPDX-License-Identifier: GPL-2.0+

/*! \file
  This file declares all the wizards the draw sidepane needs.
 */

#ifndef DRAWWIZ_H
#define DRAWWIZ_H

#include <wx/wx.h>
#include <wx/statline.h>

#include "BTextCtrl.h"
#include "Configuration.h"

//! A wizard for explicit plots using draw
class ExplicitWiz : public wxDialog
{
public:
  ExplicitWiz(wxWindow *parent, Configuration *config, wxString expression, int Dimensions = 2);
  wxString GetValue();
private:
  int m_dimensions;
  BTextCtrl *m_expression;
  BTextCtrl *m_filledfunc;
  BTextCtrl *m_x;
  BTextCtrl *m_xStart;
  BTextCtrl *m_xEnd;
  BTextCtrl *m_y;
  BTextCtrl *m_yStart;
  BTextCtrl *m_yEnd;
};

//! A wizard for implicit plots using draw
class ImplicitWiz : public wxDialog
{
public:
  ImplicitWiz(wxWindow *parent, Configuration *config, wxString expression, int Dimensions = 2);
  wxString GetValue();
private:
  int m_dimensions;
  BTextCtrl *m_expression;
  BTextCtrl *m_x;
  BTextCtrl *m_xStart;
  BTextCtrl *m_xEnd;
  BTextCtrl *m_y;
  BTextCtrl *m_yStart;
  BTextCtrl *m_yEnd;
  BTextCtrl *m_z;
  BTextCtrl *m_zStart;
  BTextCtrl *m_zEnd;
};

//! A wizard for axis setup for draw
class AxisWiz : public wxDialog
{
public:
  AxisWiz(wxWindow *parent, Configuration *config, int Dimensions = 2);
  wxString GetValue();
private:
  int m_dimensions;
  BTextCtrl *m_xLabel;
  BTextCtrl *m_xStart;
  BTextCtrl *m_xEnd;
  BTextCtrl *m_yLabel;
  BTextCtrl *m_yStart;
  BTextCtrl *m_yEnd;
  BTextCtrl *m_zLabel;
  BTextCtrl *m_zStart;
  BTextCtrl *m_zEnd;
  wxCheckBox *m_useSecondaryX;
  wxCheckBox *m_useSecondaryY;
  BTextCtrl *m_x2Label;
  BTextCtrl *m_x2Start;
  BTextCtrl *m_x2End;
  BTextCtrl *m_y2Label;
  BTextCtrl *m_y2Start;
  BTextCtrl *m_y2End;
  BTextCtrl *m_z2Label;
  BTextCtrl *m_z2Start;
  BTextCtrl *m_z2End;
};


#endif // DRAWWIZ_H
