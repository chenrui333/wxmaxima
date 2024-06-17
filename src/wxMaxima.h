// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2013 Doug Ilijev <doug.ilijev@gmail.com>
//            (C) 2014-2018 Gunter Königsmann <wxMaxima@physikbuch.de>
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

/*!\file
  This file declares the class wxMaxima that contains most of the program's logic.

  The worksheet is defined in the class MathCtrl instead and
  everything surrounding it in wxMaximaFrame.
*/


#ifndef WXMAXIMA_H
#define WXMAXIMA_H

#include <vector>
#include "wxMaximaFrame.h"
#include "WXMXformat.h"
#include "MathParser.h"
#include "MaximaIPC.h"
#include "Dirstructure.h"
#include <wx/socket.h>
#include <wx/config.h>
#include <wx/process.h>
#include <wx/regex.h>
#include <wx/dnd.h>
#include <wx/txtstrm.h>
#include <wx/sckstrm.h>
#include <wx/buffer.h>
#include <wx/power.h>
#include <wx/debugrpt.h>
#include <memory>
#ifdef __WXMSW__
#include <windows.h>
#endif
#include <unordered_map>

//! How many milliseconds should we wait between polling for stdout+cpu power?
#define MAXIMAPOLLMSECS 2000

class Maxima; // The Maxima process interface

/* The top-level window and the main application logic

 */
class wxMaxima : public wxMaximaFrame
{
public:
  wxMaxima(wxWindow *parent, int id, const wxString &title,
           const wxString &filename = wxEmptyString,
           const wxString &initialWorksheetContents = wxEmptyString,
           const wxPoint pos = wxDefaultPosition, const wxSize size = wxDefaultSize);

  virtual ~wxMaxima();
  wxString EscapeFilenameForShell(wxString name);
  //! Exit if we encounter an error
  static void ExitOnError(){m_exitOnError = true;}
  //! Do we exit if we encounter an error?
  static bool GetExitOnError(){return m_exitOnError;}
  /*! Allow maxima to click buttons in wxMaxima

    Disabled by default for security reasons
  */
  static void EnableIPC(){ MaximaIPC::EnableIPC(); }
  //! Do we allow maxima to click buttons in wxMaxima?
  static bool GetEnableIPC(){ return MaximaIPC::GetEnableIPC(); }
  //! Additional maxima arguments from the command line
  static void ExtraMaximaArgs(const wxString &args){m_extraMaximaArgs = args;}
  //! Which additional maxima arguments did we get from the command line?
  static wxString ExtraMaximaArgs(){return m_extraMaximaArgs;}

  //! An enum of individual IDs for all timers this class handles
  enum TimerIDs
  {
    //! The keyboard was inactive long enough that we can attempt an auto-save.
    KEYBOARD_INACTIVITY_TIMER_ID,
    //! The time between two auto-saves has elapsed.
    AUTO_SAVE_TIMER_ID,
    //! We look if we got new data from maxima's stdout.
    MAXIMA_STDOUT_POLL_ID
  };

#ifdef wxHAS_POWER_EVENTS
  void OnPowerEvent(wxPowerEvent &event);
#endif

  //! Is triggered when a timer this class is responsible for requires
  void OnTimerEvent(wxTimerEvent &event);

  //! A timer that polls for output from the maxima process.
  wxTimer m_maximaStdoutPollTimer;

  void ShowTip(bool force);
  //! Do we want to evaluate the document on startup?
  void EvalOnStartup(bool eval)
    {
      m_evalOnStartup = eval;
    }

  //! Do we want to exit the program after evaluation?
  void ExitAfterEval(bool exitaftereval)
    {
      m_exitAfterEval = exitaftereval;
      if(exitaftereval)
      {
        m_logPane->SetBatchMode();
      }
    }

  void StripLispComments(wxString &s);

  //! Launches the help browser on the uri passed as an argument.
  void LaunchHelpBrowser(wxString uri);

  void SendMaxima(wxString s, bool addToHistory = false);

  //! Open a file
  bool OpenFile(const wxString &file, const wxString &command ={});

  //! Does this document need saving?
  bool DocumentSaved()
    { return m_fileSaved; }

  //! Load an image from a file into the worksheet.
  void LoadImage(const wxString &file)
    { GetWorksheet()->OpenHCaret(file, GC_TYPE_IMAGE); }

