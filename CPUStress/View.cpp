// View.cpp : implementation of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"
#include <algorithm>
#include "View.h"
#include "Thread.h"
#include "AffinityDlg.h"
#include "CPUSetsDlg.h"

CView::CView(CUpdateUIBase& ui, IMainFrame* pFrame) : m_UI(ui), m_pFrame(pFrame), m_ShowAllThreads(false) {
	ui.UISetCheck(ID_VIEW_SHOWALLTHREADS, FALSE);
}

void CView::Refresh() {
	m_ThreadMgr.EnumThreads();
	m_Threads = m_ShowAllThreads ? m_ThreadMgr.GetThreads() : m_ThreadMgr.GetUserThreads();
	auto psi = GetSortInfo(0);
	if (psi)
		DoSort(psi);
	else {
		SortInfo si;
		si.SortAscending = true;
		si.SortColumn = 0;
		DoSort(&si);
	}

	SetItemCountEx(static_cast<int>(m_Threads.size()), LVSICF_NOSCROLL);
	UpdateUI();
	CString text;
	text.Format(L"User Created Threads: %d", m_ThreadMgr.GetUserThreads().size());
	m_UI.UISetText(0, text);

	auto count = std::count_if(m_ThreadMgr.GetUserThreads().begin(), m_ThreadMgr.GetUserThreads().end(), [](auto& t) { return !t->IsSuspended(); });
	text.Format(L"Active Threads: %d", count);
	m_UI.UISetText(1, text);

	text.Format(L"Total Threads: %d", m_ThreadMgr.GetThreadCount());
	m_UI.UISetText(2, text);
	
	DWORD_PTR affinity, sysAffinity;
	::GetProcessAffinityMask(::GetCurrentProcess(), &affinity, &sysAffinity);
	text.Format(L"Process Affinity: 0x%llX", (DWORD64)affinity);
	m_UI.UISetText(3, text);

	text.Format(L"Process Priority: %s", PriorityClassToString(::GetPriorityClass(::GetCurrentProcess())));
	m_UI.UISetText(4, text);
}

BOOL CView::PreTranslateMessage(MSG* pMsg) {
	pMsg;
	return FALSE;
}

Thread& CView::GetItem(int index) {
	return *m_Threads[index];
}

std::shared_ptr<Thread> CView::GetFullItem(int index) const {
	return m_Threads[index];
}

void CView::DoSort(const SortInfo* si) {
	std::stable_sort(m_Threads.begin(), m_Threads.end(), [si](auto& t1, auto& t2) {
		return CompareItems(*t1, *t2, si);
		});
}

void CView::SetThreadActivity(int activity) {
	for (auto& t : GetSelectedThreads())
		t->SetActivityLevel((ActivityLevel)(activity + 1));
	RedrawItems(GetTopIndex(), GetTopIndex() + GetCountPerPage());
}

std::vector<std::shared_ptr<Thread>> CView::GetSelectedThreads() const {
	std::vector<std::shared_ptr<Thread>> threads;
	int count = GetSelectedCount();
	threads.reserve(count);
	int item = -1;
	for (int i = 0; i < count; i++) {
		item = GetNextItem(item, LVNI_SELECTED);
		auto t = GetFullItem(item);
		if (t->IsUserCreated())
			threads.push_back(t);
	}
	return threads;
}

DWORD CView::OnPrePaint(int, LPNMCUSTOMDRAW) {
	return CDRF_NOTIFYITEMDRAW;
}

DWORD CView::OnItemPrePaint(int, LPNMCUSTOMDRAW) {
	return CDRF_NOTIFYSUBITEMDRAW;
}
DWORD CView::OnSubItemPrePaint(int, LPNMCUSTOMDRAW cd) {
	auto lvcd = (NMLVCUSTOMDRAW*)cd;
	int index = (int)cd->dwItemSpec;
	auto& t = GetItem(index);
	auto col = lvcd->iSubItem;
	lvcd->clrTextBk = lvcd->clrText = CLR_INVALID;
	if (!t.IsUserCreated()) {
		lvcd->clrText = RGB(128, 128, 128);
		return CDRF_SKIPPOSTPAINT;
	}
	switch (col) {
		case 4:
			if (!t.IsSuspended()) {
				auto colors = ActivityLevelToColor(t.GetActivityLevel());
				lvcd->clrTextBk = colors.first;
				lvcd->clrText = colors.second;
			}
			break;
	}

	return CDRF_SKIPPOSTPAINT;
}

