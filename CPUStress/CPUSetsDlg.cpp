#include "pch.h"
#include "CPUSetsDlg.h"
#include <VersionHelpers.h>
#include <string>
#include "Thread.h"
#include "CpuTopology.h"

CCPUSetsDlg::CCPUSetsDlg(CPUSetsType type, Thread* thread) : m_Type(type), m_pThread(thread) {
}

ULONG* CCPUSetsDlg::GetCpuSet(ULONG& count) const {
	count = m_CpuSetCount;
	return m_CpuSets.get();
}

LRESULT CCPUSetsDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	m_List.Attach(GetDlgItem(IDC_CPUSETS));
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | (m_Type != CPUSetsType::System ? LVS_EX_CHECKBOXES : 0));

	m_List.InsertColumn(0, L"ID", LVCFMT_RIGHT, 100);
	m_List.InsertColumn(1, L"Group", LVCFMT_CENTER, 50);
	m_List.InsertColumn(2, L"Core", LVCFMT_CENTER, 60);
	m_List.InsertColumn(3, L"LP", LVCFMT_CENTER, 60);
	m_List.InsertColumn(4, L"Node", LVCFMT_CENTER, 60);
	m_List.InsertColumn(5, L"Flags", LVCFMT_RIGHT, 80);
	m_List.InsertColumn(6, L"Allocated", LVCFMT_CENTER, 70);
	m_List.InsertColumn(7, L"Efficiency", LVCFMT_CENTER, 70);
	m_List.InsertColumn(8, L"Type", LVCFMT_CENTER, 60);

	GetDlgItem(IDOK).ShowWindow(m_Type != CPUSetsType::System ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDCANCEL).ShowWindow(m_Type != CPUSetsType::System ? SW_SHOW : SW_HIDE);

	if (m_Type == CPUSetsType::Process)
		SetWindowText(L"Process CPU Set");
	else if (m_Type == CPUSetsType::Thread)
		SetWindowText((L"Thread " + std::to_wstring(m_pThread->GetId()) + L" Selected CPU Set").c_str());

	FillCPUSets();

	return TRUE;
}

LRESULT CCPUSetsDlg::OnCancel(WORD, WORD wID, HWND, BOOL&) {
	EndDialog(wID);
	return 0;
}

LRESULT CCPUSetsDlg::OnOK(WORD, WORD wID, HWND, BOOL&) {
	if (m_Type != CPUSetsType::System) {
		m_CpuSets = std::make_unique<ULONG[]>(m_List.GetItemCount());
		int count = 0;
		for (int i = 0; i < m_List.GetItemCount(); i++)
			if (m_List.GetCheckState(i))
				m_CpuSets[count++] = (ULONG)m_List.GetItemData(i);
		m_CpuSetCount = count;
	}
	EndDialog(wID);
	return 0;
}

void CCPUSetsDlg::FillCPUSets() {
	// the entries are variable sized and there can be many (hundreds of CPUs)
	ULONG len = 0;
	::GetSystemCpuSetInformation(nullptr, 0, &len, ::GetCurrentProcess(), 0);
	auto buffer = std::make_unique<BYTE[]>(len ? len : 1);
	if (len == 0 || !::GetSystemCpuSetInformation(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.get()), len, &len, ::GetCurrentProcess(), 0)) {
		AtlMessageBox(*this, L"Failed to get system CPU set", L"CPU Stress", MB_ICONERROR);
		EndDialog(IDCANCEL);
		return;
	}
	std::vector<PSYSTEM_CPU_SET_INFORMATION> info;
	for (ULONG offset = 0; offset < len; ) {
		auto entry = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.get() + offset);
		if (entry->Size == 0)
			break;
		offset += entry->Size;
		if (entry->Type == CpuSetInformation)
			info.push_back(entry);
	}

	ULONG count = (ULONG)info.size();
	auto ids = std::make_unique<ULONG[]>(count);
	if (m_Type == CPUSetsType::Process) {
		if (!::GetProcessDefaultCpuSets(::GetCurrentProcess(), ids.get(), count, &len)) {
			AtlMessageBox(*this, L"Failed to get process CPU set", L"CPU Stress", MB_ICONERROR);
			EndDialog(IDCANCEL);
			return;
		}
	}
	else if (m_Type == CPUSetsType::Thread) {
		std::vector<ULONG> s;
		if(!m_pThread->GetCpuSet(s)) {
			AtlMessageBox(*this, L"Failed to get thread selected CPU set", L"CPU Stress", MB_ICONERROR);
			EndDialog(IDCANCEL);
			return;
		}
		len = (ULONG)s.size();
		::memcpy(ids.get(), s.data(), len * sizeof(ULONG));
	}

	CString text;
	ULONG index = 0;
	for (ULONG i = 0; i < count; i++) {
		auto& set = info[i]->CpuSet;
		text.Format(L"0x%X (%u)", set.Id, set.Id);
		int n = m_List.InsertItem(i, text);
		if (m_Type != CPUSetsType::System && index < len && ids[index] == set.Id) {
			m_List.SetCheckState(n, TRUE);
			index++;
		}
		m_List.SetItemData(n, set.Id);
		m_List.SetItemText(n, 1, std::to_wstring(set.Group).c_str());
		m_List.SetItemText(n, 2, std::to_wstring(set.CoreIndex).c_str());
		m_List.SetItemText(n, 3, std::to_wstring(set.LogicalProcessorIndex).c_str());
		m_List.SetItemText(n, 4, std::to_wstring(set.NumaNodeIndex).c_str());
		text.Format(L"0x%X (%u)", set.AllFlags, set.AllFlags);
		m_List.SetItemText(n, 5, text);
		m_List.SetItemText(n, 6, set.AllocatedToTargetProcess ? L"Yes" : L"No");
		m_List.SetItemText(n, 7, std::to_wstring(set.EfficiencyClass).c_str());
		if (auto cpu = CpuTopology::Get().Find(set.Group * 64 + set.LogicalProcessorIndex))
			m_List.SetItemText(n, 8, CpuTopology::TypeShortName(cpu->Type));
	}
}
