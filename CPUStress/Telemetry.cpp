#include "pch.h"
#include "Telemetry.h"
#include <pdhmsg.h>
#include <algorithm>

#pragma comment(lib, "pdh.lib")

Telemetry::~Telemetry() {
	if (m_Query)
		::PdhCloseQuery(m_Query);
}

bool Telemetry::AddCounter(PCWSTR path, PDH_HCOUNTER& counter) {
	// English path so it works on localized Windows; a counter that doesn't exist here is simply skipped
	return ::PdhAddEnglishCounterW(m_Query, path, 0, &counter) == ERROR_SUCCESS;
}

bool Telemetry::Open() {
	if (::PdhOpenQuery(nullptr, 0, &m_Query) != ERROR_SUCCESS)
		return false;

	AddCounter(L"\\Processor Information(*)\\% Processor Performance", m_Performance);
	AddCounter(L"\\Processor Information(*)\\Processor Frequency", m_BaseFrequency);
	AddCounter(L"\\Processor Information(*)\\% Performance Limit", m_PerfLimit);
	AddCounter(L"\\Thermal Zone Information(*)\\Temperature", m_Temperature);
	AddCounter(L"\\Energy Meter(*)\\Power", m_Power);

	// rate style counters need two readings
	::PdhCollectQueryData(m_Query);
	return true;
}

bool Telemetry::Read(PDH_HCOUNTER counter, std::vector<Instance>& values) {
	values.clear();
	if (!counter)
		return false;

	DWORD size = 0, count = 0;
	if (::PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &size, &count, nullptr) != PDH_MORE_DATA)
		return false;

	auto buffer = std::make_unique<BYTE[]>(size);
	auto items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.get());
	if (::PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &size, &count, items) != ERROR_SUCCESS)
		return false;

	for (DWORD i = 0; i < count; i++)
		if (items[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA || items[i].FmtValue.CStatus == PDH_CSTATUS_NEW_DATA)
			values.push_back({ items[i].szName, items[i].FmtValue.doubleValue });
	return !values.empty();
}

// "0,3" -> CPU index 3 of group 0 (group * 64 + number); false for "_Total" style instances
static bool ParseCpuInstance(const CString& name, int& index) {
	int group, number;
	if (swscanf_s(name, L"%d,%d", &group, &number) != 2)
		return false;
	index = group * 64 + number;
	return true;
}

bool Telemetry::Collect(TelemetrySample& sample) {
	sample = {};
	if (!m_Query || ::PdhCollectQueryData(m_Query) != ERROR_SUCCESS)
		return false;
	if (!m_Primed) {
		m_Primed = true;
		return false;
	}

	std::vector<Instance> values;

	// frequency = base frequency x performance %, which exceeds 100% while boosting
	std::unordered_map<int, double> baseMHz;
	if (Read(m_BaseFrequency, values))
		for (auto& v : values) {
			int index;
			if (ParseCpuInstance(v.Name, index))
				baseMHz[index] = v.Value;
		}
	if (Read(m_Performance, values)) {
		double sum = 0, max = 0;
		for (auto& v : values) {
			int index;
			if (!ParseCpuInstance(v.Name, index))
				continue;
			auto base = baseMHz.find(index);
			if (base == baseMHz.end())
				continue;
			double mhz = base->second * v.Value / 100.0;
			sample.CpuMHz[index] = mhz;
			sum += mhz;
			max = (std::max)(max, mhz);
		}
		if (!sample.CpuMHz.empty()) {
			sample.AverageMHz = sum / sample.CpuMHz.size();
			sample.MaxMHz = max;
		}
	}

	if (Read(m_PerfLimit, values)) {
		for (auto& v : values)
			if (v.Name == L"_Total")
				sample.PerformanceLimit = v.Value;
	}

	if (Read(m_Temperature, values)) {
		double hottest = -1;
		for (auto& v : values)
			hottest = (std::max)(hottest, v.Value - 273.15);		// reported in Kelvin
		if (hottest > 0 && hottest < 150)		// ignore bogus zones
			sample.TemperatureC = hottest;
	}

	if (Read(m_Power, values)) {
		// RAPL package counters; the unit is milliwatts
		double total = 0;
		bool found = false;
		for (auto& v : values)
			if (v.Name.MakeUpper().Find(L"PKG") >= 0) {
				total += v.Value;
				found = true;
			}
		if (found)
			sample.PowerW = total / 1000.0;
	}
	return true;
}
