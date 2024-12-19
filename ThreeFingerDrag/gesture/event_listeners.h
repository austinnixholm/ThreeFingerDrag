#pragma once
#include "../config/globalconfig.h"
#include "../event/touch_events.h"
#include "../mouse/cursor.h"
#include "../logging/logger.h"
#include <array>
#include <numeric>

namespace EventListeners
{
    constexpr auto NUM_TOUCH_CONTACTS_REQUIRED = 3;
    constexpr auto MAX_CONTACT_SIZE = 10;
    constexpr auto MIN_VALID_TOUCH_CONTACTS = 1;
    constexpr auto INACTIVITY_THRESHOLD_MS = 100;
    constexpr auto GESTURE_START_THRESHOLD_MS = 50;
    constexpr auto FINGER_LIFT_TRANSITION_MS = 50;

    inline GlobalConfig* config = GlobalConfig::GetInstance();
    
    static std::chrono::time_point<std::chrono::steady_clock> finger_transition_start_;
    static bool transition_timestamp_is_current_ = false;

    static void CancelGesture()
    {
        Cursor::LeftMouseUp();
        config->SetCancellationStarted(false);
        config->SetGestureStarted(false);
        transition_timestamp_is_current_ = false;
        if (config->LogDebug())
            DEBUG("Cancelled gesture.");
    }

    static float CalculateElapsedTimeMs(const std::chrono::time_point<std::chrono::steady_clock>& start_time,
                                        const std::chrono::time_point<std::chrono::steady_clock>& end_time)
    {
        const std::chrono::duration<float> elapsed = end_time - start_time;
        return elapsed.count() * 1000.0f;
    }

