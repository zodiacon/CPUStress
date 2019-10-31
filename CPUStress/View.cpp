// View.cpp : implementation of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include <algorithm>
#include "View.h"
#include "Thread.h"

CView::CView(CUpdateUIBase& ui) : m_UI(ui), m_ShowAllThreads(false) {
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

		case 5:	// priority
			return SortNumbers(t1.GetPriority(), t2.GetPriority(), si->SortAscending);

		case 6:	// ideal CPU
			return SortNumbers(t1.GetIdealCPU(), t2.GetIdealCPU(), si->SortAscending);

		case 7:	// affinity
			return SortNumbers(t1.GetAffinity(), t2.GetAffinity(), si->SortAscending);

		case 8:	// created
			return SortNumbers(t1.GetCreateTime(), t2.GetCreateTime(), si->SortAscending);

		case 9:	// TEB
			return SortNumbers(t1.GetTeb(), t2.GetTeb(), si->SortAscending);

		case 10:	// stack base, limit
		case 11:
			PVOID start1, end1, start2, end2;
			t1.GetStackLimits(start1, end1);
			t2.GetStackLimits(start2, end2);
			return SortNumbers(si->SortColumn == 10 ? start1 : end1, si->SortColumn == 10 ? start2 : end2, si->SortAscending);

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

void CView::Redraw() {
	RedrawItems(GetTopIndex(), GetTopIndex() + GetCountPerPage());
}

void CView::UpdateUI() {
	auto selected = GetSelectedCount();
	auto threads = GetSelectedThreads();
	m_UI.UIEnable(ID_ACTIVITY_BUSY, !threads.empty());
	m_UI.UIEnable(ID_ACTIVITY_LOW, !threads.empty());
	m_UI.UIEnable(ID_ACTIVITY_MEDIUM, !threads.empty());
	m_UI.UIEnable(ID_ACTIVITY_MAXIMUM, !threads.empty());
	m_UI.UIEnable(ID_THREAD_RESUME, !threads.empty());
	m_UI.UIEnable(ID_THREAD_KILL, !threads.empty());
	m_UI.UIEnable(ID_THREAD_SUSPEND, !threads.empty());
}

LRESULT CView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	DefWindowProc();

	SetExtendedListViewStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	struct {
		PCWSTR text;
		int width;
		int format = LVCFMT_LEFT;
	} columns[] = {
		{ L"Index", 70 },
		{ L"CPU %", 70, LVCFMT_RIGHT },
		{ L"ID", 100, LVCFMT_RIGHT },
		{ L"Type", 80 },
		{ L"Activity", 80 },
		{ L"Priority", 100 },
		{ L"Ideal CPU", 90, LVCFMT_RIGHT },
		{ L"Affinity", 120, LVCFMT_RIGHT },
		{ L"Created", 80 },
		{ L"TEB", 130, LVCFMT_RIGHT },
		{ L"Stack Base", 130, LVCFMT_RIGHT },
		{ L"Stack Limit", 130, LVCFMT_RIGHT },
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
	UpdateUI();

	SetTimer(1, 1000, nullptr);

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

			case 5:	// priority
				item.pszText = (PWSTR)ThreadPriorityToString(data.GetPriority());
				break;

			case 6:	// ideal CPU
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data.GetIdealCPU());
				break;

			case 7:	// affinity
				::StringCchPrintf(item.pszText, item.cchTextMax, L"0x%p", data.GetAffinity());
				break;

			case 8:	// created
				::StringCchCopy(item.pszText, item.cchTextMax, data.GetCreateTime().Format(L"%X"));
				break;

			case 9:	// TEB
				::StringCchPrintf(item.pszText, item.cchTextMax, L"0x%p", data.GetTeb());
				break;

			case 10: // stack base
			case 11: // stack limit
			{
				void* start;
				void* end;
				data.GetStackLimits(start, end);
				::StringCchPrintf(item.pszText, item.cchTextMax, L"0x%p", col == 10 ? start : end);
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
	for (auto& t : GetSelectedThreads()) {
		m_ThreadMgr.RemoveThread(t->GetId());
	}
	return 0;
}

LRESULT CView::OnShowAllThreads(WORD, WORD, HWND, BOOL&) {
	m_ShowAllThreads = !m_ShowAllThreads;
	m_UI.UISetCheck(ID_VIEW_SHOWALLTHREADS, m_ShowAllThreads);

	return 0;
}
