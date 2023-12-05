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


void Cursor::MoveCursor(const int delta_x, const int delta_y)
{
	GlobalConfig* config = GlobalConfig::GetInstance();

	// Calculate the scaling factor which is the ratio of precision touch cursor speed to mouse cursor speed. 
	// To avoid division by zero, we replace mouse_cursor_speed_ with 1 when it's 0.
	const float scaling_factor = (config->GetPrecisionTouchCursorSpeed() / (config->GetMouseCursorSpeed() > 0 ? config->GetMouseCursorSpeed() : 1));

	// Apply a non-linear transformation to the scaling by squaring it. This will scale cursor movements less when mouse speed is low and more when mouse speed is high.
	const float non_linear_scaling_factor = scaling_factor * scaling_factor;

	// Prepare an INPUT structure for the SendInput function to cause a relative mouse move.
	INPUT input;
	input.type = INPUT_MOUSE;

	input.mi.dx = static_cast<int>(delta_x * non_linear_scaling_factor);
	input.mi.dy = static_cast<int>(delta_y * non_linear_scaling_factor);
	input.mi.mouseData = 0;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	input.mi.time = 0;
	input.mi.dwExtraInfo = 0;

	// Asynchronously call the SendInput function to cause the cursor movement. 
	//auto a = std::async(std::launch::async, SendInput, 1, &input, sizeof(INPUT));
	SendInput(1, &input, sizeof(INPUT));
}

void Cursor::LeftMouseDown() {
	SimulateClick(MOUSEEVENTF_LEFTDOWN);
}

void Cursor::LeftMouseUp() {
	SimulateClick(MOUSEEVENTF_LEFTUP);
}