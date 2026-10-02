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

	// restored (not minimized) window position and size, and whether it was maximized
	struct WindowState {
		RECT Rect;
		DWORD Maximized;
	};

	static bool LoadWindow(WindowState& state) {
		DWORD size = sizeof(state);
		return ::RegGetValue(HKEY_CURRENT_USER, RegPath, L"Window",
			RRF_RT_REG_BINARY, nullptr, &state, &size) == ERROR_SUCCESS && size == sizeof(state);
	}

	static void SaveWindow(const WindowState& state) {
		::RegSetKeyValue(HKEY_CURRENT_USER, RegPath, L"Window",
			REG_BINARY, &state, sizeof(state));
	}

	// splitter position between the thread list and the CPU graphs, as a fraction (in 1/1000) of the height
	static int SplitterRatio() {
		DWORD value = 650, size = sizeof(value);
		::RegGetValue(HKEY_CURRENT_USER, RegPath, L"SplitterRatio",
			RRF_RT_REG_DWORD, nullptr, &value, &size);
		return (std::min)((std::max)((int)value, 100), 900);
	}

	static void SplitterRatio(int ratio) {
		DWORD value = ratio;
		::RegSetKeyValue(HKEY_CURRENT_USER, RegPath, L"SplitterRatio",
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
