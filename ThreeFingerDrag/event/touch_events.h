#pragma once
#include <chrono>
#include "events.h"
#include "../data/touch_data.h"

class TouchActivityEventArgs : public EventArgs
{
public:
    TouchInputData* data;
    std::vector<TouchContact> previous_data;
    std::chrono::time_point<std::chrono::steady_clock> time;

    TouchActivityEventArgs(
        std::chrono::time_point<std::chrono::steady_clock> time,
        TouchInputData* data,
        const std::vector<TouchContact>& previous_data)
        : data(data), previous_data(previous_data), time(time)
    {
    }
};

class TouchUpEventArgs : public TouchActivityEventArgs
{
public:
    TouchUpEventArgs(
        std::chrono::time_point<std::chrono::steady_clock> time,
        TouchInputData* data,
        const std::vector<TouchContact>& previous_data)
        : TouchActivityEventArgs{time, data, previous_data}
    {
    }
};
