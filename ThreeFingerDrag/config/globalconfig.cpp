#include "globalconfig.h"

GlobalConfig* GlobalConfig::instance = nullptr;

GlobalConfig::GlobalConfig() {
    // Set default values
    gesture_speed_ = DEFAULT_ACCELERATION_FACTOR;
    cancellation_delay_ms_ = DEFAULT_CANCELLATION_DELAY_MS;
    precision_touch_cursor_speed_ = DEFAULT_PRECISION_CURSOR_SPEED;
    mouse_cursor_speed_ = DEFAULT_MOUSE_CURSOR_SPEED;
    cancellation_started_ = false;
    is_dragging_ = false;
}

GlobalConfig* GlobalConfig::GetInstance() {
    if (instance == nullptr)
        instance = new GlobalConfig();
    return instance;
}

int GlobalConfig::GetCancellationDelayMs() const {
    return cancellation_delay_ms_;
}

void GlobalConfig::SetCancellationDelayMs(int delay) {
    cancellation_delay_ms_ = delay;
}

double GlobalConfig::GetPrecisionTouchCursorSpeed() const {
    return precision_touch_cursor_speed_;
}

void GlobalConfig::SetPrecisionTouchCursorSpeed(double speed) {
    precision_touch_cursor_speed_ = speed;
}

double GlobalConfig::GetMouseCursorSpeed() const {
    return mouse_cursor_speed_;
}

void GlobalConfig::SetMouseCursorSpeed(double speed) {
    mouse_cursor_speed_ = speed;
}

double GlobalConfig::GetGestureSpeed() const {
    return gesture_speed_;
}

void GlobalConfig::SetGestureSpeed(double speed) {
    gesture_speed_ = speed;
}

bool GlobalConfig::IsDragging() const {
    return is_dragging_;
}

void GlobalConfig::SetDragging(bool dragging) {
    is_dragging_ = dragging;
}

bool GlobalConfig::IsGestureStarted() const {
    return gesture_started_;
}

void GlobalConfig::SetGestureStarted(bool started) {
    gesture_started_ = started;
}

bool GlobalConfig::IsCancellationStarted() const {
    return cancellation_started_;
}

void GlobalConfig::SetCancellationStarted(bool started) {
    cancellation_started_ = started;
}

std::chrono::time_point<std::chrono::steady_clock> GlobalConfig::GetCancellationTime() const {
    return cancellation_time_;
}

void GlobalConfig::SetCancellationTime(std::chrono::time_point<std::chrono::steady_clock> time) {
    cancellation_time_ = time;
}

std::chrono::time_point<std::chrono::steady_clock> GlobalConfig::GetLastValidMovement() const {
    return last_valid_movement_;
}

void GlobalConfig::SetLastValidMovement(std::chrono::time_point<std::chrono::steady_clock> time) {
    last_valid_movement_ = time;
}

std::chrono::time_point<std::chrono::steady_clock> GlobalConfig::GetLastGesture() const {
    return last_gesture_;
}

void GlobalConfig::SetLastGesture(std::chrono::time_point<std::chrono::steady_clock> time) {
    last_gesture_ = time;
}