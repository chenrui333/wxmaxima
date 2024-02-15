// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
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
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
//  SPDX-License-Identifier: GPL-2.0+

#ifndef LOGPANE_H
#define LOGPANE_H

#include "precomp.h"
#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include "ErrorRedirector.h"
#include "stx/optional.hpp"

/*! A "debug messages" sidepane

 */
class LogPane : public wxPanel
{
public:
  //! The destructor
  explicit LogPane(wxWindow *parent, wxWindowID id = wxID_ANY, bool becomeLogTarget = true);
  /*! Causes this log pane to receive all log messages from wxWidgets

    Important on MacOs, where all active windows need to be handled from the same
    wxMaxima process
  */
  void BecomeLogTarget();
  //! Causes log messages to be output to stderr, as well.
  void SetBatchMode() {
    if (m_errorRedirector)
      m_errorRedirector->SetBatchMode();
  }
  /*! Stop receiving log messages

    Tells wxLog to discard all log messages.
   */
  void DropLogTarget();
  //! True, if this is the log panel that receives all log messages
  bool IsLogTarget() {return m_logPanelTarget.has_value();}
  //! The destructor
  ~LogPane();

private:  
  //! The textctrl all log messages appear on
  wxTextCtrl *m_textCtrl;
  //! Shows all error messages on gui dialogues
  stx::optional<wxLogTextCtrl> m_logPanelTarget;
  //! Redirects error messages - here to a wxLog
  stx::optional<ErrorRedirector> m_errorRedirector;
#if wxUSE_STD_IOSTREAM
  stx::optional<wxStreamToTextRedirector> m_textRedirector;
#endif
};

#endif // LOGPANE_H
