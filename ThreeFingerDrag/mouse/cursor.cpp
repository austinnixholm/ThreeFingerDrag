#include "cursor.h"

void Cursor::SimulateClick(const DWORD click_flags)
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dx = 0;
    input.mi.dy = 0;
    input.mi.mouseData = 0;
    input.mi.dwFlags = click_flags;
    input.mi.time = 0;
    input.mi.dwExtraInfo = 0;
    SendInput(1, &input, sizeof(INPUT));
}

void Cursor::MoveCursor(const double delta_x, const double delta_y)
{
    // Prepare an INPUT structure for the SendInput function to cause a relative mouse move.
    INPUT input;
    input.type = INPUT_MOUSE;

    input.mi.dx = delta_x;
    input.mi.dy = delta_y;
    input.mi.mouseData = 0;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.time = 0;
    input.mi.dwExtraInfo = 0;

    SendInput(1, &input, sizeof(INPUT));
}

void Cursor::LeftMouseDown()
{
    SimulateClick(MOUSEEVENTF_LEFTDOWN);
}

void Cursor::LeftMouseUp()
{
    SimulateClick(MOUSEEVENTF_LEFTUP);
}
