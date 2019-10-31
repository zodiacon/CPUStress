#pragma once

#include "resource.h"
#include "Thread.h"

class CAffinityDlg : public CDialogImpl<CAffinityDlg> {
public:
	enum { IDD = IDD_AFFINITY };

	CAffinityDlg(bool affinity, Thread* pThread, const CString& title);
	CAffinityDlg(const CString& title);

	enum { IDC_FIRST = 100 };

	DWORD_PTR GetSelectedAffinity() const {
		return m_SelectedAfinity;
	}
	int GetSelectedCPU() const {
		return m_IdealCPU;
	}

	BEGIN_MSG_MAP(CAffinityDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOk)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_SELECTALL, OnSelectAll)
		COMMAND_ID_HANDLER(IDC_UNSELECTALL, OnUnselectAll)
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSelectAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnUnselectAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	DWORD_PTR CalcAffinity();
	void DisableNonProcessAffinity();

	bool m_Affinity;
	bool m_Process = false;
	CString m_Title;
	CButton m_Buttons[64];
	Thread* m_pThread{ nullptr };
	DWORD_PTR m_SelectedAfinity;
	int m_IdealCPU;

};

