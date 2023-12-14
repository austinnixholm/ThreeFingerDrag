#include "globalconfig.h"

GlobalConfig* GlobalConfig::instance_ = nullptr;

GlobalConfig::GlobalConfig()
{
    // Set default values
    gesture_speed_ = DEFAULT_ACCELERATION_FACTOR;
    cancellation_delay_ms_ = DEFAULT_CANCELLATION_DELAY_MS;
    precision_touch_cursor_speed_ = DEFAULT_PRECISION_CURSOR_SPEED;
    mouse_cursor_speed_ = DEFAULT_MOUSE_CURSOR_SPEED;
    gesture_started_ = false;
    cancellation_started_ = false;
    is_dragging_ = false;
}

GlobalConfig* GlobalConfig::GetInstance()
{
    if (instance_ == nullptr)
        instance_ = new GlobalConfig();
    return instance_;
}

int GlobalConfig::GetCancellationDelayMs() const
{
    return cancellation_delay_ms_;
}

void GlobalConfig::SetCancellationDelayMs(int delay)
{
    cancellation_delay_ms_ = delay;
}

double GlobalConfig::GetGestureSpeed() const
{
    return gesture_speed_;
}

void GlobalConfig::SetGestureSpeed(double speed)
{
    gesture_speed_ = speed;
}

bool GlobalConfig::IsDragging() const
{
    return is_dragging_;
}

void GlobalConfig::SetDragging(bool dragging)
{
    is_dragging_ = dragging;
}

bool GlobalConfig::IsGestureStarted() const
{
    return gesture_started_;
}

void GlobalConfig::SetGestureStarted(bool started)
{
    gesture_started_ = started;
}

bool GlobalConfig::IsCancellationStarted() const
{
    return cancellation_started_;
}

void GlobalConfig::SetCancellationStarted(bool started)
{
    cancellation_started_ = started;
}

std::chrono::time_point<std::chrono::steady_clock> GlobalConfig::GetCancellationTime() const
{
    return cancellation_time_;
}

void GlobalConfig::SetCancellationTime(std::chrono::time_point<std::chrono::steady_clock> time)
{
    cancellation_time_ = time;
}

std::chrono::time_point<std::chrono::steady_clock> GlobalConfig::GetLastValidMovement() const
{
    return last_valid_movement_;
}

void GlobalConfig::SetLastValidMovement(std::chrono::time_point<std::chrono::steady_clock> time)
{
    last_valid_movement_ = time;
}

std::chrono::time_point<std::chrono::steady_clock> GlobalConfig::GetLastGesture() const
{
    return last_gesture_;
}

void GlobalConfig::SetLastGesture(std::chrono::time_point<std::chrono::steady_clock> time)
{
    last_gesture_ = time;
}

TouchInputData GlobalConfig::GetPreviousTouchData() const
{
    return previous_touch_data_;
}

void GlobalConfig::SetPreviousTouchData(TouchInputData&& data)
{
    previous_touch_data_ = std::move(data);
}



