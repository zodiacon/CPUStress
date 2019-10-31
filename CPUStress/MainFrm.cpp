// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "View.h"
#include "MainFrm.h"

CMainFrame::CMainFrame() : m_view(*this, this) {
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
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, nullptr, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	// attach menu
	m_CmdBar.AttachMenu(GetMenu());
	// remove old menu
	SetMenu(nullptr);

	CToolBarCtrl tb;
	tb.Create(m_hWnd, nullptr, nullptr, ATL_SIMPLE_TOOLBAR_PANE_STYLE, 0, ATL_IDW_TOOLBAR);
	CImageList tbImages, cbImages;
	tbImages.Create(32, 32, ILC_COLOR32 | ILC_COLOR, 8, 4);
	//m_CmdBar.SetImageList(cbImages.Detach());

	struct {
		UINT id;
		int image;
		int style = TBSTYLE_BUTTON;
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

	m_CmdBar.m_bAlphaImages = true;

	for (auto& b : buttons) {
		if (b.id == 0)
			tb.AddSeparator(0);
		else {
			int image = tbImages.AddIcon(AtlLoadIcon(b.image));
			tb.AddButton(b.id, b.style, TBSTATE_ENABLED, image, nullptr, 0);
		}
	}

	struct {
		UINT id, icon;
	} cmds[] = {
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
	for (auto& cmd : cmds)
		m_CmdBar.AddIcon(AtlLoadIcon(cmd.icon), cmd.id);

	HWND hWndToolBar = tb.Detach();

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, nullptr, TRUE);
	CReBarCtrl(m_hWndToolBar).LockBands(TRUE);

	CreateSimpleStatusBar();
	m_StatusBar.SubclassWindow(m_hWndStatusBar);
	int paneWidths[] = { 140, 250, 420, 620 };
	int panes[] = { 120, 121, 122, 123 };
	m_StatusBar.SetPanes(panes, _countof(panes));
	m_StatusBar.SetParts(_countof(panes), paneWidths);

	m_hWndClient = m_view.Create(m_hWnd, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA | LVS_REPORT,
		WS_EX_CLIENTEDGE);

	UIAddToolBar(hWndToolBar);
	UIAddStatusBar(m_hWndStatusBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	CString text;
	GetWindowText(text);
	text.Format(L"%s (PID: %d)", text, ::GetCurrentProcessId());
	SetWindowText(text);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	SetWindowPos(nullptr, 0, 0, 1200, 500, SWP_NOMOVE | SWP_NOREPOSITION);

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
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
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

LRESULT CMainFrame::OnAlwaysOnTop(WORD, WORD, HWND, BOOL&) {
	auto onTop = GetWindowLongPtr(GWL_EXSTYLE) & WS_EX_TOPMOST;
	SetWindowPos(onTop ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	UISetCheck(ID_OPTIONS_ALWAYSONTOP, !onTop);

	return 0;
}

BOOL CMainFrame::ShowContextMenu(HMENU hMenu, POINT pt) {
	m_CmdBar.TrackPopupMenu(hMenu, 0, pt.x, pt.y);

	return 0;
}

