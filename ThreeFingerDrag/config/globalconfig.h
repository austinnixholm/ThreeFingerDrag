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
constexpr auto DEFAULT_GESTURE_START_THRESHOLD_MS = 500;
constexpr auto DEFAULT_ONE_FINGER_TRANSITION_DELAY_MS = 100;
constexpr auto DEFAULT_AUTOMATIC_TIMEOUT_DELAY_MS = 33;

class GlobalConfig
{
private:
    double gesture_speed_;
    double precision_touch_cursor_speed_;
    double mouse_cursor_speed_;
    int gesture_activation_threshold_ms_;
    int cancellation_delay_ms_;
    int automatic_timeout_delay_ms;
    int one_finger_transition_delay_ms_;
    int last_contact_count_;
    bool gesture_started_;
    bool cancellation_started_;
    bool log_debug_;
    bool portable_mode_;
    bool using_activation_threshold_;
    bool drag_cancels_on_finger_count_change_;
    std::chrono::time_point<std::chrono::steady_clock> cancellation_time_;
    std::chrono::time_point<std::chrono::steady_clock> last_valid_movement_;
    std::chrono::time_point<std::chrono::steady_clock> last_event_;
    std::chrono::time_point<std::chrono::steady_clock> last_initial_activity_time_;
    std::chrono::time_point<std::chrono::steady_clock> last_one_finger_switch_time_;
    std::vector<TouchContact> previous_touch_contacts_;
    TouchEventType last_touch_event_type_;
    static GlobalConfig* instance_;

    // Private constructor
    GlobalConfig();

public:
    // Singleton instance
    static GlobalConfig* GetInstance();

    int GetGestureActivationThresholdMs() const;
    int GetCancellationDelayMs() const;
    int GetAutomaticTimeoutDelayMs() const;
    int GetOneFingerTransitionDelayMs() const;
    int GetLastContactCount() const;
    double GetGestureSpeed() const;
    bool IsGestureStarted() const;
    bool IsCancellationStarted() const;
    bool LogDebug() const;
    bool IsPortableMode() const;
    bool IsUsingActivationThreshold() const;
    bool DragCancelsOnFingerCountChange() const;
    std::chrono::time_point<std::chrono::steady_clock> GetCancellationTime() const;
    std::chrono::time_point<std::chrono::steady_clock> GetLastValidMovement() const;
    std::chrono::time_point<std::chrono::steady_clock> GetLastEvent() const;
    std::chrono::time_point<std::chrono::steady_clock> GetLastInitialActivityTime() const;
    std::chrono::time_point<std::chrono::steady_clock> GetLastOneFingerSwitchTime() const;
    std::vector<TouchContact> GetPreviousTouchContacts() const;
    TouchEventType GetLastTouchEventType() const;

    void SetGestureActivationThresholdMs(int delay);
    void SetCancellationDelayMs(int delay);
    void SetAutomaticTimeoutDelayMs(int delay);
    void SetOneFingerTransitionDelayMs(int delay);
    void SetLastContactCount(int count);
    void SetGestureSpeed(double speed);
    void SetGestureStarted(bool started);
    void SetCancellationStarted(bool started);
    void SetLogDebug(bool log);
    void SetPortableMode(bool portable);
    void SetUsingActivationThreshold(bool flag);
    void SetDragCancelsOnFingerCountChange(bool flag);
    void SetCancellationTime(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetLastValidMovement(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetLastEvent(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetLastInitialActivityTime(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetLastOneFingerSwitchTime(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetPreviousTouchContacts(const std::vector<TouchContact>& data);
    void SetLastTouchEventType(TouchEventType type);
};

#endif // GLOBALCONFIG_H
