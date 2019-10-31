// View.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ThreadManager.h"
#include "VirtualListView.h"
#include "Thread.h"
#include <utility>

class CView : 
	public CWindowImpl<CView, CListViewCtrl>,
	public CVirtualListView<CView>,
	public CCustomDraw<CView>
{
public:
	DECLARE_WND_SUPERCLASS(nullptr, CListViewCtrl::GetWndClassName())

	CView(CUpdateUIBase& ui);

	void Refresh();
	BOOL PreTranslateMessage(MSG* pMsg);
	Thread& GetItem(int index);
	std::shared_ptr<Thread> GetFullItem(int index) const;

	void DoSort(const SortInfo* si);
	void SetThreadActivity(int activity);
	std::vector<std::shared_ptr<Thread>> GetSelectedThreads() const;

	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);

	BEGIN_MSG_MAP(CView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
		CHAIN_MSG_MAP_ALT(CVirtualListView, 1)
		CHAIN_MSG_MAP_ALT(CCustomDraw<CView>, 1)

	ALT_MSG_MAP(2)
		COMMAND_ID_HANDLER(ID_VIEW_REFRESH, OnRefresh)
		COMMAND_RANGE_HANDLER(ID_ACTIVITY_LOW, ID_ACTIVITY_MAXIMUM, OnThreadActivity)
		COMMAND_ID_HANDLER(ID_THREAD_CREATENEWTHREAD, OnNewThread)
		COMMAND_ID_HANDLER(ID_THREAD_RESUME, OnThreadResume)
		COMMAND_ID_HANDLER(ID_THREAD_SUSPEND, OnThreadSuspend)
		COMMAND_ID_HANDLER(ID_VIEW_SHOWALLTHREADS, OnShowAllThreads)
		COMMAND_ID_HANDLER(ID_THREAD_KILL, OnThreadKill)
	END_MSG_MAP()

private:
	static CString GetThreadType(Thread& t);
	static bool CompareItems(Thread& t1, Thread& t2, const SortInfo* si);
	static PCWSTR ActivityLevelToString(ActivityLevel level);
	static PCWSTR ThreadPriorityToString(int priority);
	static std::pair<COLORREF, COLORREF> ActivityLevelToColor(ActivityLevel level);
	void Redraw();
	void UpdateUI();

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnGetDispInfo(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnThreadActivity(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNewThread(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnThreadResume(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnThreadSuspend(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnThreadKill(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnShowAllThreads(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	ThreadManager m_ThreadMgr;
	std::vector<std::shared_ptr<Thread>> m_Threads;
	CUpdateUIBase& m_UI;
	bool m_ShowAllThreads : 1;
};
