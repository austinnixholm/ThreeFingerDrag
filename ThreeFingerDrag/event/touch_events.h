#pragma once
#include <chrono>
#include "events.h"
#include "../touch_data.h"

class TouchActivityEventArgs : public EventArgs {
public:
	TouchInputData* data;
	const TouchInputData& previous_data;
	std::chrono::time_point<std::chrono::steady_clock> time;
	TouchActivityEventArgs(std::chrono::time_point<std::chrono::steady_clock> time, TouchInputData* data, const TouchInputData& previous_data)
		: data(data), previous_data(previous_data), time(time) {}
};

class TouchDownEventArgs : public TouchActivityEventArgs {
public:
	TouchDownEventArgs(std::chrono::time_point<std::chrono::steady_clock> time, TouchInputData* data, const TouchInputData& previous_data) : TouchActivityEventArgs{ time, data, previous_data } {}
};

class TouchUpEventArgs : public TouchActivityEventArgs {
public:
	TouchUpEventArgs(std::chrono::time_point<std::chrono::steady_clock> time, TouchInputData* data, const TouchInputData& previous_data) : TouchActivityEventArgs{ time, data, previous_data } {}
};