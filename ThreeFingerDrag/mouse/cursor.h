#pragma once
#include "../framework.h"
#include "../config/globalconfig.h"

class Cursor {
public:
	static void SimulateClick(const DWORD click_flags);
	static void MoveCursor(const int delta_x, const int delta_y);
	static void LeftMouseDown();
	static void LeftMouseUp();
};