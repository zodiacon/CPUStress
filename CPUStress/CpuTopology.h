// CpuTopology.h : logical CPU layout (processor groups, hybrid core types), from GetSystemCpuSetInformation
//
// CPUs are identified the way the rest of the app does it: group * 64 + number in group. The order
// of Cpus() is also the system-wide processor order, which is the order of NtQuerySystemInformation's
// per-processor data.

#pragma once

#include <vector>

enum class CoreType {
	Unknown,
	Performance,	// highest efficiency class on a hybrid system
	Efficiency		// any lower class on a hybrid system
};

struct CpuInfo {
	ULONG CpuSetId;				// id for the CPU set APIs (not the same as the CPU index)
	int Index;					// group * 64 + number
	WORD Group;
	BYTE Number;				// logical processor number within the group
	BYTE Core;					// core index (SMT siblings share it)
	BYTE EfficiencyClass;		// higher is more performant
	BYTE NumaNode;
	CoreType Type;
};

class CpuTopology {
public:
	static const CpuTopology& Get();

	const std::vector<CpuInfo>& Cpus() const {
		return m_Cpus;
	}
	bool IsHybrid() const {
		return m_Hybrid;
	}
	int GroupCount() const {
		return m_GroupCount;
	}
	int CountOf(CoreType type) const;

	// null if the index is not a CPU of this system
	const CpuInfo* Find(int index) const;

	// CPU set ids of all CPUs of the given type
	std::vector<ULONG> CpuSetIdsOf(CoreType type) const;

	static PCWSTR TypeName(CoreType type);			// "P-core" / "E-core" / ""
	static PCWSTR TypeShortName(CoreType type);		// "P" / "E" / ""

private:
	CpuTopology();

	std::vector<CpuInfo> m_Cpus;
	bool m_Hybrid{ false };
	int m_GroupCount{ 1 };
};