  //! Query the value of a new maxima variable
  bool QueryVariableValue();

  //! A version number that can be compared using "<" and ">"
  class VersionNumber
  {
  public:
    explicit VersionNumber(const wxString &version);
    long Major() const {return m_major;}
    long Minor() const {return m_minor;}
    long Patchlevel() const {return m_patchlevel;}
    friend bool operator<(const VersionNumber& v1, const VersionNumber& v2);
    friend bool operator>(const VersionNumber& v1, const VersionNumber& v2);
  private:
    long m_major = -1;
    long m_minor = -1;
    long m_patchlevel = -1;
  };

  //! Use this maxima command (from a command line option) instead of the configured path
  static void Set_Maxima_Commandline_Filename(const wxString &filename)
    {maxima_command_line_filename = filename;}
  //! The maxima command, if we use the --maxima=<str> command. If empty we use the configured path, not a command line option --maxima=<str>.
  static const wxString Get_Maxima_Commandline_Filename() {return maxima_command_line_filename;}

  static int GetExitCode(){return m_exitCode;}

private:
  //! If we use the command line option --maxima=<str>, this variable is not-empty and contains its name
  static wxString maxima_command_line_filename;
  //! True if maxima has sent us the correct Auth string
  bool m_maximaAuthenticated = false;
  //! True if maxima has failed to authenticate and we therefore distrust its data.
  bool m_discardAllData = false;
  //! The string Maxima needs to know in order to prove to be the correct process
  wxString m_maximaAuthString;
  //! The object that allows maxima to send us GUI events for testing purposes
  MaximaIPC m_ipc{this};
  //! True if we want to exit if we encounter an error
  static bool m_exitOnError;
  //! Extra arguments wxMaxima's command line told us to pass to maxima
  static wxString m_extraMaximaArgs;
  //! The variable names to query for the variables pane and for internal reasons
  std::vector<wxString> m_varNamesToQuery;

  //! Is true if opening the file from the command line failed before updating the statusbar.
  bool m_openInitialFileError = false;
  //! Escape strings into a format lisp accepts
  wxString EscapeForLisp(wxString str);
  //! The number of Jiffies Maxima had used the last time we asked
  long long m_maximaJiffies_old = 0;
  //! The number of Jiffies the CPU had made the last time
  long long m_cpuTotalJiffies_old = 0;
  //! Do we need to update the menus + toolbars?
  //! All configuration commands we still have to send to maxima
  wxString m_configCommands;
  //! A RegEx that matches gnuplot errors.
  static wxRegEx m_gnuplotErrorRegex;
  //! Clear the evaluation queue and return true if "Abort on Error" is set.
  bool AbortOnError();
  //! This string allows us to detect when the string we search for has changed.
  wxString m_oldFindString;
  //! This string allows us to detect when the string we search for has changed.
  int m_oldFindFlags = 0;
  //! On opening a new file we only need a new maxima process if the old one ever evaluated cells.
  bool m_hasEvaluatedCells = false;
  //! The number of output cells the current command has produced so far.
  long m_outputCellsFromCurrentCommand = 0;
  //! The maximum number of lines per command we will display
  long m_maxOutputCellsPerCommand = 0;
  /*! Double the number of consecutive unsuccessful attempts to connect to the maxima server

    Each prompt is deemed as but one hint for a working maxima while each crash counts twice
    which hinders us from endlessly restarting in case maxima crashes, outputs something
    seemingly sensible and crashes again.
  */
  int m_unsuccessfulConnectionAttempts = 11;
  //! The current working directory maxima's file I/O is relative to.
  wxString m_CWD;
  //! Do we want to evaluate the file after startup?
  bool m_evalOnStartup = false;
  //! Do we want to exit the program after the evaluation was successful?
  bool m_exitAfterEval = false;
  //! Can we display the "ready" prompt right now?
  bool m_ready = false;