    class TouchActivityListener
    {
    public:
        void OnTouchActivity(const TouchActivityEventArgs& args)
        {
            config->SetPreviousTouchContacts(args.data->contacts);

            // Check if it's the initial gesture
            const bool is_dragging = Cursor::IsLeftMouseDown();
            const bool is_initial_gesture = !is_dragging && args.data->can_perform_initial_gesture;
            const auto current_time = args.time;

            // If it's the initial gesture, set the gesture start time
            if (is_initial_gesture && !config->IsGestureStarted())
            {
                if (config->IsUsingActivationThreshold())
                {
                    const auto touch_start = config->GetLastInitialActivityTime();
                    const auto elapsed_since_initial_activity = CalculateElapsedTimeMs(touch_start, current_time);

                    if (elapsed_since_initial_activity > config->GetGestureActivationThresholdMs())
                        return;
                }
                config->SetGestureStarted(true);
                gesture_start_ = current_time;
                if (config->LogDebug())
                    DEBUG("Started gesture.");
            }

            // If there's no previous data, return
            if (args.previous_data.empty())
                return;

            // Calculate the time elapsed 
            const float ms_since_gesture_start = CalculateElapsedTimeMs(gesture_start_, current_time);
            const float ms_since_last_event = CalculateElapsedTimeMs(config->GetLastEvent(), current_time);

            // Prevent initial movement jitter 
            if (ms_since_last_event > GESTURE_START_THRESHOLD_MS)
                return;

            // Ignore initial movement
            if (ms_since_gesture_start <= GESTURE_START_THRESHOLD_MS)
                return;

            // If invalid amount of fingers, and gesture is not currently performing
            if (!args.data->can_perform_initial_gesture && !is_dragging)
                return;

            // Loop through each touch contact
            int valid_touches = 0;
            for (int i = 0; i < args.data->contacts.size(); i++)
            {
                if (i > args.previous_data.size() - 1 || i > args.data->contacts.size() - 1 || i > MAX_CONTACT_SIZE)
                    continue;

                const auto& contact = args.data->contacts[i];
                const auto& previous_contact = args.previous_data[i];

                if (!contact.on_surface || !previous_contact.on_surface)
                {
                    // Reset the accumulated movement for this finger
                    accumulated_delta_x_[i] = 0;
                    accumulated_delta_y_[i] = 0;
                    continue;
                }

                // Only compare identical touch contact points
                if (contact.contact_id != previous_contact.contact_id)
                    continue;

                // Ignore initial movement of contact point if inactivity, to prevent jitter
                const float ms_since_movement = CalculateElapsedTimeMs(movement_times_[i], current_time);
                movement_times_[i] = current_time;

                if (ms_since_movement > INACTIVITY_THRESHOLD_MS)
                    continue;

                // Cancel immediately if a previous cancellation has begun, and this is a non-gesture movement
                if (config->IsCancellationStarted() && !args.data->can_perform_initial_gesture && config->
                    IsGestureStarted())
                {
                    CancelGesture();
                    return;
                }

                // Calculate the movement delta for the current finger
                const double x_diff = contact.x - previous_contact.x;
                const double y_diff = contact.y - previous_contact.y;
                const double movement_delta = std::abs(x_diff) + std::abs(y_diff);
                const double accumulated_movement =
                    std::abs(accumulated_delta_x_[i]) + std::abs(accumulated_delta_y_[i]);

                const bool include_fine_movement = accumulated_movement >= 1.0 && movement_delta > 0;

                // Check if any valid movement was present
                if (!include_fine_movement && movement_delta < 1.0)
                    continue;

                // Accumulate the movement delta for the current finger
                accumulated_delta_x_[i] += x_diff;
                accumulated_delta_y_[i] += y_diff;
                valid_touches++;
            }
            
            if (config->DragCancelsOnFingerCountChange())
            {
                // Gesture is started, but finger count has changed
                if (!args.data->can_perform_initial_gesture && config->IsGestureStarted() && is_dragging)
                {
                    // Set timestamp for when finger lift occurred
                    if (!transition_timestamp_is_current_)
                    {
                        finger_transition_start_ = current_time;
                        transition_timestamp_is_current_ = true;
                    }

                    // Only start cancellation if the finger has been lifted for longer than ~50ms
                    const auto elapsed = CalculateElapsedTimeMs(finger_transition_start_, current_time);
                    if (elapsed > FINGER_LIFT_TRANSITION_MS)
                    {
                        config->SetCancellationStarted(true);
                        return;
                    }
                } else if (transition_timestamp_is_current_ && args.data->can_perform_initial_gesture && config->IsGestureStarted() && is_dragging)
                {
                    // Fingers were put back down during transition time. Mark to reset the timestamp if checked again
                    transition_timestamp_is_current_ = false;
                }
            }

            const auto contact_count = args.data->contact_count;

            // Switched to one finger during gesture
            if (contact_count == 1 && config->GetLastContactCount() == 1)
            {
                const float ms_since_last_switch = CalculateElapsedTimeMs(
                    config->GetLastOneFingerSwitchTime(), current_time);

                // After a short delay, stop continuing the gesture movement from this event in favor of
                // default touchpad cursor movement to prevent input flooding.
                if (ms_since_last_switch > config->GetOneFingerTransitionDelayMs())
                {
                    accumulated_delta_x_ = {0.0};
                    accumulated_delta_y_ = {0.0};
                    return;
                }
            }

            if (config->IsGestureStarted() && config->GetLastContactCount() > 1 && contact_count == 1)
                config->SetLastOneFingerSwitchTime(current_time);

            config->SetLastContactCount(contact_count);

            // If there are not enough valid touches, return
            if (valid_touches < MIN_VALID_TOUCH_CONTACTS)
                return;

            // Apply movement acceleration 
            const double gesture_speed = config->GetGestureSpeed() / 100.0;

            const double delta_x =
                std::accumulate(accumulated_delta_x_.begin(), accumulated_delta_x_.end(), 0.0) * gesture_speed;
            const double delta_y =
                std::accumulate(accumulated_delta_y_.begin(), accumulated_delta_y_.end(), 0.0) * gesture_speed;

            config->SetCancellationStarted(false);

            // Move the mouse pointer based on the calculated vector
            Cursor::MoveCursor(delta_x, delta_y);

            // Start dragging if left mouse is not already down
            if (!is_dragging)
                Cursor::LeftMouseDown();

            const auto change_x = std::abs(delta_x);
            const auto change_y = std::abs(delta_y);
            const auto total_change = change_x + change_y;

            // Check if any movement occurred
            if (total_change < 1.0)
                return;

            // Set timestamp for last valid movement
            config->SetLastValidMovement(current_time);

            // Reset accumulated x/y data if necessary 
            if (change_x >= 1.0)
                accumulated_delta_x_.fill(0);
            if (change_y >= 1.0)
                accumulated_delta_y_.fill(0);
        }

    private:
        std::array<double, MAX_CONTACT_SIZE> accumulated_delta_x_ = {0.0};
        std::array<double, MAX_CONTACT_SIZE> accumulated_delta_y_ = {0.0};
        std::array<std::chrono::time_point<std::chrono::steady_clock>, MAX_CONTACT_SIZE> movement_times_;
        std::chrono::time_point<std::chrono::steady_clock> gesture_start_;

    };

    class TouchUpListener
    {
    public:
        void OnTouchUp(const TouchUpEventArgs& args)
        {
            config->SetPreviousTouchContacts(args.data->contacts);

            if (!config->IsGestureStarted())
                return;

            // Calculate the time elapsed since the last valid gesture movement
            const auto current_time = std::chrono::high_resolution_clock::now();
            const float ms_since_last_movement = CalculateElapsedTimeMs(config->GetLastValidMovement(), current_time);

            // If there hasn't been any movement for same amount of time we will delay, then cancel immediately
            if (ms_since_last_movement >= static_cast<float>(config->GetCancellationDelayMs()))
            {
                CancelGesture();
                return;
            }

            config->SetCancellationStarted(true);
            config->SetCancellationTime(current_time);
            config->SetLastValidMovement(current_time);

            if (config->LogDebug())
                DEBUG("Started gesture cancellation.");
        }
    };
}
