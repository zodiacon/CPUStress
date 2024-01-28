#pragma once

struct IMainFrame {
	virtual bool ShowContextMenu(HMENU hMenu, POINT pt) = 0;
	virtual bool SetStatusText(int pane, PCWSTR text) = 0;
};
