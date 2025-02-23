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
constexpr auto DEFAULT_ONE_FINGER_TRANSITION_DELAY_MS = 100;
constexpr auto DEFAULT_AUTOMATIC_TIMEOUT_DELAY_MS = 33;
constexpr auto DEFAULT_INERTIA_FRICTION_START = 0.94;           // percentage
constexpr auto DEFAULT_INERTIA_FRICTION_END = 0.86;             // percentage
constexpr auto DEFAULT_MIN_FLICK_VELOCITY = 300.0;              // pixels/second
constexpr auto DEFAULT_MIN_FLICK_DISTANCE = 50.0;               // pixels
constexpr auto DEFAULT_MAX_FLICK_TIMESPAN = 0.2;                // seconds (200ms)
constexpr auto DEFAULT_INERTIA_SPEED_MULTIPLIER = 30;           // range (1 -> 100)

class GlobalConfig
{
private:
    uint16_t inertia_speed_multiplier_;
    double gesture_speed_;
    double precision_touch_cursor_speed_;
    double mouse_cursor_speed_;
    double inertia_velocity_x_;
    double inertia_velocity_y_; 
    double inertia_friction_percentage_start_;
    double inertia_friction_percentage_end_;
    double min_flick_velocity_;
    double min_flick_distance_px_;
    double min_flick_timespan_seconds_;
    int cancellation_delay_ms_;
    int automatic_timeout_delay_ms;
    int one_finger_transition_delay_ms_;
    int last_contact_count_;
    bool gesture_started_;
    bool inertia_enabled_;
    bool inertia_active_;
    bool cancellation_started_;
    bool log_debug_;
    bool portable_mode_;
    std::chrono::time_point<std::chrono::steady_clock> cancellation_time_;
    std::chrono::time_point<std::chrono::steady_clock> last_valid_movement_;
    std::chrono::time_point<std::chrono::steady_clock> last_event_;
    std::chrono::time_point<std::chrono::steady_clock> last_one_finger_switch_time_;
    std::chrono::time_point<std::chrono::steady_clock> inertia_start_time_;
    std::vector<TouchContact> previous_touch_contacts_;

    static GlobalConfig* instance_;

    // Private constructor
    GlobalConfig();

public:
    // Singleton instance
    static GlobalConfig* GetInstance();
    
    uint16_t GetInertiaSpeedMultiplier() const;
    int GetCancellationDelayMs() const;
    int GetAutomaticTimeoutDelayMs() const;
    int GetOneFingerTransitionDelayMs() const;
    int GetLastContactCount() const;
    double GetGestureSpeed() const;
    double GetMinimumFlickVelocity() const;
    double GetMinimumFlickDistancePx() const;
    double GetMinimumFlickTimespanSeconds() const;
    double GetInertiaFrictionPercentageStart() const;
    double GetInertiaFrictionPercentageEnd() const;
    bool IsGestureStarted() const;
    bool IsInertiaEnabled() const;
    bool IsInertiaActive() const;
    bool IsCancellationStarted() const;
    bool LogDebug() const;
    bool IsPortableMode() const;
    std::chrono::time_point<std::chrono::steady_clock> GetCancellationTime() const;
    std::chrono::time_point<std::chrono::steady_clock> GetLastValidMovement() const;
    std::chrono::time_point<std::chrono::steady_clock> GetLastEvent() const;
    std::chrono::time_point<std::chrono::steady_clock> GetLastOneFingerSwitchTime() const;
    std::vector<TouchContact> GetPreviousTouchContacts() const;


    void GetInertiaVelocity(double& vx, double& vy);
    void SetCancellationDelayMs(int delay);
    void SetAutomaticTimeoutDelayMs(int delay);
    void SetOneFingerTransitionDelayMs(int delay);
    void SetLastContactCount(int count);
    void SetGestureSpeed(double speed);
    void SetGestureStarted(bool started);
    void SetMinimumFlickVelocity(double v);
    void SetMinimumFlickDistancePx(double px);
    void SetMinimumFlickTimespanSeconds(double seconds);
    void SetInertiaFrictionPercentageStart(double percentage);
    void SetInertiaFrictionPercentageEnd(double percentage);
    void SetInertiaSpeedMultiplier(uint16_t multiplier);
    void SetInertiaEnabled(bool enabled);
    void StartInertia(double vx, double vy);
    void StopInertia();
    void SetCancellationStarted(bool started);
    void SetLogDebug(bool log);
    void SetPortableMode(bool portable);
    void SetCancellationTime(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetLastValidMovement(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetLastEvent(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetLastOneFingerSwitchTime(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetPreviousTouchContacts(const std::vector<TouchContact>& data);
};

#endif // GLOBALCONFIG_H
