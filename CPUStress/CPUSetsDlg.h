#pragma once

#include "resource.h"

enum class CPUSetsType {
	System,
	Process,
	Thread
};

class Thread;

class CCPUSetsDlg : public CDialogImpl<CCPUSetsDlg> {
public:
	CCPUSetsDlg(CPUSetsType type, Thread* thread = nullptr);
	enum { IDD = IDD_CPUSETS };

	ULONG* GetCpuSet(ULONG& count) const;

	BEGIN_MSG_MAP(CCPUSetsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDOK, OnOK)
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void FillCPUSets();

private:
	CListViewCtrl m_List;
	CPUSetsType m_Type;
	std::unique_ptr<ULONG[]> m_CpuSets;
	ULONG m_CpuSetCount;
	Thread* m_pThread;
};

