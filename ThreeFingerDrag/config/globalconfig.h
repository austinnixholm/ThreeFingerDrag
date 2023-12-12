#pragma once
#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H
#include <iostream>
#include <chrono>

constexpr auto DEFAULT_ACCELERATION_FACTOR = 75.0;
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
    bool is_dragging_;
    bool gesture_started_;
    bool cancellation_started_;
    std::chrono::time_point<std::chrono::steady_clock> cancellation_time_;
    std::chrono::time_point<std::chrono::steady_clock> last_valid_movement_;
    std::chrono::time_point<std::chrono::steady_clock> last_gesture_;
    static GlobalConfig* instance;

    // Private constructor
    GlobalConfig();

public:
    // Singleton instance
    static GlobalConfig* GetInstance();

    int GetCancellationDelayMs() const;
    double GetPrecisionTouchCursorSpeed() const;
    double GetMouseCursorSpeed() const;
    double GetGestureSpeed() const;
    bool IsDragging() const;
    bool IsGestureStarted() const;
    bool IsCancellationStarted() const;
    std::chrono::time_point<std::chrono::steady_clock> GetCancellationTime() const;
    std::chrono::time_point<std::chrono::steady_clock> GetLastValidMovement() const;
    std::chrono::time_point<std::chrono::steady_clock> GetLastGesture() const;

    void SetCancellationDelayMs(int delay);
    void SetPrecisionTouchCursorSpeed(double speed);
    void SetMouseCursorSpeed(double speed);
    void SetGestureSpeed(double speed);
    void SetDragging(bool dragging);
    void SetGestureStarted(bool started);
    void SetCancellationStarted(bool started);
    void SetCancellationTime(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetLastValidMovement(std::chrono::time_point<std::chrono::steady_clock> time);
    void SetLastGesture(std::chrono::time_point<std::chrono::steady_clock> time);
};

#endif // GLOBALCONFIG_H
