// CPUGraphView.h : per-CPU usage history graphs
//
// Samples system-wide per-logical-CPU usage once a second (NtQuerySystemInformation) and draws
// one small area graph per CPU, laid out in a grid that adapts to the window size.

#pragma once

#include <vector>
#include <deque>

class CCPUGraphView : public CWindowImpl<CCPUGraphView> {
public:
	DECLARE_WND_CLASS_EX(L"CPUStressGraphView", CS_HREDRAW | CS_VREDRAW, -1)

	static constexpr int HistorySize = 60;		// samples (seconds) kept per CPU

	BEGIN_MSG_MAP(CCPUGraphView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
	END_MSG_MAP()

private:
	struct CpuTimes {
		ULONGLONG idle, busy;	// cumulative 100 ns units; busy = kernel + user - idle
	};

	LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnTimer(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL&) {
		return 1;	// everything is painted off screen
	}

	bool Sample();
	void DrawCell(CDCHandle dc, const CRect& rc, int cpu, bool dark);

	std::vector<CpuTimes> m_Last;
	std::vector<std::deque<int>> m_History;		// per CPU, percent 0-100, oldest first
};