CString CView::GetThreadType(Thread& t) {
	if (t.GetId() == ::GetCurrentThreadId())
		return L"GUI";

	return t.IsUserCreated() ? L"User Created" : L"(Unknown)";
}

bool CView::CompareItems(Thread& t1, Thread& t2, const SortInfo* si) {
	switch (si->SortColumn) {
		case 0:
			return SortNumbers(t1.GetIndex(), t2.GetIndex(), si->SortAscending);

		case 1:	// CPU
			return SortNumbers(t1.GetCPU(), t2.GetCPU(), si->SortAscending);

		case 2:
			return SortNumbers(t1.GetId(), t2.GetId(), si->SortAscending);

		case 3:
			return SortStrings(GetThreadType(t1), GetThreadType(t2), si->SortAscending);

		case 4:	// activity
			return SortNumbers(t1.GetActivityLevel(), t2.GetActivityLevel(), si->SortAscending);

		case 5:	// base priority
			return SortNumbers(t1.GetBasePriority(), t2.GetBasePriority(), si->SortAscending);

		case 6:	// base priority
			return SortNumbers(t1.GetPriority(), t2.GetPriority(), si->SortAscending);

		case 7:	// ideal CPU
			return SortNumbers(t1.GetIdealCPU(), t2.GetIdealCPU(), si->SortAscending);

		case 8:	// affinity
			return SortNumbers(t1.GetAffinity(), t2.GetAffinity(), si->SortAscending);

		case 9:	// created
			return SortNumbers(t1.GetCreateTime(), t2.GetCreateTime(), si->SortAscending);

		case 10: // CPU time
			return SortNumbers(t1.GetCPUTime().GetTimeSpan(), t2.GetCPUTime().GetTimeSpan(), si->SortAscending);

		case 11:	// TEB
			return SortNumbers(t1.GetTeb(), t2.GetTeb(), si->SortAscending);

		case 12:	// stack base, limit
		case 13:
			PVOID start1, end1, start2, end2;
			t1.GetStackLimits(start1, end1);
			t2.GetStackLimits(start2, end2);
			return SortNumbers(si->SortColumn == 12 ? start1 : end1, si->SortColumn == 12 ? start2 : end2, si->SortAscending);

	}
	return false;
}

PCWSTR CView::ActivityLevelToString(ActivityLevel level) {
	switch (level) {
		case ActivityLevel::Busy: return L"Busy";
		case ActivityLevel::Low: return L"Low";
		case ActivityLevel::Medium: return L"Medium";
		case ActivityLevel::Maximum: return L"Maximum";
	}
	ATLASSERT(false);
	return L"";
}

PCWSTR CView::ThreadPriorityToString(int priority) {
	switch (priority) {
		case THREAD_PRIORITY_IDLE: return L"Idle (-SAT)";
		case THREAD_PRIORITY_LOWEST: return L"Lowest (-2)";
		case THREAD_PRIORITY_NORMAL: return L"Normal";
		case THREAD_PRIORITY_ABOVE_NORMAL: return L"Above Normal (+1)";
		case THREAD_PRIORITY_BELOW_NORMAL: return L"Below Normal (-1)";
		case THREAD_PRIORITY_HIGHEST: return L"Highest (+2)";
		case THREAD_PRIORITY_TIME_CRITICAL: return L"Time Critical (+SAT)";
	}
	ATLASSERT(false);
	return L"";
}

std::pair<COLORREF, COLORREF> CView::ActivityLevelToColor(ActivityLevel level) {
	switch (level) {
		case ActivityLevel::Low: return { RGB(192, 192, 0), RGB(0, 0, 0) };
		case ActivityLevel::Medium: return { RGB(128, 128, 0), RGB(0, 0, 0) };
		case ActivityLevel::Busy: return { RGB(192, 0, 0), RGB(255, 255, 255) };
		case ActivityLevel::Maximum: return { RGB(128, 0, 0), RGB(255, 255, 255) };
	}
	return { CLR_INVALID, CLR_INVALID };
}

WORD CView::PriorityClassToId(int priority) {
	switch (priority) {
		case IDLE_PRIORITY_CLASS: return ID_PRIORITYCLASS_IDLE;
		case BELOW_NORMAL_PRIORITY_CLASS: return ID_PRIORITYCLASS_BELOWNORMAL;
		case NORMAL_PRIORITY_CLASS: return ID_PRIORITYCLASS_NORMAL;
		case ABOVE_NORMAL_PRIORITY_CLASS: return ID_PRIORITYCLASS_ABOVENORMAL;
		case HIGH_PRIORITY_CLASS: return ID_PRIORITYCLASS_HIGH;
		case REALTIME_PRIORITY_CLASS: return ID_PRIORITYCLASS_REALTIME;
	}
	ATLASSERT(false);
	return ID_PRIORITYCLASS_NORMAL;
}

