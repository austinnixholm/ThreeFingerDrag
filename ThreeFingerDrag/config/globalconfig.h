#pragma once
#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H
#include <chrono>
#include "../data/touch_data.h"
#include "../event/touch_events.h"

constexpr auto DEFAULT_ACCELERATION_FACTOR = 15.0;
constexpr auto DEFAULT_PRECISION_CURSOR_SPEED = 0.5;
constexpr auto DEFAULT_MOUSE_CURSOR_SPEED = 0.5;
constexpr auto DEFAULT_CANCELLATION_DELAY_MS = 500;

class GlobalConfig
{
private:
    double gesture_speed_;
    double precision_touch_cursor_speed_;
    double mouse_cursor_speed_;
    int cancellation_delay_ms_;
    int last_contact_count_;
    bool gesture_started_;
    bool cancellation_started_;
    bool log_debug_;
    std::chrono::time_point<std::chrono::steady_clock> cancellation_time_;
    std::chrono::time_point<std::chrono::steady_clock> last_valid_movement_;
    std::chrono::time_point<std::chrono::steady_clock> last_event_;
    std::chrono::time_point<std::chrono::steady_clock> last_one_finger_switch_time_;
    std::vector<TouchContact> previous_touch_contacts_;
    static GlobalConfig* instance_;

    // Private constructor
    GlobalConfig();

public:
    // Singleton instance
    static GlobalConfig* GetInstance();
    
    int GetCancellationDelayMs() const;
    int GetLastContactCount() const;
    double GetGestureSpeed() const;
    bool IsGestureStarted() const;
    bool IsCancellationStarted() const;
    bool LogDebug() const;
    std::chrono::time_point<std::chrono::steady_clock> GetCancellationTime() const;
    std::chrono::time_point<std::chrono::steady_clock> GetLastValidMovement() const;
    std::chrono::time_point<std::chrono::steady_clock> GetLastEvent() const;
    std::chrono::time_point<std::chrono::steady_clock> GetLastOneFingerSwitchTime() const;
    std::vector<TouchContact> GetPreviousTouchContacts() const;

    void SetCancellationDelayMs(int delay);
    void SetLastContactCount(int count);
    void SetGestureSpeed(double speed);
    void SetGestureStarted(bool started);
    void SetCancellationStarted(bool started);
    void SetLogDebug(bool log);
    void SetCancellationTime(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetLastValidMovement(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetLastEvent(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetLastOneFingerSwitchTime(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetPreviousTouchContacts(const std::vector<TouchContact>& data);
};

#endif // GLOBALCONFIG_H
