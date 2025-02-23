#include "globalconfig.h"

GlobalConfig* GlobalConfig::instance_ = nullptr;

GlobalConfig::GlobalConfig()
{
    gesture_speed_ = DEFAULT_ACCELERATION_FACTOR;
    cancellation_delay_ms_ = DEFAULT_CANCELLATION_DELAY_MS;
    automatic_timeout_delay_ms = DEFAULT_AUTOMATIC_TIMEOUT_DELAY_MS;
    one_finger_transition_delay_ms_ = DEFAULT_ONE_FINGER_TRANSITION_DELAY_MS;
    precision_touch_cursor_speed_ = DEFAULT_PRECISION_CURSOR_SPEED;
    mouse_cursor_speed_ = DEFAULT_MOUSE_CURSOR_SPEED;
    min_flick_velocity_ = DEFAULT_MIN_FLICK_VELOCITY;
    min_flick_distance_px_ = DEFAULT_MIN_FLICK_DISTANCE;
    min_flick_timespan_seconds_ = DEFAULT_MAX_FLICK_TIMESPAN;
    inertia_speed_multiplier_ = DEFAULT_INERTIA_SPEED_MULTIPLIER;
    inertia_friction_percentage_start_ = DEFAULT_INERTIA_FRICTION_START;
    inertia_friction_percentage_end_ = DEFAULT_INERTIA_FRICTION_END;

    inertia_enabled_ = true;
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

int GlobalConfig::GetOneFingerTransitionDelayMs() const
{
    return one_finger_transition_delay_ms_;
}

void GlobalConfig::SetOneFingerTransitionDelayMs(int delay)
{
    one_finger_transition_delay_ms_ = delay;
}

bool GlobalConfig::IsPortableMode() const
{
    return portable_mode_;
}

void GlobalConfig::SetPortableMode(bool portable)
{
    portable_mode_ = portable;
}

int GlobalConfig::GetAutomaticTimeoutDelayMs() const
{
    return automatic_timeout_delay_ms;
}

void GlobalConfig::SetAutomaticTimeoutDelayMs(int delay)
{
    automatic_timeout_delay_ms = delay;
}

void GlobalConfig::StartInertia(double vx, double vy) {
    inertia_active_ = true;
    inertia_velocity_x_ = vx;
    inertia_velocity_y_ = vy;
    inertia_start_time_ = std::chrono::steady_clock::now();
}

void GlobalConfig::StopInertia() {
    inertia_active_ = false;
    inertia_velocity_x_ = 0;
    inertia_velocity_y_ = 0;
}

bool GlobalConfig::IsInertiaActive() const {
    return inertia_active_;
}

void GlobalConfig::GetInertiaVelocity(double& vx, double& vy) {
    vx = inertia_velocity_x_;
    vy = inertia_velocity_y_;
}

double GlobalConfig::GetMinimumFlickVelocity() const {
    return min_flick_velocity_;
}

double GlobalConfig::GetMinimumFlickDistancePx() const {
    return min_flick_distance_px_;
}

double GlobalConfig::GetMinimumFlickTimespanSeconds() const {
    return min_flick_timespan_seconds_;
}

void GlobalConfig::SetMinimumFlickVelocity(double v) {
    min_flick_velocity_ = v;
}

void GlobalConfig::SetMinimumFlickDistancePx(double px) {
    min_flick_distance_px_ = px;
}

void GlobalConfig::SetMinimumFlickTimespanSeconds(double seconds){
    min_flick_timespan_seconds_ = seconds;
}

uint16_t GlobalConfig::GetInertiaSpeedMultiplier() const {
    return inertia_speed_multiplier_;
}

void GlobalConfig::SetInertiaSpeedMultiplier(uint16_t multiplier) {
    inertia_speed_multiplier_ = multiplier;
}

bool GlobalConfig::IsInertiaEnabled() const {
    return inertia_enabled_;
}

void GlobalConfig::SetInertiaEnabled(bool enabled) {
    inertia_enabled_ = enabled;
}

double GlobalConfig::GetInertiaFrictionPercentageStart() const {
    return inertia_friction_percentage_start_;
}

double GlobalConfig::GetInertiaFrictionPercentageEnd() const {
    return inertia_friction_percentage_end_;
}

void GlobalConfig::SetInertiaFrictionPercentageStart(double percentage) {
    inertia_friction_percentage_start_ = percentage;
}

void GlobalConfig::SetInertiaFrictionPercentageEnd(double percentage) {
    inertia_friction_percentage_end_ = percentage;
}