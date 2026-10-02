// Telemetry.h : CPU frequency, temperature and power from Windows performance counters (PDH)
//
// No driver is needed, so what is available depends on the machine:
//  - frequency: "Processor Information" (% Processor Performance x base frequency); works almost everywhere
//  - temperature: ACPI "Thermal Zone Information"; many systems expose a single zone that is not the CPU die
//  - power: "Energy Meter" RAPL package counters, present only on some Intel systems
// Anything not available is reported as a negative value.

#pragma once

#include <pdh.h>
#include <unordered_map>
#include <vector>

struct TelemetrySample {
	std::unordered_map<int, double> CpuMHz;		// by CPU index (group * 64 + number)
	double AverageMHz{ -1 };
	double MaxMHz{ -1 };
	double TemperatureC{ -1 };		// hottest thermal zone
	double PowerW{ -1 };			// CPU package
	double PerformanceLimit{ -1 };	// "% Performance Limit": 100 when unrestricted, lower when the OS/platform caps performance
};

class Telemetry {
public:
	Telemetry() = default;
	~Telemetry();
	Telemetry(const Telemetry&) = delete;
	Telemetry& operator=(const Telemetry&) = delete;

	bool Open();
	// takes a new reading; the first call after Open only primes the counters and returns false
	bool Collect(TelemetrySample& sample);

private:
	struct Instance {
		CString Name;
		double Value;
	};
	bool AddCounter(PCWSTR path, PDH_HCOUNTER& counter);
	static bool Read(PDH_HCOUNTER counter, std::vector<Instance>& values);

	PDH_HQUERY m_Query{ nullptr };
	PDH_HCOUNTER m_Performance{ nullptr };
	PDH_HCOUNTER m_BaseFrequency{ nullptr };
	PDH_HCOUNTER m_PerfLimit{ nullptr };
	PDH_HCOUNTER m_Temperature{ nullptr };
	PDH_HCOUNTER m_Power{ nullptr };
	bool m_Primed{ false };
};
