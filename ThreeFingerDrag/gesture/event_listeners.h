#pragma once
#include "../config/globalconfig.h"
#include "../event/touch_events.h"
#include "../mouse/cursor.h"
#include "../logging/logger.h"
#include <array>
#include <numeric>
#include <deque> 

namespace EventListeners
{
    constexpr auto NUM_TOUCH_CONTACTS_REQUIRED = 3;
    constexpr auto MAX_CONTACT_SIZE = 10;
    constexpr auto MIN_VALID_TOUCH_CONTACTS = 1;
    constexpr auto INACTIVITY_THRESHOLD_MS = 100;
    constexpr auto GESTURE_START_THRESHOLD_MS = 50;
    // TODO: ini settings
    constexpr double MIN_FLICK_VELOCITY = 300.0;   // pixels/second
    constexpr double MIN_FLICK_DISTANCE = 50.0;    // pixels
    constexpr double MAX_FLICK_TIMESPAN = 0.2;    // seconds (150ms)

    inline GlobalConfig* config = GlobalConfig::GetInstance();

    struct FingerHistory {
        std::deque<std::pair<double, double>> deltas;  // Stores (dx, dy)
        std::deque<std::chrono::time_point<std::chrono::steady_clock>> timestamps;
    };


    static void CancelGesture()
    {
        Cursor::LeftMouseUp();
        config->SetCancellationStarted(false);
        config->SetGestureStarted(false);
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
            const bool is_initial_gesture = !is_dragging && args.data->can_perform_gesture;
            const auto current_time = args.time;
            const int last_contact_count = config->GetLastContactCount();
            const auto contact_count = args.data->contact_count;

            // If it's the initial gesture, set the gesture start time
            if (is_initial_gesture && !config->IsGestureStarted())
            {
                config->SetGestureStarted(true);
                gesture_start_ = current_time;
                finger_history_.clear();
                if (config->LogDebug())
                    DEBUG("Started gesture.");
            }

            if (config->IsInertiaActive() && contact_count > last_contact_count) {
                config->StopInertia();
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
            if (!args.data->can_perform_gesture && !is_dragging)
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

                // Calculate delta for this frame
                double dx = contact.x - previous_contact.x;
                double dy = contact.y - previous_contact.y;

                finger_history_[contact.contact_id].deltas.push_back({ dx, dy });
                finger_history_[contact.contact_id].timestamps.push_back(current_time);

                if (finger_history_[contact.contact_id].deltas.size() > 6) {
                    finger_history_[contact.contact_id].deltas.pop_front();
                    finger_history_[contact.contact_id].timestamps.pop_front();
                }

                // Ignore initial movement of contact point if inactivity, to prevent jitter
                const float ms_since_movement = CalculateElapsedTimeMs(movement_times_[i], current_time);
                movement_times_[i] = current_time;

                if (ms_since_movement > INACTIVITY_THRESHOLD_MS)
                    continue;

                // Cancel immediately if a previous cancellation has begun, and this is a non-gesture movement
                if (config->IsCancellationStarted() && !args.data->can_perform_gesture && config->IsGestureStarted())
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


            // Switched to one finger during gesture
            if (contact_count == 1 && config->GetLastContactCount() == 1)
            {
                const float ms_since_last_switch = CalculateElapsedTimeMs(config->GetLastOneFingerSwitchTime(), current_time);
                
                // After a short delay, stop continuing the gesture movement from this event in favor of
                // default touchpad cursor movement to prevent input flooding.
                if (ms_since_last_switch > config->GetOneFingerTransitionDelayMs())
                {
                    accumulated_delta_x_ = {0.0};
                    accumulated_delta_y_ = {0.0};
                    return;
                }
            }

            if (config->IsGestureStarted() && last_contact_count > 1 && contact_count == 1)
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

            bool inertia_started = false;

            // Inertial momentum
            if (last_contact_count == 3 && contact_count == 2) {
                // Identify the released finger
                int released_id = -1;

                // Compare previous and current contacts to find the missing finger
                for (const auto& prev_contact : args.previous_data) {
                    bool exists = false;
                    for (const auto& curr_contact : args.data->contacts) {
                        if (prev_contact.contact_id == curr_contact.contact_id) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        released_id = prev_contact.contact_id;
                        break;
                    }
                }

                // If the released finger is found and has movement history
                if (released_id != -1 && finger_history_.count(released_id)) {
                    const auto& history = finger_history_[released_id];
                
                    // Calculate time span in seconds with maximal precision
                    const auto duration = history.timestamps.back() - history.timestamps.front();
                    const double time_span = std::chrono::duration<double>(duration).count(); // Precision to nanoseconds
                
                    double total_dx = 0.0, total_dy = 0.0;
                    double total_weight = 0.0;
                    const auto now = std::chrono::steady_clock::now();
                
                    for (size_t i = 0; i < history.deltas.size(); i++) {
                        const auto age = std::chrono::duration<double>(now - history.timestamps[i]).count();
                        const double weight = std::exp(-age * 18.5); // TODO: ini setting
                        total_dx += history.deltas[i].first * weight;
                        total_dy += history.deltas[i].second * weight;
                        total_weight += weight;
                    }
                
                    if (total_weight > 0 && time_span > 0) {
                        // Calculate actual physical movement (not just velocity)
                        const double total_distance = std::hypot(total_dx, total_dy);

                        // Calculate velocity
                        double velocity_x = (total_dx / total_weight) / time_span;
                        double velocity_y = (total_dy / total_weight) / time_span;
                        const double speed = std::hypot(velocity_x, velocity_y);

                        bool speed_condition_met = speed >= MIN_FLICK_VELOCITY;
                        bool distance_condition_met = total_distance >= MIN_FLICK_DISTANCE;
                        bool time_span_condition_met = time_span <= MAX_FLICK_TIMESPAN;

                        bool conditions_met = speed_condition_met && distance_condition_met && time_span_condition_met;

                        // Only trigger inertia if all conditions met
                        if (conditions_met) {
                            const double amplification = 0.030; // TODO: ini setting
                            const double initial_vx = velocity_x * amplification;
                            const double initial_vy = velocity_y * amplification;

                            config->StartInertia(initial_vx, initial_vy);
                            Cursor::MoveCursor(initial_vx, initial_vy);
                            inertia_started = true;
                        }
                        if (!conditions_met) {
                            OutputDebugString(L"==== Inertial Momentum Conditions Not Met ====\n");
                        }
                        if (!speed_condition_met) {
                            OutputDebugString(L"Speed condition not met.\n");
                        }
                        if (!distance_condition_met) {
                            OutputDebugString(L"Distance condition not met.\n");
                        }
                        if (!time_span_condition_met) {
                            OutputDebugString(L"Time Span condition not met.\n");
                        }
                    }
                    

                    // Clear history for the released finger
                    finger_history_.erase(released_id);
                }
                else if (released_id == -1) {
                    OutputDebugString(L"no released id\n");
                }
                else {
                    OutputDebugString(L"hi\n");
                }
            }

            config->SetCancellationStarted(false);

            // Move the mouse pointer based on the calculated vector
            if (!inertia_started)
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
        std::unordered_map<int, FingerHistory> finger_history_;
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

            if (config->IsInertiaActive()) {
                config->StopInertia();
            }

            if (config->IsCancellationStarted() || !config->IsGestureStarted())
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
