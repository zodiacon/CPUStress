#include "stdafx.h"
#include "AffinityDlg.h"

CAffinityDlg::CAffinityDlg(bool affinity, Thread* pThread, const CString& title)
	: m_Affinity(affinity), m_Title(title), m_pThread(pThread) {
}

CAffinityDlg::CAffinityDlg(const CString& title) 
	: m_Title(title), m_Process(true), m_Affinity(true) {
}

LRESULT CAffinityDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	SetWindowText(m_Title);

	CRect rc;
	GetDlgItem(IDC_DUMMY).GetClientRect(&rc);
	int cpus = Thread::GetCPUCount();

	DWORD_PTR processAffinity, systemAffinity;
	BOOL validAffinity = FALSE;

	if (m_Affinity && m_Process)
		validAffinity = ::GetProcessAffinityMask(::GetCurrentProcess(), &processAffinity, &systemAffinity);

	CString name;
	int row = cpus > 32 ? 8 : 4;
	auto font = GetFont();
	for (int i = 0; i < cpus; i++) {
		name.Format(L"CPU %d", i);
		m_Buttons[i].Create(*this, CRect(CPoint(10 + (rc.Width() + 10) * (i % row), 20 + (rc.Height() + 10) * (i / row)), CSize(rc.Width(), rc.Height())),
			name, WS_CHILD | WS_VISIBLE | (m_Affinity ? BS_AUTOCHECKBOX : BS_AUTORADIOBUTTON | (i == 0 ? WS_GROUP : 0) | WS_TABSTOP),
			0, IDC_FIRST + i);
		if (m_Affinity && m_pThread == nullptr)
			CheckDlgButton(IDC_FIRST + i, (validAffinity && processAffinity & (1i64 << i)) == 0 ? BST_UNCHECKED : BST_CHECKED);
		m_Buttons[i].SetFont(font);
	}

	if (cpus > 16) {
		CRect rcButton, rcWindow;
		GetDlgItem(IDOK).GetClientRect(&rcButton);
		GetWindowRect(&rcWindow);
		int sizex = row == 4 ? rcWindow.Width() : (rcWindow.Width() - rcButton.Width()) * 2 + rcButton.Width();
		if (row > 4) {
			int x1 = sizex / 2 - rcButton.Width() - 20;
			int x2 = sizex / 2 + 20;
			CRect rcButton2;
			GetDlgItem(IDOK).GetWindowRect(&rcButton2);
			ScreenToClient(&rcButton2);
			GetDlgItem(IDOK).MoveWindow(x1, rcButton2.top, rcButton.Width(), rcButton.Height());
			GetDlgItem(IDCANCEL).MoveWindow(x2, rcButton2.top, rcButton.Width(), rcButton.Height());

			GetDlgItem(IDC_SELECTALL).MoveWindow(sizex - rcButton.Width() - 30, 20, rcButton.Width(), rcButton.Height());
			GetDlgItem(IDC_UNSELECTALL).MoveWindow(sizex - rcButton.Width() - 30, 30 + rcButton.Height(), rcButton.Width(), rcButton.Height());
		}
		SetWindowPos(nullptr, 0, 0, sizex, rcWindow.Height(), SWP_NOMOVE | SWP_NOZORDER);
	}

	if (!m_Affinity) {
		GetDlgItem(IDC_SELECTALL).ShowWindow(SW_HIDE);
		GetDlgItem(IDC_UNSELECTALL).ShowWindow(SW_HIDE);
	}

	if (m_Affinity && m_pThread) {
		auto affinity = m_pThread->GetAffinity();
		int i = 0;
		while (affinity) {
			if (affinity & 1)
				CheckDlgButton(IDC_FIRST + i, BST_CHECKED);
			affinity >>= 1;
			i++;
		}
	}
	if (m_Affinity && !m_Process)
		DisableNonProcessAffinity();

	if (!m_Affinity) {
		m_IdealCPU = m_pThread ? m_pThread->GetIdealCPU() : 0;
		m_Buttons[m_IdealCPU].SetCheck(1);
	}

	return 0;
}

LRESULT CAffinityDlg::OnOk(WORD, WORD wID, HWND, BOOL&) {
	if (m_Affinity) {
		auto affinity = CalcAffinity();
		if (affinity == 0) {
			AtlMessageBox(*this, L"Affinity of zero is not allowed");
			return 0;
		}
		m_SelectedAfinity = affinity;
	}
	else {
		for (int i = 0; i < _countof(m_Buttons); i++)
			if (m_Buttons[i].GetCheck()) {
				m_IdealCPU = i;
				break;
			}
	}
	EndDialog(IDOK);
	return 0;
}

LRESULT CAffinityDlg::OnCancel(WORD, WORD wID, HWND, BOOL&) {
	EndDialog(IDCANCEL);
	return 0;
}

void CAffinityDlg::DisableNonProcessAffinity() {
	DWORD_PTR processAffinity, systemAffinity;
	if (!::GetProcessAffinityMask(::GetCurrentProcess(), &processAffinity, &systemAffinity))
		return;

	if (processAffinity == systemAffinity)
		return;

	// disable CPUs that are not part of the process affinity mask
	int n = 0;
	while (processAffinity) {
		if ((processAffinity & 1) == 0) {
			CheckDlgButton(IDC_FIRST + n, BST_UNCHECKED);
			m_Buttons[n].EnableWindow(FALSE);
		}
		processAffinity >>= 1;
		++n;
	}
}

DWORD_PTR CAffinityDlg::CalcAffinity() {
	DWORD_PTR affinity = 0;
	for (int i = 0; i < Thread::GetCPUCount(); i++)
		if (m_Buttons[i].GetCheck() == BST_CHECKED)
			affinity |= (DWORD_PTR)1 << i;
	return affinity;
}

LRESULT CAffinityDlg::OnSelectAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	for (int i = 0; i < Thread::GetCPUCount(); i++)
		CheckDlgButton(IDC_FIRST + i, BST_CHECKED);
	return 0;
}

LRESULT CAffinityDlg::OnUnselectAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	for (int i = 0; i < Thread::GetCPUCount(); i++)
		CheckDlgButton(IDC_FIRST + i, BST_UNCHECKED);
	return 0;
}
