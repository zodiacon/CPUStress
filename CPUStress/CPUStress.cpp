// CPUStress.cpp : main source file for CPUStress.exe
//

#include "pch.h"

#include "resource.h"

#include "MainFrm.h"
#include "WTLHelper.h"
#include "Settings.h"

// fixDarkScrollBar() is declared behind _DARKMODELIB_USE_SCROLLBAR_FIX in the library headers
// (which this TU doesn't define), so declare it here to route scroll bars through the dark theme.
namespace dmlib_hook { void fixDarkScrollBar(); }

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = nullptr, int nCmdShow = SW_SHOWDEFAULT) {
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame wndMain;

	if (wndMain.CreateEx() == nullptr) {
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
	HRESULT hRes = ::CoInitialize(nullptr);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	// install the dark mode hook before any window is created
	WTLHelper::InitDarkMode(Settings::DarkMode() ? DarkModeKind::Dark : DarkModeKind::Light);

	// route comctl32 scroll bars (e.g. the list view's) through the dark "Explorer::ScrollBar" theme.
	// one-time, process-global IAT patch: must run after InitDarkMode (which loads the original
	// OpenNcThemeData) and before any window is created.
	dmlib_hook::fixDarkScrollBar();

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