  /*! A human-readable presentation of eventual unmatched-parenthesis type errors

    If text doesn't contain any error this function returns wxEmptyString
  */
  wxString GetUnmatchedParenthesisState(wxString text, std::size_t &index);
  //! The buffer all text from maxima is stored in before converting it to a wxString.
  wxMemoryBuffer m_uncompletedChars;

protected:
  //! Invoke our standard wizard that generates maxima commands
  void CommandWiz(const wxString &title,
                  const wxString &description, const wxString &description_tooltip,
                  const wxString &commandRule,
                  wxString label1, wxString defaultval1, wxString tooltip1 = {},
                  wxString label2 = {}, wxString defaultval2 = {}, wxString tooltip2 = {},
                  wxString label3 = {}, wxString defaultval3 = {}, wxString tooltip3 = {},
                  wxString label4 = {}, wxString defaultval4 = {}, wxString tooltip4 = {},
                  wxString label5 = {}, wxString defaultval5 = {}, wxString tooltip5 = {},
                  wxString label6 = {}, wxString defaultval6 = {}, wxString tooltip6 = {},
                  wxString label7 = {}, wxString defaultval7 = {}, wxString tooltip7 = {},
                  wxString label8 = {}, wxString defaultval8 = {}, wxString tooltip8 = {},
                  wxString label9 = {}, wxString defaultval9 = {}, wxString tooltip9 = {}
    );
  //! Reads a potentially unclosed XML tag and closes it
  wxString ReadPotentiallyUnclosedTag(wxStringTokenizer &lines, wxString firstLine);

  //! Finds the name of an opening tag
  static wxRegEx m_xmlOpeningTagName;
  //! Looks if this opening tag is actually complete.
  static wxRegEx m_xmlOpeningTag;
  //! The gnuplot process info
  std::unique_ptr<wxProcess> m_gnuplotProcess;
  //! Info about the gnuplot process we start for querying the terminals it supports
  std::unique_ptr<wxProcess>  m_gnuplotTerminalQueryProcess;
  //! Is this window active?
  bool m_isActive = true;
  //! Called when this window is focussed or defocussed.
  void OnFocus(wxFocusEvent &event);

  //! Forwards the keyboard focus to a text control that might need it
  void PassKeyboardFocus();
  //! Called when this window is minimized.
  void OnMinimize(wxIconizeEvent &event);
  //! Is called on start and whenever the configuration changes
  void ConfigChanged();
  //! Called when the "Scroll to last error" button is pressed.
  void OnJumpToError(wxCommandEvent &event);
  //! Sends a new char to the symbols sidebar
  void OnSymbolAdd(wxCommandEvent &event);
  //! Called when the "Scroll to currently evaluated" button is pressed.
  void OnFollow(wxCommandEvent &event);
  void OnWizardAbort(wxCommandEvent &event);
  void OnWizardOK(wxCommandEvent &event);
  void OnWizardInsert(wxCommandEvent &event);
  void OnWizardHelpButton(wxCommandEvent &event);

  //! Show the help for Maxima
  void ShowMaximaHelp(wxString = {});

  //! Show the help for Maxima (without handling of anchors).
  void ShowMaximaHelpWithoutAnchor();

  //! Show the help for wxMaxima
  void ShowWxMaximaHelp();

  //! Try to determine if help is needed for maxima or wxMaxima and show this help
  void ShowHelp(const wxString &keyword);

  void CheckForUpdates(bool reportUpToDate = false);

  void OnRecentDocument(wxCommandEvent &event);
  void OnRecentPackage (wxCommandEvent &event);
  void OnUnsavedDocument(wxCommandEvent &event);

  void OnChar(wxKeyEvent &event);
  void OnKeyDown(wxKeyEvent &event);

  /*! The idle task that refreshes the gui (worksheet, menus, title line,...)

    In GUIs normally all events (mouse events, key presses,...) are
    put into a queue and then are executed one by one. This makes
    sure that they will be processed in order - but makes a gui a
    more or less single-processor-task.

    If the queue is empty (which means that the computer has caught
    up with the incoming events) this function is called once.

    Moving the screen update to this function makes sure that we
    don't do many subsequent updates slowing down the computer any
    further if there are still a handful of key presses to process
    lowering the average response times of the program.

    The worksheet is refreshed by a timer task in case the computer
    is too busy to execute the idle task at all.
  */
  void OnIdle(wxIdleEvent &event);
  bool m_dataFromMaximaIs = false;

