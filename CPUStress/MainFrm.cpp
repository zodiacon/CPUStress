// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"
#include "AboutDlg.h"
#include "View.h"
#include "MainFrm.h"
#include "CPUSetsDlg.h"
#include "SysInfoDlg.h"
#include "WTLHelper.h"
#include "DarkMode/DarkModeSubclass.h"
#include "Settings.h"

CMainFrame::CMainFrame() : m_view(*this, this) {
}

bool CMainFrame::IsCPUSetsAvailable() const {
	if (!IsWindows10OrGreater()) {
		AtlMessageBox(*this, L"CPU Sets are supported on Windows 10 and later versions", L"CPU Stress", MB_ICONEXCLAMATION);
		return false;
	}
	return true;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	return m_view.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle() {
	UIUpdateToolBar();
	UIUpdateStatusBar();
	return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// add icons to the (standard) menu, themed by WTLHelper
	InitMenu(GetMenu());

	CToolBarCtrl tb;
	tb.Create(m_hWnd, nullptr, nullptr, ATL_SIMPLE_TOOLBAR_PANE_STYLE, 0, ATL_IDW_TOOLBAR);
	CImageList tbImages;
	tbImages.Create(32, 32, ILC_COLOR32 | ILC_COLOR, 8, 4);

	struct {
		UINT id;
		int image;
		int style = BTNS_BUTTON;
	} buttons[] = {
		{ ID_THREAD_CREATENEWTHREAD, IDI_THREAD_ADD },
		{ 0 },
		{ ID_THREAD_SUSPEND, IDI_THREAD_PAUSE },
		{ ID_THREAD_RESUME, IDI_THREAD_RUN },
		{ 0 },
		{ ID_THREAD_KILL, IDI_THREAD_DELETE },
		{ 0 },
		{ ID_ACTIVITY_LOW, IDI_ACTIVITY_LOW },
		{ ID_ACTIVITY_MEDIUM, IDI_ACTIVITY_MEDIUM },
		{ ID_ACTIVITY_BUSY, IDI_ACTIVITY_BUSY },
		{ ID_ACTIVITY_MAXIMUM, IDI_ACTIVITY_MAX },
		{ 0 },
		{ ID_VIEW_SHOWALLTHREADS, IDI_THREADS },
	};

	tb.SetImageList(tbImages);

	for (auto& b : buttons) {
		if (b.id == 0)
			tb.AddSeparator(0);
		else {
			int image = tbImages.AddIcon(AtlLoadIcon(b.image));
			tb.AddButton(b.id, b.style, TBSTATE_ENABLED, image, nullptr, 0);
		}
	}

	HWND hWndToolBar = tb.Detach();

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndToolBar, nullptr, TRUE);
	CReBarCtrl(m_hWndToolBar).LockBands(TRUE);

	CreateSimpleStatusBar();
	m_StatusBar.SubclassWindow(m_hWndStatusBar);
	int paneWidths[] = { 150, 300, 430, 530, 980 };
	m_StatusBar.SetParts(_countof(paneWidths), paneWidths);

	m_hWndClient = m_view.Create(m_hWnd, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA | LVS_REPORT,
		WS_EX_CLIENTEDGE);

	UIAddMenu(GetMenu());
	UIAddToolBar(hWndToolBar);
	UIAddStatusBar(m_hWndStatusBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	bool dark = Settings::DarkMode();
	UISetCheck(ID_OPTIONS_DARKMODE, dark);
	SetDarkMode(dark);

	auto lf = Settings::Font();
	if (lf.lfHeight) {
		m_Font.CreateFontIndirect(&lf);
		m_view.SetFont(m_Font);
	}

	CString text;
	GetWindowText(text);
	text.Format(L"%s (PID: %u)", (PCWSTR)text, ::GetCurrentProcessId());
	SetWindowText(text);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	SetWindowPos(nullptr, 0, 0, 890, 430, SWP_NOMOVE | SWP_NOREPOSITION);

	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST);	// toolbar is the only band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnSystemCPUSets(WORD, WORD, HWND, BOOL&) {
	if (!IsCPUSetsAvailable())
		return 0;

	CCPUSetsDlg dlg(CPUSetsType::System);
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnProcessCPUSets(WORD, WORD, HWND, BOOL&) {
	if (!IsCPUSetsAvailable())
		return 0;

	CCPUSetsDlg dlg(CPUSetsType::Process);
	if (dlg.DoModal() == IDOK) {
		ULONG count;
		auto sets = dlg.GetCpuSet(count);
		ATLASSERT(sets);
		if (!::SetProcessDefaultCpuSets(::GetCurrentProcess(), count ? sets : nullptr, count))
			MessageBox(L"Failed to set process CPU set", L"CPU Stress", MB_ICONERROR);
	}
	return 0;
}

LRESULT CMainFrame::OnAlwaysOnTop(WORD, WORD, HWND, BOOL&) {
	auto onTop = GetWindowLongPtr(GWL_EXSTYLE) & WS_EX_TOPMOST;
	SetWindowPos(onTop ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	UISetCheck(ID_OPTIONS_ALWAYSONTOP, !onTop);

	return 0;
}

LRESULT CMainFrame::OnSystemInfo(WORD, WORD, HWND, BOOL&) {
	CSysInfoDlg().DoModal();

	return 0;
}

LRESULT CMainFrame::OnLaunchCPUStress(WORD, WORD, HWND, BOOL&) {
	PROCESS_INFORMATION pi;
	STARTUPINFO si = { sizeof(si) };
	WCHAR path[MAX_PATH];
	::GetModuleFileName(nullptr, path, _countof(path));
	if (::CreateProcess(path, nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);
	}
	else {
		AtlMessageBox(m_hWnd, L"Error launching process.", IDR_MAINFRAME, MB_ICONERROR);
	}
	return 0;
}

LRESULT CMainFrame::OnDarkMode(WORD, WORD id, HWND, BOOL&) {
	bool dark = !WTLHelper::IsDarkMode();
	Settings::DarkMode(dark);
	SetDarkMode(dark);
	UISetCheck(id, dark);
	return 0;
}

LRESULT CMainFrame::OnOptionsFont(WORD, WORD, HWND, BOOL&) {
	LOGFONT lf{};
	HFONT hFont = m_view.GetFont();
	if (hFont)
		CFontHandle(hFont).GetLogFont(lf);
	else {
		// no custom font yet: seed the dialog with the default UI font the list view renders with
		NONCLIENTMETRICS ncm{ sizeof(ncm) };
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
		lf = ncm.lfMessageFont;
	}

	// suspend the dark mode hook: it interferes with the common font dialog
//	WTLHelper::SuspendHook();
	CFontDialog dlg(&lf);
	if (dlg.DoModal() == IDOK) {
		dlg.GetCurrentFont(&lf);
		if (m_Font)
			m_Font.DeleteObject();
		m_Font.CreateFontIndirect(&lf);
		m_view.SetFont(m_Font);
		m_view.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
		Settings::Font(lf);
	}
	//WTLHelper::ResumeHook();

	return 0;
}

LRESULT CMainFrame::OnSettingChange(UINT, WPARAM, LPARAM lParam, BOOL& bHandled) {
	// let the dark mode library react to system theme changes
	DarkMode::handleSettingChange(lParam);
	bHandled = FALSE;
	return 0;
}

void CMainFrame::SetDarkMode(bool dark) {
	WTLHelper::SwitchToMode(dark ? DarkModeKind::Dark : DarkModeKind::Light, m_hWnd);

	m_view.SetBkColor(::GetSysColor(COLOR_WINDOW));
	// regenerate menu icon bitmaps so their background matches the new theme
	InitMenu(GetMenu());
	DrawMenuBar();

	::EnumThreadWindows(::GetCurrentThreadId(), [](auto h, auto) {
		::RedrawWindow(h, nullptr, nullptr,
			RDW_ERASENOW | RDW_INTERNALPAINT | RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
		return TRUE;
		}, 0);
}

void CMainFrame::InitMenu(HMENU hMenu) {
	MenuItemData cmds[] = {
		{ ID_THREAD_CREATENEWTHREAD, IDI_THREAD_ADD },
		{ ID_THREAD_SUSPEND, IDI_THREAD_PAUSE },
		{ ID_THREAD_RESUME, IDI_THREAD_RUN },
		{ ID_THREAD_KILL, IDI_THREAD_DELETE },
		{ ID_VIEW_REFRESH, IDI_REFRESH },
		{ ID_EDIT_COPY, IDI_COPY },
		{ ID_ACTIVITY_LOW, IDI_ACTIVITY_LOW },
		{ ID_ACTIVITY_MEDIUM, IDI_ACTIVITY_MEDIUM },
		{ ID_ACTIVITY_BUSY, IDI_ACTIVITY_BUSY },
		{ ID_ACTIVITY_MAXIMUM, IDI_ACTIVITY_MAX },
		{ ID_VIEW_SHOWALLTHREADS, IDI_THREADS },
		{ ID_EDIT_SELECT_ALL, IDI_SELECTALL },
		{ ID_EDIT_SELECTNONE, IDI_SELECT_NONE },
		{ ID_EDIT_INVERTSELECTION, IDI_SELECT_INVERT },
	};
	WTLHelper::InitMenu(hMenu, cmds, _countof(cmds));
}

bool CMainFrame::ShowContextMenu(HMENU hMenu, POINT pt) {
	InitMenu(hMenu);
	return ::TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, nullptr);
}

bool CMainFrame::SetStatusText(int pane, PCWSTR text) {
	return m_StatusBar.SetText(pane, text);
}

