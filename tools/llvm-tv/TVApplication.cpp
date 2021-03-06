//===-- TVApplication.cpp - Main application class for llvm-tv ------------===//
//
// The gui for llvm-tv.
//
//===----------------------------------------------------------------------===//

#include "CodeViewer.h"
#include "TVFrame.h"
#include "llvm-tv/Support/FileUtils.h"
#include "llvm-tv/Config.h"
#undef _DEBUG
#undef __WXDEBUG__
#include "TVApplication.h"
#include "wxUtils.h"
#include <wx/image.h>
#include <cerrno>
#include <iostream>
#include <fstream>
#include <functional>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

///==---------------------------------------------------------------------==///

static TVApplication *TheApp;

void sigHandler(int sigNum) {
  TheApp->ReceivedSignal();
}

void TVApplication::ReceivedSignal () {
  // Whenever we catch our prearranged signal, refresh the snapshot list.
  myFrame->refreshSnapshotList();
}

/// FatalErrorBox - pop up an error message and quit.
///
void FatalErrorBox (const std::string msg) {
  wxMessageBox(wxS(msg), wxT("Fatal Error"), wxOK | wxICON_ERROR);
  exit (1);
}

static void setUpMenus (wxFrame *frame) {
  wxMenuBar *menuBar = new wxMenuBar ();

  wxMenu *fileMenu = new wxMenu (wxT(""), 0);
  fileMenu->Append (wxID_OPEN, wxT("Add module..."));
  fileMenu->Append (LLVM_TV_REFRESH, wxT("Refresh list"));
  fileMenu->Append (wxID_EXIT, wxT("Quit"));
  menuBar->Append (fileMenu, wxT("File"));

  wxMenu *viewMenu = new wxMenu (wxT(""), 0);
  viewMenu->Append (LLVM_TV_CALLGRAPHVIEW, wxT("View call graph"));
  viewMenu->Append (LLVM_TV_CFGVIEW, wxT("View control-flow graph"));
  viewMenu->Append (LLVM_TV_BUDS_VIEW, wxT("View BU datastructure graph"));
  viewMenu->Append (LLVM_TV_TDDS_VIEW, wxT("View TD datastructure graph"));
  viewMenu->Append (LLVM_TV_LOCALDS_VIEW, wxT("View Local datastructure graph"));
  viewMenu->Append (LLVM_TV_CODEVIEW, wxT("View code (interactive)"));
  menuBar->Append (viewMenu, wxT("View"));

  wxMenu *helpMenu = new wxMenu (wxT(""), 0);
  helpMenu->Append (wxID_HELP_CONTENTS, wxT("Help with LLVM-TV"));
  helpMenu->Append (wxID_ABOUT, wxT("About LLVM-TV"));
  menuBar->Append (helpMenu, wxT("Help"));

  frame->SetMenuBar (menuBar);
}

IMPLEMENT_APP (TVApplication)

/// saveMyPID - Save my process ID into a temporary file.
static void saveMyPID () {
  EnsureDirectoryExists (llvmtvPath);

  std::ofstream pidFile (llvmtvPID.c_str ());
  if (pidFile.good () && pidFile.is_open ()) {
    pidFile << getpid ();
    pidFile.close ();
  } else {
    std::cerr << "Warning: could not save PID into " << llvmtvPID << "\n";
  }
}

// eraseMyPID - Erase the PID file created by saveMyPID.
static void eraseMyPID () {
  unlink (llvmtvPID.c_str ());
}

void TVApplication::GoodbyeFrom (wxWindow *dyingWindow) {
  std::vector<wxWindow *>::iterator where =
    find (allMyWindows.begin(), allMyWindows.end(), dyingWindow);
  if (where != allMyWindows.end ())
    allMyWindows.erase (where);
}

void TVApplication::Quit () {
  // Destroy all the picture windows, then the toplevel window.
  for_each (allMyWindows.begin (), allMyWindows.end (),
            std::mem_fun (&wxWindow::Destroy));
  myFrame->Destroy ();
}

bool TVApplication::OnInit () {
  // Save my PID into the file where the snapshot-making pass knows to
  // look for it.
  saveMyPID ();
  atexit (eraseMyPID);

  wxInitAllImageHandlers ();

  // Build top-level window.
  myFrame = new TVFrame (this, "LLVM Visualizer");
  SetTopWindow (myFrame);

  // Build top-level window's menu bar.
  setUpMenus (myFrame);

  // Read the snapshot list out of the given directory,
  // and load the snapshot list view into the frame.
  EnsureDirectoryExists (snapshotsPath);
  
  snapshotList = new TVSnapshotList(snapshotsPath);
  
  myFrame->initializeSnapshotListAndView();

  // Set up signal handler so that we can get notified when
  // the -snapshot pass hands us new snapshot bytecode files.
  TheApp = this;
  signal(SIGUSR1, sigHandler);

  return true;
}
