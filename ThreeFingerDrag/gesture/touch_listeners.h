#pragma once
#include "../event/touch_events.h"
#include "../mouse/cursor.h"
#include "../notification/popups.h"

namespace GestureListeners {

	constexpr auto NUM_TOUCH_CONTACTS_REQUIRED = 3;
	constexpr auto MIN_VALID_TOUCH_CONTACTS = 1;
	constexpr auto INACTIVITY_THRESHOLD_MS = 50;
	constexpr auto GESTURE_START_THRESHOLD_MS = 20;
	constexpr auto CANCELLATION_TIME_MS = 650;

	static void CancelGesture() {
		GlobalConfig* config = GlobalConfig::GetInstance();
		Cursor::LeftMouseUp();
		config->SetDragging(false);
		config->SetCancellationStarted(false);
	}

	class TouchActivityListener {
	public:
		void OnTouchActivity(TouchActivityEventArgs args) {
			GlobalConfig* config = GlobalConfig::GetInstance();

			// Check if it's the initial gesture
			const bool is_initial_gesture = !config->IsDragging() && args.data->can_perform_gesture;
			const auto now = std::chrono::high_resolution_clock::now();

			// If it's the initial gesture, set the gesture start time
			if (is_initial_gesture && !config->IsGestureStarted()) {
				gesture_start_ = now;
				config->SetGestureStarted(true);
			}

			// If there's no previous data, return
			if (args.previous_data.contacts.empty())
				return;

			// Calculate the time elapsed since the initial touchpad contact
			std::chrono::duration<float> duration = now - gesture_start_;
			const float ms_since_start = duration.count() * 1000.0f;

			// Calculate the time elapsed since previous touchpad data was received
			duration = now - config->GetLastGesture();
			const float ms_since_last_gesture = duration.count() * 1000.0f;

			// Prevents initial jumpy movement due to old data comparison
			if (ms_since_last_gesture > INACTIVITY_THRESHOLD_MS) {
				return;
			}

			// Loop through each touch contact
			int valid_touches = 0;
			for (int i = 0; i < NUM_TOUCH_CONTACTS_REQUIRED; i++)
			{
				const auto& contact = args.data->contacts[i];
				const auto& previous_contact = args.previous_data.contacts[i];

				// Only compare identical touch contact points
				if (contact.contact_id != previous_contact.contact_id)
					continue;

				// Calculate the movement delta for the current finger
				const double x_diff = contact.x - previous_contact.x;
				const double y_diff = contact.y - previous_contact.y;

				// Check if any movement was present since the last received raw input
				if (std::abs(x_diff) > 0 || std::abs(y_diff) > 0)
				{
					// Cancel immediately if a previous cancellation has begun, and this is a non-gesture movement
					if (config->IsCancellationStarted() && !args.data->can_perform_gesture) {
						CancelGesture();
						return;
					}

					// Accumulate the movement delta for the current finger
					accumulated_delta_x_ += x_diff;
					accumulated_delta_y_ += y_diff;
					valid_touches++;
				}
			}

			// If there are not enough valid touches, return
			if (valid_touches < MIN_VALID_TOUCH_CONTACTS)
				return;

			// Centroid calculation
			const double divisor = static_cast<double>(NUM_TOUCH_CONTACTS_REQUIRED);

			accumulated_delta_x_ /= divisor;
			accumulated_delta_y_ /= divisor;

			// Apply movement acceleration using a logarithmic function 
			const double gesture_speed = config->GetGestureSpeed();
			const double movement_mag = std::sqrt(accumulated_delta_x_ * accumulated_delta_x_ + accumulated_delta_y_ * accumulated_delta_y_);
			const double factor = (std::log2(movement_mag + 1)) * (1 + config->GetPrecisionTouchCursorSpeed());

			double total_delta_x = accumulated_delta_x_;
			double total_delta_y = accumulated_delta_y_;

			// Apply the gesture_speed at the end of the calculation
			total_delta_x *= (gesture_speed / 100);
			total_delta_y *= (gesture_speed / 100);


			// Delay initial dragging gesture movement, and ignore invalid movement actions
			if (ms_since_start <= GESTURE_START_THRESHOLD_MS || !(args.data->can_perform_gesture || config->IsDragging())) {
				return;
			}

			config->SetCancellationStarted(false);

			// Move the mouse pointer based on the calculated vector
			Cursor::MoveCursor(total_delta_x, total_delta_y);

			const auto change = std::abs(total_delta_x + total_delta_y);
			
			// Set timestamp for when any cursor movement occurred
			if (change > 0)
				config->SetLastValidMovement(now);

			// Start dragging if left mouse is not already down.
			if (!config->IsDragging()) {
				Cursor::LeftMouseDown();
				config->SetDragging(true);
			}
		}

	private:
		// Accumulated deltas for x and y movements
		double accumulated_delta_x_ = 0;
		double accumulated_delta_y_ = 0;
		// Time point for gesture start
		std::chrono::time_point<std::chrono::steady_clock> gesture_start_;
	};

	class TouchUpListener {
	public:
		void OnTouchUp(TouchUpEventArgs args) {
			GlobalConfig* config = GlobalConfig::GetInstance();
			config->SetGestureStarted(false);
			if (config->IsDragging() && !config->IsCancellationStarted()) {
				// Calculate the time elapsed since the last valid gesture movement
				const auto now = std::chrono::high_resolution_clock::now();
				const std::chrono::duration<float> duration = now - config->GetLastValidMovement();
				const float ms_since_last_movement = duration.count() * 1000.0f;

				// If there hasn't been any movement for same amount of time we will delay, then cancel immediately
				if (ms_since_last_movement >= CANCELLATION_TIME_MS) {
					CancelGesture();
					return;
				}
				config->SetCancellationStarted(true);
				config->SetCancellationTime(now);
			}
		}
	};
}