  void MenuCommand(const wxString &cmd);           //!< Inserts command cmd into the worksheet
  void FileMenu(wxCommandEvent &event);            //!< Processes "file menu" clicks
  void MaximaMenu(wxCommandEvent &event);          //!< Processes "maxima menu" clicks
  void MatrixMenu(wxCommandEvent &event);         //!< Processes "algebra menu" clicks
  void PropertiesMenu(wxCommandEvent &event);         //!< Processes "Variable/Function props menu" clicks
  void EquationsMenu(wxCommandEvent &event);       //!< Processes "equations menu" clicks
  void CalculusMenu(wxCommandEvent &event);        //!< event handling for menus
  void SimplifyMenu(wxCommandEvent &event);        //!< Processes "Simplify menu" clicks
  void PlotMenu(wxCommandEvent &event);            //!< Processes "Plot menu" cloicks
  void ListMenu(wxCommandEvent &event);            //!< Processes "list menu" clicks
  void DrawMenu(wxCommandEvent &event);            //!< Processes "draw menu" clicks
  void NumericalMenu(wxCommandEvent &event);       //!< Processes "Numerical menu" clicks
  void HelpMenu(wxCommandEvent &event);            //!< Processes "Help menu" clicks
  void EditMenu(wxCommandEvent &event);            //!< Processes "Edit menu" clicks
  void ReplaceSuggestion(wxCommandEvent &event);   //!< Processes clicks on suggestions
  void Interrupt(wxCommandEvent &event);           //!< Interrupt button and hotkey presses
  //! Make the menu item, toolbars and panes visible that should be visible right now.
  void UpdateMenus();        //!< Enables and disables the Right menu buttons
  void UpdateToolBar();      //!< Enables and disables the Right toolbar buttons
  void UpdateSlider();       //!< Updates the slider to show the right frame
  /*! Toggle the visibility of a pane
    \param event The event that triggered calling this function.
  */
  void ShowPane(wxCommandEvent &event);            //<! Makes a sidebar visible
  void OnMaximaClose(wxProcessEvent &event);      //
  void OnMaximaClose();      //
  void OnGnuplotClose(wxProcessEvent &event);      //
  void OnGnuplotQueryTerminals(wxProcessEvent &event);      //
  void PopupMenu(wxCommandEvent &event);           //
  void StatsMenu(wxCommandEvent &event);           //

  //! Is triggered when the textstyle drop-down box's value is changed.
  void ChangeCellStyle(wxCommandEvent &event);

  //! Is triggered when the "Find" button in the search dialog is pressed
  void OnFind(wxFindDialogEvent &event);

  //! Is triggered when the "Replace" button in the search dialog is pressed
  void OnReplace(wxFindDialogEvent &event);

  //! Is triggered when the "Replace All" button in the search dialog is pressed
  void OnReplaceAll(wxFindDialogEvent &event);

  //! Is called if maxima connects to wxMaxima.
  void OnMaximaConnect();

  //! Maxima sends or receives data, or disconnects
  void MaximaEvent(wxThreadEvent &event);

  //! Server event: Maxima connects
  void ServerEvent(wxSocketEvent &event);

  /*! Add a parameter to a draw command

    \param cmd The parameter to  add to the draw command
    \param dimensionsOfNewDrawCommand The number of dimensions the new draw command needs to
    have if we need to create one..
  */
  void AddDrawParameter(wxString cmd, int dimensionsOfNewDrawCommand = 2);

  /* Append something to the console. Might be Text or XML maths.

     \return A pointer to the last line of Unicode text that was appended or
     NULL, if there is no such line (for example if the appended object is
     maths instead).
  */
  TextCell *ConsoleAppend(wxString s, CellType type);        //!< append maxima output to console
  void ConsoleAppend(wxXmlDocument xml, CellType type, const wxString &userLabel = {});        //!< append maxima output to console

  enum AppendOpt { NewLine = 1, BigSkip = 2, PromptToolTip = 4, DefaultOpt = NewLine|BigSkip };
  void DoConsoleAppend(wxString s, CellType type, AppendOpt opts = AppendOpt::DefaultOpt,
                       const wxString &userLabel = {});

  /*!Append one or more lines of ordinary unicode text to the console

    \return A pointer to the last line that was appended or NULL, if there is no such line
  */
  TextCell *DoRawConsoleAppend(wxString s, CellType  type, AppendOpt opts = {});

