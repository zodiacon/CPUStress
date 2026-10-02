#include "pch.h"
#include "CpuTopology.h"
#include <algorithm>

const CpuTopology& CpuTopology::Get() {
	static CpuTopology topology;
	return topology;
}

CpuTopology::CpuTopology() {
	ULONG len = 0;
	::GetSystemCpuSetInformation(nullptr, 0, &len, ::GetCurrentProcess(), 0);	// fails with the required size
	if (len == 0)
		return;

	auto buffer = std::make_unique<BYTE[]>(len);
	if (!::GetSystemCpuSetInformation(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.get()), len, &len, ::GetCurrentProcess(), 0))
		return;

	// entries are variable sized
	for (ULONG offset = 0; offset < len;) {
		auto info = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.get() + offset);
		if (info->Size == 0)
			break;
		offset += info->Size;
		if (info->Type != CpuSetInformation)
			continue;

		auto& set = info->CpuSet;
		CpuInfo cpu{};
		cpu.CpuSetId = set.Id;
		cpu.Group = set.Group;
		cpu.Number = set.LogicalProcessorIndex;
		cpu.Index = set.Group * 64 + set.LogicalProcessorIndex;
		cpu.Core = set.CoreIndex;
		cpu.EfficiencyClass = set.EfficiencyClass;
		cpu.NumaNode = set.NumaNodeIndex;
		m_Cpus.push_back(cpu);
	}

	std::stable_sort(m_Cpus.begin(), m_Cpus.end(), [](auto& a, auto& b) { return a.Index < b.Index; });

	if (m_Cpus.empty())
		return;

	// hybrid: more than one efficiency class. the highest is the P-cores, anything lower an E-core
	auto [lo, hi] = std::minmax_element(m_Cpus.begin(), m_Cpus.end(),
		[](auto& a, auto& b) { return a.EfficiencyClass < b.EfficiencyClass; });
	m_Hybrid = lo->EfficiencyClass != hi->EfficiencyClass;
	for (auto& cpu : m_Cpus) {
		if (m_Hybrid)
			cpu.Type = cpu.EfficiencyClass == hi->EfficiencyClass ? CoreType::Performance : CoreType::Efficiency;
		m_GroupCount = std::max<int>(m_GroupCount, cpu.Group + 1);
	}
}

int CpuTopology::CountOf(CoreType type) const {
	return (int)std::count_if(m_Cpus.begin(), m_Cpus.end(), [type](auto& c) { return c.Type == type; });
}

const CpuInfo* CpuTopology::Find(int index) const {
	auto it = std::lower_bound(m_Cpus.begin(), m_Cpus.end(), index, [](auto& c, int i) { return c.Index < i; });
	return it != m_Cpus.end() && it->Index == index ? &*it : nullptr;
}

std::vector<ULONG> CpuTopology::CpuSetIdsOf(CoreType type) const {
	std::vector<ULONG> ids;
	for (auto& cpu : m_Cpus)
		if (cpu.Type == type)
			ids.push_back(cpu.CpuSetId);
	return ids;
}

PCWSTR CpuTopology::TypeName(CoreType type) {
	switch (type) {
		case CoreType::Performance: return L"P-core";
		case CoreType::Efficiency: return L"E-core";
	}
	return L"";
}

PCWSTR CpuTopology::TypeShortName(CoreType type) {
	switch (type) {
		case CoreType::Performance: return L"P";
		case CoreType::Efficiency: return L"E";
	}
	return L"";
}