PCWSTR CView::PriorityClassToString(int pc) {
	switch (pc) {
		case IDLE_PRIORITY_CLASS: return L"Idle (4)";
		case BELOW_NORMAL_PRIORITY_CLASS: return L"Below Normal (6)";
		case NORMAL_PRIORITY_CLASS: return L"Normal (8)";
		case ABOVE_NORMAL_PRIORITY_CLASS: return L"Above Normal (10)";
		case HIGH_PRIORITY_CLASS: return L"High (13)";
		case REALTIME_PRIORITY_CLASS: return L"Realtime (24)";
	}
	ATLASSERT(false);
	return L"";
}

void CView::Redraw() {
	RedrawItems(GetTopIndex(), GetTopIndex() + GetCountPerPage());
}

void CView::UpdateUI() {
	auto selected = GetSelectedCount();
	auto threads = GetSelectedThreads();
	auto userThreads = !threads.empty();
	m_UI.UIEnable(ID_ACTIVITY_BUSY, userThreads);
	m_UI.UIEnable(ID_ACTIVITY_LOW, !threads.empty());
	m_UI.UIEnable(ID_ACTIVITY_MEDIUM, !threads.empty());
	m_UI.UIEnable(ID_ACTIVITY_MAXIMUM, !threads.empty());
	m_UI.UIEnable(ID_THREAD_RESUME, !threads.empty());
	m_UI.UIEnable(ID_THREAD_KILL, !threads.empty());
	m_UI.UIEnable(ID_THREAD_SUSPEND, !threads.empty());
	m_UI.UIEnable(ID_THREAD_AFFINITY, threads.size() == 1);
	m_UI.UIEnable(ID_THREAD_IDEALCPU, threads.size() == 1);
	m_UI.UIEnable(ID_THREAD_SELECTEDCPUSETS, threads.size() == 1);

	m_UI.UIEnable(ID_PRIORITY_IDLE, threads.size() == 1);
	m_UI.UIEnable(ID_PRIORITY_LOWEST, threads.size() == 1);
	m_UI.UIEnable(ID_PRIORITY_BELOWNORMAL, threads.size() == 1);
	m_UI.UIEnable(ID_PRIORITY_NORMAL, threads.size() == 1);
	m_UI.UIEnable(ID_PRIORITY_ABOVENORMAL, threads.size() == 1);
	m_UI.UIEnable(ID_PRIORITY_HIGHEST, threads.size() == 1);
	m_UI.UIEnable(ID_PRIORITY_TIMECRITICAL, threads.size() == 1);
}

LRESULT CView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	DefWindowProc();

	m_UI.UISetRadioMenuItem(PriorityClassToId(::GetPriorityClass(::GetCurrentProcess())), ID_PRIORITYCLASS_IDLE, ID_PRIORITYCLASS_REALTIME);

	SetExtendedListViewStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	struct {
		PCWSTR text;
		int width;
		int format = LVCFMT_LEFT;
	} columns[] = {
		{ L"Index", 50 },
		{ L"CPU %", 50, LVCFMT_CENTER },
		{ L"ID", 100, LVCFMT_CENTER },
		{ L"Type", 80 },
		{ L"Activity", 70 },
		{ L"Base Priority", 120 },
		{ L"Priority", 50, LVCFMT_CENTER },
		{ L"Ideal CPU", 70, LVCFMT_CENTER },
		{ L"Affinity", 10 + 4 * Thread::GetCPUCount(), LVCFMT_RIGHT },
		{ L"Created", 80 },
		{ L"CPU Time", 90 },
		//{ L"TEB", 130, LVCFMT_RIGHT },
		//{ L"Stack Base", 130, LVCFMT_RIGHT },
		//{ L"Stack Limit", 130, LVCFMT_RIGHT },
	};

	CImageList images;
	images.Create(24, 24, ILC_COLOR32, 4, 4);
	UINT icons[] = { IDI_THREAD, IDI_THREAD_RUN, IDI_THREAD_PAUSE };
	for(auto icon : icons)
		images.AddIcon(AtlLoadIcon(icon));
	SetImageList(images.Detach(), LVSIL_SMALL);

	int i = 0;
	for (auto& c : columns)
		InsertColumn(i++, c.text, c.format, c.width);

	for (int i = 0; i < 4; i++) {
		auto t = m_ThreadMgr.AddNewThread();
		if(i == 0)
			t->Resume();
	}

	Refresh();

	SetTimer(1, 1000, nullptr);

	return 0;
}

