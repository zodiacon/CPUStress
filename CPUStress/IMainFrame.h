#pragma once

struct IMainFrame {
	virtual BOOL ShowContextMenu(HMENU hMenu, POINT pt) = 0;
};
