#include "pch.h"
#include "CPUGraphView.h"
#include "NtDll.h"
#include "Thread.h"
#include "WTLHelper.h"
#include <algorithm>
#include <cmath>

static constexpr UINT_PTR SampleTimer = 1;
static constexpr int SampleIntervalMs = 1000;

LRESULT CCPUGraphView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	int count = Thread::GetCPUCount();
	m_History.assign(count, {});
	Sample();		// establishes the baseline; the first graph point comes one interval later
	SetTimer(SampleTimer, SampleIntervalMs);
	return 0;
}

LRESULT CCPUGraphView::OnDestroy(UINT, WPARAM, LPARAM, BOOL& handled) {
	KillTimer(SampleTimer);
	handled = FALSE;
	return 0;
}

bool CCPUGraphView::Sample() {
	std::vector<NT::SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> info(m_History.size());
	ULONG size = (ULONG)(info.size() * sizeof(info[0]));
	if (NT::NtQuerySystemInformation(NT::SystemProcessorPerformanceInformation, info.data(), size, nullptr) < 0)
		return false;

	std::vector<CpuTimes> now(info.size());
	for (size_t i = 0; i < info.size(); i++) {
		now[i].idle = info[i].IdleTime.QuadPart;
		now[i].busy = info[i].KernelTime.QuadPart + info[i].UserTime.QuadPart - now[i].idle;
	}

	if (m_Last.size() == now.size()) {
		for (size_t i = 0; i < now.size(); i++) {
			auto idle = now[i].idle - m_Last[i].idle;
			auto busy = now[i].busy - m_Last[i].busy;
			int percent = idle + busy == 0 ? 0 : (int)(busy * 100 / (idle + busy));
			auto& h = m_History[i];
			h.push_back(std::clamp(percent, 0, 100));
			if (h.size() > HistorySize)
				h.pop_front();
		}
	}
	m_Last = std::move(now);
	return true;
}

LRESULT CCPUGraphView::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id == SampleTimer && Sample())
		Invalidate(FALSE);
	return 0;
}

void CCPUGraphView::DrawCell(CDCHandle dc, const CRect& rc, int cpu, bool dark) {
	const COLORREF back = dark ? RGB(24, 24, 24) : RGB(255, 255, 255);
	const COLORREF grid = dark ? RGB(56, 56, 56) : RGB(214, 214, 214);
	const COLORREF text = dark ? RGB(220, 220, 220) : RGB(40, 40, 40);

	dc.FillSolidRect(&rc, back);

	CRect plot(rc);
	plot.DeflateRect(1, 1);

	// horizontal grid lines at 25/50/75%
	CPen gridPen;
	gridPen.CreatePen(PS_SOLID, 1, grid);
	auto oldPen = dc.SelectPen(gridPen);
	for (int pct = 25; pct < 100; pct += 25) {
		int y = plot.bottom - plot.Height() * pct / 100;
		dc.MoveTo(plot.left, y);
		dc.LineTo(plot.right, y);
	}

	// the history, newest sample on the right edge
	auto& h = m_History[cpu];
	if (!h.empty() && plot.Width() > 1 && plot.Height() > 1) {
		std::vector<POINT> pts;
		pts.reserve(h.size() + 2);
		double step = (double)plot.Width() / (HistorySize - 1);
		int first = HistorySize - (int)h.size();
		pts.push_back({ plot.left + (int)std::lround(first * step), plot.bottom });
		for (size_t i = 0; i < h.size(); i++)
			pts.push_back({ plot.left + (int)std::lround((first + (int)i) * step), plot.bottom - (plot.Height() - 1) * h[i] / 100 });
		pts.push_back({ pts.back().x, plot.bottom });

		const COLORREF fill = dark ? RGB(26, 78, 40) : RGB(176, 224, 190);
		const COLORREF line = dark ? RGB(80, 220, 110) : RGB(20, 140, 50);
		CBrush brush;
		brush.CreateSolidBrush(fill);
		CPen linePen;
		linePen.CreatePen(PS_SOLID, 1, line);
		auto oldBrush = dc.SelectBrush(brush);
		dc.SelectPen(linePen);
		dc.Polygon(pts.data(), (int)pts.size());
		dc.SelectBrush(oldBrush);
	}
	dc.SelectPen(oldPen);

	CPen border;
	border.CreatePen(PS_SOLID, 1, grid);
	oldPen = dc.SelectPen(border);
	dc.SelectStockBrush(NULL_BRUSH);
	dc.Rectangle(&rc);
	dc.SelectPen(oldPen);

	CString label;
	label.Format(L"CPU %d  %d%%", cpu, h.empty() ? 0 : h.back());
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(text);
	CRect textRect(rc);
	textRect.DeflateRect(4, 2);
	dc.DrawText(label, -1, &textRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
}

LRESULT CCPUGraphView::OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
	CPaintDC pdc(m_hWnd);
	CRect rc;
	GetClientRect(&rc);
	if (rc.IsRectEmpty())
		return 0;

	CMemoryDC dc(pdc, rc);
	bool dark = WTLHelper::IsDarkMode();
	dc.FillSolidRect(&rc, dark ? RGB(24, 24, 24) : ::GetSysColor(COLOR_WINDOW));
	dc.SelectFont(AtlGetDefaultGuiFont());

	int count = (int)m_History.size();
	if (count == 0)
		return 0;

	// pick the column count whose cells come closest to a 3:1 aspect ratio, which suits time-series
	int bestCols = 1;
	double bestScore = 1e18;
	for (int cols = 1; cols <= count; cols++) {
		int rows = (count + cols - 1) / cols;
		double w = (double)rc.Width() / cols, h = (double)rc.Height() / rows;
		double score = std::abs(std::log((w / h) / 3.0));
		if (score < bestScore) {
			bestScore = score;
			bestCols = cols;
		}
	}
	int cols = bestCols, rows = (count + cols - 1) / cols;

	for (int i = 0; i < count; i++) {
		int r = i / cols, c = i % cols;
		CRect cell(rc.left + rc.Width() * c / cols, rc.top + rc.Height() * r / rows,
			rc.left + rc.Width() * (c + 1) / cols, rc.top + rc.Height() * (r + 1) / rows);
		cell.DeflateRect(1, 1);
		DrawCell(dc.m_hDC, cell, i, dark);
	}
	return 0;
}