LRESULT CView::OnContextMenu(UINT, WPARAM, LPARAM, BOOL&) {
	auto threads = GetSelectedThreads();
	CMenuHandle hMenu;
	hMenu.LoadMenuW(IDR_CONTEXT);
	POINT pt;
	::GetCursorPos(&pt);
	m_pFrame->ShowContextMenu(hMenu.GetSubMenu(threads.empty() && GetSelectedCount() == 0 ? 1 : 0), pt);

	return 0;
}

LRESULT CView::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id == 1)
		Refresh();
	return 0;
}

LRESULT CView::OnGetDispInfo(int, LPNMHDR hdr, BOOL&) {
	auto di = reinterpret_cast<NMLVDISPINFO*>(hdr);
	auto& item = di->item;
	auto index = item.iItem;
	auto col = item.iSubItem;
	auto& data = GetItem(index);

	if (di->item.mask & LVIF_TEXT) {
		switch (col) {
			case 0:	// index
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%3d", data.GetIndex());
				break;

			case 1:	// cpu
				if (!data.IsSuspended())
					::StringCchPrintf(item.pszText, item.cchTextMax, L"%.2f", data.GetCPU() / 100.0);
				break;

			case 2:	// ID
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%d (0x%X)", data.GetId(), data.GetId());
				break;

			case 3:	// type
				::StringCchCopy(item.pszText, item.cchTextMax, GetThreadType(data));
				break;

			case 4:	// activity
				if(data.IsUserCreated())
					item.pszText = (PWSTR)ActivityLevelToString(data.GetActivityLevel());
				break;

			case 5:	// base priority
				item.pszText = (PWSTR)ThreadPriorityToString(data.GetBasePriority());
				break;

			case 6:	// priority
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data.GetPriority());
				break;

			case 7:	// ideal CPU
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data.GetIdealCPU());
				break;

			case 8:	// affinity
				::StringCchPrintf(item.pszText, item.cchTextMax, L"0x%llX", (DWORD64)data.GetAffinity());
				break;

			case 9:	// created
				::StringCchCopy(item.pszText, item.cchTextMax, data.GetCreateTime().Format(L"%X"));
				break;

			case 10:	// CPU time
				::StringCchCopy(item.pszText, item.cchTextMax, CTimeSpan(data.GetCPUTime().GetTimeSpan() / 10000000LL).Format(L"%H:%M:%S"));
				break;

			case 11:	// TEB
				::StringCchPrintf(item.pszText, item.cchTextMax, L"0x%p", data.GetTeb());
				break;

			case 12: // stack base
			case 13: // stack limit
			{
				void* start;
				void* end;
				data.GetStackLimits(start, end);
				::StringCchPrintf(item.pszText, item.cchTextMax, L"0x%p", col == 12 ? start : end);
				break;
			}
		}
	}
	if (di->item.mask & LVIF_IMAGE) {
		int image = 0;
		if (data.IsUserCreated()) {
			image = data.IsSuspended() ? 2 : 1;
		}
		item.iImage = image;
	}
	return 0;
}

LRESULT CView::OnRefresh(WORD, WORD, HWND, BOOL&) {
	Refresh();
	return 0;
}

LRESULT CView::OnItemChanged(int, LPNMHDR hdr, BOOL&) {
	UpdateUI();
	return 0;
}

LRESULT CView::OnThreadActivity(WORD, WORD id, HWND, BOOL&) {
	SetThreadActivity(id - ID_ACTIVITY_LOW);

	return 0;
}

LRESULT CView::OnNewThread(WORD, WORD, HWND, BOOL&) {
	auto t = m_ThreadMgr.AddNewThread();
	t->Suspend();

	return 0;
}

LRESULT CView::OnThreadResume(WORD, WORD, HWND, BOOL&) {
	for (auto& t : GetSelectedThreads())
		t->Resume();
	Redraw();
	return 0;
}

LRESULT CView::OnThreadSuspend(WORD, WORD, HWND, BOOL&) {
	for (auto& t : GetSelectedThreads())
		t->Suspend();
	Redraw();
	return 0;
}

LRESULT CView::OnThreadKill(WORD, WORD, HWND, BOOL&) {
	auto threads = GetSelectedThreads();
	auto count = static_cast<int>(threads.size());
	ATLASSERT(count > 0);

	CString text;
	if (count == 1)
		text = L"Kill thread?";
	else
		text.Format(L"Kill %d threads?", count);
	if (MessageBox(text, L"CPUStress", MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONWARNING) == IDCANCEL)
		return 0;

	for (auto& t : threads) {
		m_ThreadMgr.RemoveThread(t->GetId());
	}
	return 0;
}

