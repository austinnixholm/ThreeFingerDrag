#pragma once
#include <chrono>
#include "events.h"
#include "../data/touch_data.h"

class TouchActivityEventArgs : public EventArgs
{
public:
    TouchInputData* data;
    TouchInputData previous_data;
    std::chrono::time_point<std::chrono::steady_clock> time;

    TouchActivityEventArgs(std::chrono::time_point<std::chrono::steady_clock> time, TouchInputData* data,
                           TouchInputData previous_data)
        : data(data), previous_data(std::move(previous_data)), time(time)
    {
    }
};

class TouchUpEventArgs : public TouchActivityEventArgs
{
public:
    TouchUpEventArgs(std::chrono::time_point<std::chrono::steady_clock> time, TouchInputData* data,
                      TouchInputData previous_data) : TouchActivityEventArgs{time, data, std::move(previous_data)}
    {
    }
};
