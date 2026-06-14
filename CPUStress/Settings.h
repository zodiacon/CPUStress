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
};