LRESULT CView::OnShowAllThreads(WORD, WORD, HWND, BOOL&) {
	m_ShowAllThreads = !m_ShowAllThreads;
	m_UI.UISetCheck(ID_VIEW_SHOWALLTHREADS, m_ShowAllThreads);

	return 0;
}

LRESULT CView::OnSelectAll(WORD, WORD, HWND, BOOL&) {
	SelectAllItems(true);
	return 0;
}

LRESULT CView::OnSelectNone(WORD, WORD, HWND, BOOL&) {
	SelectAllItems(false);
	return 0;
}

LRESULT CView::OnSelectInvert(WORD, WORD, HWND, BOOL&) {
	for (int i = 0; i < GetItemCount(); i++)
		SetItemState(i, GetItemState(i, LVIS_SELECTED) == 0 ? LVIS_SELECTED : 0, LVIS_SELECTED);

	return 0;
}

LRESULT CView::OnSetProcessPriority(WORD, WORD id, HWND, BOOL&) {
	const DWORD pc[] = {
		IDLE_PRIORITY_CLASS,
		BELOW_NORMAL_PRIORITY_CLASS,
		NORMAL_PRIORITY_CLASS,
		ABOVE_NORMAL_PRIORITY_CLASS,
		HIGH_PRIORITY_CLASS,
		REALTIME_PRIORITY_CLASS
	};
	ATLASSERT(id - ID_PRIORITYCLASS_IDLE < _countof(pc) && id >= ID_PRIORITYCLASS_IDLE);

	::SetPriorityClass(::GetCurrentProcess(), pc[id - ID_PRIORITYCLASS_IDLE]);
	m_UI.UISetRadioMenuItem(id, ID_PRIORITYCLASS_IDLE, ID_PRIORITYCLASS_REALTIME);

	return 0;
}

LRESULT CView::OnSetThreadPriority(WORD, WORD id, HWND, BOOL&) {
	const int tp[] = {
		THREAD_PRIORITY_IDLE,
		THREAD_PRIORITY_LOWEST,
		THREAD_PRIORITY_BELOW_NORMAL,
		THREAD_PRIORITY_NORMAL,
		THREAD_PRIORITY_ABOVE_NORMAL,
		THREAD_PRIORITY_HIGHEST,
		THREAD_PRIORITY_TIME_CRITICAL
	};
	ATLASSERT(id - ID_PRIORITY_IDLE < _countof(tp));
	
	for (auto& t : GetSelectedThreads())
		t->SetBasePriority(tp[id - ID_PRIORITY_IDLE]);
	Redraw();

	return 0;
}

LRESULT CView::OnProcessAffinity(WORD, WORD, HWND, BOOL&) {
	CAffinityDlg dlg(L"Process Affinity");
	if (IDOK == dlg.DoModal()) {
		::SetProcessAffinityMask(::GetCurrentProcess(), dlg.GetSelectedAffinity());
		Redraw();
	}
	return 0;
}

LRESULT CView::OnThreadAffinity(WORD, WORD, HWND, BOOL&) {
	auto t = GetSelectedThreads()[0];
	CString title;
	title.Format(L"Thread %d (TID: %d) Affinity", t->GetIndex(), t->GetId());
	CAffinityDlg dlg(true, t.get(), title);
	if (IDOK == dlg.DoModal()) {
		t->SetAffinity(dlg.GetSelectedAffinity());
		Redraw();
	}
	return 0;
}

LRESULT CView::OnThreadIdealCPU(WORD, WORD, HWND, BOOL&) {
	auto t = GetSelectedThreads()[0];
	CString title;
	title.Format(L"Thread %d (TID: %d) Ideal CPU", t->GetIndex(), t->GetId());
	CAffinityDlg dlg(false, t.get(), title);
	if (IDOK == dlg.DoModal()) {
		t->SetIdealCPU(dlg.GetSelectedCPU());
		Redraw();
	}
	return 0;
}

LRESULT CView::OnThreadSelectedCPUset(WORD, WORD, HWND, BOOL&) {
	auto thread = GetSelectedThreads()[0];

	CCPUSetsDlg dlg(CPUSetsType::Thread, thread.get());
	if (dlg.DoModal() == IDOK) {
		ULONG count;
		auto set = dlg.GetCpuSet(count);
		thread->SetCPUSet(set, count);
	}
	return LRESULT();
}
