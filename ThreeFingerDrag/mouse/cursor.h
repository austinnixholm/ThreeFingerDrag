#pragma once
#include "../framework.h"

class Cursor
{
public:
    static void SimulateClick(const DWORD click_flags);
    static void MoveCursor(const double delta_x, const double delta_y);
    static void LeftMouseDown();
    static void LeftMouseUp();
    static bool IsLeftMouseDown();
};
