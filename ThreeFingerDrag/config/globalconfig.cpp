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
    log_debug_ = false;
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

void GlobalConfig::SetCancellationTime(const std::chrono::time_point<std::chrono::steady_clock> time)
{
    cancellation_time_ = time;
}

std::chrono::time_point<std::chrono::steady_clock> GlobalConfig::GetLastValidMovement() const
{
    return last_valid_movement_;
}

void GlobalConfig::SetLastValidMovement(const std::chrono::time_point<std::chrono::steady_clock> time)
{
    last_valid_movement_ = time;
}

std::chrono::time_point<std::chrono::steady_clock> GlobalConfig::GetLastEvent() const
{
    return last_event_;
}

void GlobalConfig::SetLastEvent(const std::chrono::time_point<std::chrono::steady_clock> time)
{
    last_event_ = time;
}

std::vector<TouchContact> GlobalConfig::GetPreviousTouchContacts() const
{
    return previous_touch_contacts_;
}

void GlobalConfig::SetPreviousTouchContacts(const std::vector<TouchContact>& data)
{
    previous_touch_contacts_ = data;
}

bool GlobalConfig::LogDebug() const
{
    return log_debug_;
}

void GlobalConfig::SetLogDebug(bool log)
{
    log_debug_ = log;
}

int GlobalConfig::GetLastContactCount() const
{
    return last_contact_count_;
}

void GlobalConfig::SetLastContactCount(int count)
{
    last_contact_count_ = count;
}

std::chrono::time_point<std::chrono::steady_clock> GlobalConfig::GetLastOneFingerSwitchTime() const
{
    return last_one_finger_switch_time_;
}

void GlobalConfig::SetLastOneFingerSwitchTime(std::chrono::time_point<std::chrono::steady_clock> time)
{
    last_one_finger_switch_time_ = time;
}