  /*! Spawn the "configure" menu.

    \todo Inform maxima about the new default plot window size.
  */
  void EditInputMenu(wxCommandEvent &event);
  //! Trigger reading all variables from Maxima that are shown in the Variables sidebar
  void VarReadEvent(wxCommandEvent &event);
  //! Trigger adding all variables to the variables sidebar
  void VarAddAllEvent(wxCommandEvent &event);
  void EvaluateEvent(wxCommandEvent &event);       //
  void InsertMenu(wxCommandEvent &event);          //
  void PrintMenu(wxCommandEvent &event);

  void SliderEvent(wxScrollEvent &event);

  //! Issued on double click on the network status
  void NetworkDClick(wxCommandEvent &ev);
  //! Issued on double click on the Maxima status icon
  void MaximaDClick(wxCommandEvent &ev);
  //! Issued on double click on the status message in the status bar
  void StatusMsgDClick(wxCommandEvent &ev);

  //! Issued on double click on a history item
  void HistoryDClick(wxCommandEvent &event);

  //! Issued on double click on a table of contents item
  void TableOfContentsSelection(wxListEvent &event);

  void OnInspectorEvent(wxCommandEvent &ev);

  void DumpProcessOutput();

  //! Try to evaluate the next command for maxima that is in the evaluation queue
  void TriggerEvaluation();

  void TryUpdateInspector();

  bool UpdateDrawPane();

  wxString ExtractFirstExpression(const wxString &entry);

  wxString GetDefaultEntry();

  //! starts the server
  bool StartServer();
  /*! starts maxima (uses getCommand) or restarts it if needed

    Normally a restart is only needed if
    - maxima isn't currently in the process of starting up or
    - maxima is running and has never evaluated any program
    so a restart won't change anything
    \param force true means to restart maxima unconditionally.
  */
  bool StartMaxima(bool force = false);

  void OnClose(wxCloseEvent &event);               //!< close wxMaxima window
  wxString GetCommand(bool params = true);         //!< returns the command to start maxima
  //    (uses guessConfiguration)

  //! Polls the stderr and stdout of maxima for input.
  void ReadStdErr();

  /*! Determines the process id of maxima from its initial output

    This function does several things:
    - it sets m_pid to the process id of maxima
    - it discards all data until this point
    - and it prepares the worksheet for editing.

    \param data The string ReadFirstPrompt() does read its data from.
    After leaving this function data is empty again.
  */
  void ReadFirstPrompt(const wxString &data);

  /*! Reads text that isn't enclosed between xml tags.

    Some commands provide status messages before the math output or the command has finished.
    This function makes wxMaxima output them directly as they arrive.

    After processing the lines not enclosed in xml tags they are removed from data.
  */
  void ReadMiscText(const wxString &data);

  /*! Reads the input prompt from Maxima.

    After processing the input prompt it is removed from data.
  */
  void ReadPrompt(const wxString &data);

  /*! Reads the output of wxstatusbar() commands

    wxstatusbar allows the user to give and update visual feedback from long-running
    commands and makes sure this feedback is deleted once the command is finished.
  */
  void ReadStatusBar(const wxXmlDocument &xmldoc);
  //! Read a manual topic name so we can jump to the right documentation page
  void ReadManualTopicNames(const wxXmlDocument &xmldoc);

  /*! Reads the math cell's contents from Maxima.

    Math cells are enclosed between the tags \<mth\> and \</mth\>.
    This function removes the from data after appending them
    to the console.

    After processing the status bar marker is removed from data.
  */
  void ReadMath(const wxXmlDocument &xml);

  /*! Reads autocompletion templates we get on definition of a function or variable

    After processing the templates they are removed from data.
  */

  void ReadMaximaIPC(const wxString &data){m_ipc.ReadInputData(data);}
  void ReadLoadSymbols(const wxXmlDocument &data);

  //! Read (and discard) suppressed output
  void ReadSuppressedOutput(const wxString &data);

  /*! Reads the variable values maxima advertises to us
   */
  void ReadVariables(const wxXmlDocument &xmldoc);

