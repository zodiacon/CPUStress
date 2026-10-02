#pragma once

#include "WTLHelper.h"

//
// Minimal registry-backed persistence (HKCU\Software\CPUStress).
//
struct Settings {
	inline static PCWSTR RegPath = L"Software\\CPUStress";

	static bool DarkMode() {
		DWORD value = 0, size = sizeof(value);
		if (::RegGetValue(HKEY_CURRENT_USER, RegPath, L"DarkMode",
			RRF_RT_REG_DWORD, nullptr, &value, &size) == ERROR_SUCCESS)
			return value != 0;
		// no saved preference: follow the current Windows setting
		return WTLHelper::IsSystemInDarkMode();
	}

	static void DarkMode(bool dark) {
		DWORD value = dark ? 1 : 0;
		::RegSetKeyValue(HKEY_CURRENT_USER, RegPath, L"DarkMode",
			REG_DWORD, &value, sizeof(value));
	}

	static bool CPUGraphs() {
		DWORD value = 1, size = sizeof(value);
		::RegGetValue(HKEY_CURRENT_USER, RegPath, L"CPUGraphs",
			RRF_RT_REG_DWORD, nullptr, &value, &size);
		return value != 0;		// shown unless the user hid it
	}

	static void CPUGraphs(bool show) {
		DWORD value = show ? 1 : 0;
		::RegSetKeyValue(HKEY_CURRENT_USER, RegPath, L"CPUGraphs",
			REG_DWORD, &value, sizeof(value));
	}

	// Returns the saved list view font, or a zeroed LOGFONT (lfHeight == 0) if none is stored.
	static LOGFONT Font() {
		LOGFONT lf{};
		DWORD size = sizeof(lf);
		::RegGetValue(HKEY_CURRENT_USER, RegPath, L"Font",
			RRF_RT_REG_BINARY, nullptr, &lf, &size);
		return lf;
	}

	static void Font(const LOGFONT& lf) {
		::RegSetKeyValue(HKEY_CURRENT_USER, RegPath, L"Font",
			REG_BINARY, &lf, sizeof(lf));
	}
};