  /*! Reads the "add variable to watch list" tag maxima can send us
   */
  void ReadAddVariables(const wxXmlDocument &xmldoc);
  //! Called if maxima tells us the value of the maxima variable gentranlang.
  void VariableActionGentranlang(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable numer.
  void VariableActionNumer(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable display2d_unicode.
  void VariableActionDisplay2d_Unicode(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable output_format_for_help.
  void VariableActionHtmlHelp(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable opsubst.
  void VariableActionOpSubst(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable sinnpiflag.
  void VariableActionSinnpiflag(const wxString &value);
  //! Called if maxima tells us that the maxima variable sinnpiflag is undefined.
  void VariableActionSinnpiflagUndefined();
  //! Called if maxima tells us the value of the maxima variable logexpand.
  void VariableActionLogexpand(const wxString &value);
  //! Called if maxima tells us where the user files lie.
  void VariableActionUserDir(const wxString &value);
  //! Called if maxima tells us where the temp files lie.
  void VariableActionTempDir(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable debugmode.
  void VariableActionDebugmode(const wxString &value);
  //! Called if maxima tells us the maxima version as defined by autoconf.
  void VariableActionAutoconfVersion(const wxString &value);
  //! Called if maxima tells us the maxima build host as defined by autoconf.
  void VariableActionAutoconfHost(const wxString &value);
  //! Called if maxima tells us the maxima info dir.
  void VariableActionMaximaInfodir(const wxString &value);
  //! Called if maxima tells us the maxima html dir.
  void VariableActionMaximaHtmldir(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable <code>gnuplot</code>
  void VariableActionGnuplotCommand(const wxString &value);
  //! Called if maxima tells us the maxima share dir.
  void VariableActionMaximaSharedir(const wxString &value);
  //! Called if maxima tells us the lisp name.
  void VariableActionLispName(const wxString &value);
  //! Called if maxima tells us the lisp version.
  void VariableActionLispVersion(const wxString &value);
  //! Called if maxima tells us the name of a package that was loaded
  void VariableActionWxLoadFileName(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable <code>wxsubscripts</code>
  void VariableActionWxSubscripts(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable <code>lmxchar</code>
  void VariableActionLmxChar(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable <code>display2d</code>
  void VariableActionDisplay2D(const wxString &value);
  //! Called if maxima tells us if it currently outputs XML
  void VariableActionAltDisplay2D(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable <code>algebraic</code>
  void VariableActionAlgebraic(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable <code>showtime</code>
  void VariableActionShowtime(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable <code>domain</code>
  void VariableActionDomain(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable <code>wxanimate_autoplay</code>
  void VariableActionAutoplay(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable <code>engineering_format_floats</code>
  void VariableActionEngineeringFormat(const wxString &value);
  //! Called if maxima tells us the value of the maxima variable <code>stringdisp</code>
  void VariableActionStringdisp(const wxString &value);
  //! Called if maxima sends us the list of known operators
  void VariableActionOperators(const wxString &value);
  wxString m_maximaVariable_wxSubscripts;
  wxString m_maximaVariable_lmxchar;
  wxString m_maximaVariable_display2d;
  wxString m_maximaVariable_altdisplay2d;
  wxString m_maximaVariable_engineeringFormat;
  bool m_readMaximaVariables = false;
  bool m_updateAutocompletion = true;
  /*! How much CPU time has been used by the system until now? Used by GetMaximaCPUPercentage.

    \return The CPU time elapsed in the same unit as GetMaximaCpuTime(); -1 means: Unable to determine this value.
  */
  long long GetTotalCpuTime();

  /*! How much CPU time has maxima used till now? Used by GetMaximaCPUPercentage.

    \return The CPU time maxima has used in the same unit as GetTotalCpuTime(); -1 means: Unable to determine this value.

  */
  long long GetMaximaCpuTime();

  /*! How much CPU horsepower is maxima using currently?

    \return The percentage of the CPU horsepower maxima is using or -1, if this value is unknown.
  */
  double GetMaximaCPUPercentage();

  //! Does this file contain anything worth saving?
  bool SaveNecessary();

  /*! Setup maxima's variables

    This method is called once when maxima starts. It loads wxmathml.lisp
    and sets some option variables.

    \todo Set pngcairo to be the default terminal as soon as the mac platform
    supports it.
  */
  void SetupVariables();

  void KillMaxima(bool logMessage = true);                 //!< kills the maxima process
  /*! Update the title

    Updates the "saved" status, as well, but does only do anything if saved has
    changed or force is true.
    \param saved The new "saved" status
    \param force Force update if the "saved" status hasn't changed.
  */
  void ResetTitle(bool saved, bool force = false);

  void FirstOutput();

  /*! Opens a content.xml file that has been extracted from a broken .wxmx file
   */
  bool OpenXML(const wxString &file, Worksheet *document);

  //! Complains if the version string from the XML file indicates too low a maxima version
  bool CheckWXMXVersion(const wxString &docversion);

  //! Opens a .mac file or a .out file from Xmaxima
  bool OpenMACFile(const wxString &file, Worksheet *document, bool clearDocument = true);

  //! Opens a wxm file
  bool OpenWXMFile(const wxString &file, Worksheet *document, bool clearDocument = true);

  //! Opens a wxmx file
  bool OpenWXMXFile(const wxString &file, Worksheet *document, bool clearDocument = true);

  //! Loads a wxmx description
  std::unique_ptr<GroupCell> CreateTreeFromXMLNode(wxXmlNode *xmlcells, const wxString &wxmxfilename = {});

  /*! Saves the current file

    \param forceSave true means: Always ask for a file name before saving.
    \return true, if the file was saved - or didn't need to
  */
  bool SaveFile(bool forceSave = false);

  //! Try to save the file before closing it - or return false
  bool SaveOnClose();
  /*! Save the project in a temp file.

    Returns false if a save was necessary, but not possible.
  */
  bool AutoSave();

  /*! Tries or offers to save the document

    If auto-save is on the document is automatically saved.

    \return
    * wxID_NO: No saving is necessary, currently
    */
  int SaveDocumentP();

  //! Set the current working directory file I/O from maxima is relative to.
  void SetCWD(wxString file);

  //! Get the current working directory file I/O from maxima is relative to.
  wxString GetCWD()
    {
      return m_CWD;
    }

  std::unique_ptr<Maxima> m_client;
  /*! The Right Way to delete a wxSocketServer

    The destructor might delete the server before all pending server events have been
    processed which leads to a crash.
  */
  struct ServerDeleter {
    void operator()(wxSocketServer* server) const {
      server->Destroy(); // Destroy() calls Close() automatically.
    }
  };
  /*! The server maxima connects to as client

    The destructor of the server causes
    crashes if there are still pending events.
    Instead we need to call destroy.
  */
  std::unique_ptr<wxSocketServer,  ServerDeleter> m_server;

  std::unique_ptr<wxProcess>  m_maximaProcess;
  //! The stdout of the maxima process
  wxInputStream *m_maximaStdout = NULL;
  //! The stderr of the maxima process
  wxInputStream *m_maximaStderr = NULL;
  //! The port the actual maxima process (not its wrapper script) runs at
  int m_port = -1;
  //! A marker for the start of maths
  static wxString m_mathPrefix1;
  //! A marker for the start of maths
  static wxString m_mathPrefix2;
  //! The marker for the start of a input prompt
  static wxString m_promptPrefix;
public:
  //! The marker for the end of a input prompt
  const static wxString m_promptSuffix;
protected:
  /*! True = schedule an update of the table of contents

    used by UpdateTableOfContents() and the idle task.
  */
  bool m_scheduleUpdateToc = false;
  void QuestionAnswered(){if(GetWorksheet()) GetWorksheet()->QuestionAnswered();}
    //! Is called when we get a new list of demo files
  //! Is called when we get a new list of demo files
  void OnNewDemoFiles(wxCommandEvent &event);
  //! Is called when a demo file menu is clicked
  void OnDemoFileMenu(wxCommandEvent &ev);

  //! Is called when something requests an update of the table of contents
  void OnUpdateTOCEvent(wxCommandEvent &event);

  void OnSize(wxSizeEvent &event);
  void OnMove(wxMoveEvent &event);
  void OnMaximize(wxCommandEvent &event);

  //! Sets gnuplot's command name and tries to determine gnuplot's path
  void GnuplotCommandName(wxString gnuplot);
  //! The first prompt maxima will output
  static wxString m_firstPrompt;
  bool m_dispReadOut = false;               //!< what is displayed in statusbar
  wxWindowIDRef m_gnuplot_process_id;
  wxWindowIDRef m_maxima_process_id;
  wxString m_lastPrompt;
  wxString m_lastPath;
  std::unique_ptr<wxPrintData> m_printData;
  /*! Did we tell maxima to close?

    If we didn't we respan an unexpectedly-closing maxima.
  */
  bool m_closing = false;
  //! The directory with maxima's temp files
  wxString m_maximaTempDir;
  wxString m_maximaHtmlDir;
  bool m_fileSaved = true;
  static int m_exitCode;
  //! Maxima's idea about gnuplot's location
  wxString m_gnuplotcommand;
  //! The Char the current command starts at in the current WorkingGroup
  long m_commandIndex = -1;
  FindReplacePane::FindReplaceData m_findData;
  static wxRegEx m_funRegEx;
  static wxRegEx m_varRegEx;
  static wxRegEx m_blankStatementRegEx;
  static wxRegEx m_sbclCompilationRegEx;
  MathParser m_parser;
  bool m_maximaBusy = true;
private:
  bool m_fourierLoaded = false;
  //! The value of maxima's logexpand variable
  wxString m_logexpand;
  //! A pointer to a method that handles a text chunk
  typedef void (wxMaxima::*VarReadFunction)(const wxString &value);
  typedef void (wxMaxima::*VarUndefinedFunction)();
  typedef std::unordered_map <wxString, VarReadFunction, wxStringHash> VarReadFunctionHash;
  typedef std::unordered_map <wxString, VarUndefinedFunction,
                              wxStringHash> VarUndefinedFunctionHash;
  //! A list of actions we want to execute if we are sent the contents of specific variables
  static VarReadFunctionHash m_variableReadActions;
  //! A list of actions we want to execute if certain variables are undefined;
  static VarUndefinedFunctionHash m_variableUndefinedActions;

#if wxUSE_DRAG_AND_DROP

  friend class MyDropTarget;

#endif
  friend class MaximaIPC;

  /*! A timer that determines when to do the next autosave;

    The actual autosave is triggered if both this timer is expired and the keyboard
    has been inactive for >10s so the autosave won't cause the application to shortly
    stop responding due to saving the file while the user is typing a word.

    This timer is used in one-shot mode so in the unlikely case that saving needs more
    time than this timer to expire the user still got a chance to do something against
    it between two expirys.
  */
  wxTimer m_autoSaveTimer;

  /* A timer that delays redraws while maxima evaluates

     If we always start a redraw when maxima has nearly finished a command that slows
     down evaluating many simple commands in a row.
  */
  wxTimer m_fastResponseTimer;

  //! Starts a single-shot of m_autoSaveTimer.
  void StartAutoSaveTimer();
};

#if wxUSE_DRAG_AND_DROP

// cppcheck-suppress noConstructor
class MyApp : public wxApp
{
public:
  virtual bool OnInit();
  virtual int OnRun();
  virtual int OnExit();
#if wxUSE_ON_FATAL_EXCEPTION && wxUSE_CRASHREPORT
  void	OnFatalException () override;
#endif
  /*! Create a new window

    The mac platform insists in making all windows of an application
    share the same process. On the other platforms we create a separate
    process for every wxMaxima session instead which means that each
    process uses the NewWindow() function only once.

    \param file The file name
    \param evalOnStartup Do we want to execute the file automatically, but halt on error?
    \param exitAfterEval Do we want to close the window after the file has been evaluated?
    \param wxmData A .wxm file containing the initial worksheet contents
    \param wxmLen  The length of wxmData
  */
  static void NewWindow(const wxString &file = {}, bool evalOnStartup = false, bool exitAfterEval = false, unsigned char *wxmData = NULL, std::size_t wxmLen = 0);

  void OnFileMenu(wxCommandEvent &ev);

  virtual void MacNewFile();
  void BecomeLogTarget();

  virtual void MacOpenFile(const wxString &file);

private:
  static std::vector<std::unique_ptr<wxProcess>> m_wxMaximaProcesses;
#if wxUSE_ON_FATAL_EXCEPTION && wxUSE_CRASHREPORT
  void GenerateDebugReport(wxDebugReport::Context ctx);
#endif
  std::unique_ptr<wxLocale> m_locale;
  std::unique_ptr<wxTranslations> m_translations;
  //! The name of the config file. Empty = Use the default one.
  wxString m_configFileName;
  Dirstructure m_dirstruct;
  static bool m_allWindowsInOneProcess;
};

class MyDropTarget : public wxFileDropTarget
{
public:
  explicit MyDropTarget(wxMaxima *wxmax)
    { m_wxmax = wxmax; }

  bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString &files);

private:
  wxMaxima *m_wxmax = NULL;
};

#endif


// cppcheck-suppress noConstructor

#endif // WXMAXIMA_H
