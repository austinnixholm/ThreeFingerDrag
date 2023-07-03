#pragma once
#include "framework.h"
#include <mutex>
#include <vector>
#include <chrono>

namespace Gestures
{
	struct TouchPoint
	{
		int contact_id;
		int x;
		int y;
	};

	struct TouchInputData
	{
		std::vector<TouchPoint> contacts;
		int scan_time = 0;
		int contact_count = 0;
		bool can_perform_gesture = false;
	};

	/**
	 * \brief Class that processes touch input data to enable three-finger drag functionality.
	 */
	class GestureProcessor
	{
	public:
		GestureProcessor() = default;

		/**
		 * @brief Parses raw touch data received by the touch pad.
		 * @param lParam LPARAM containing the touch data.
		 */
		void ParseRawTouchData(LPARAM lParam);

		/**
		 * @brief Function to set the speed of the windows touchpad cursor speed.
		 * @param speed The new speed to set for the touchpad.
		 */
		void SetTouchSpeed(double speed);

		/**
		 * @brief Function to set the speed of the windows mouse cursor speed.
		 * @param speed The new speed to set for the cursor.
		 */
		void SetMouseSpeed(double speed);

		/**
		 * \brief Checks if there has been no drag activity for a while.
		 * \remarks Stops dragging if there has been any detected inactivity.
		 */
		void CheckDragInactivity();

		/**
		 * \brief Checks if dragging is currently in progress.
		 * \return True if dragging is in progress, false otherwise.
		 */
		bool IsDragging() const;

		GestureProcessor(const GestureProcessor& other) = delete; // Disallow copy constructor
		GestureProcessor(GestureProcessor&& other) noexcept = delete; // Disallow move constructor
		GestureProcessor& operator=(const GestureProcessor& other) = delete; // Disallow copy assignment
		GestureProcessor& operator=(GestureProcessor&& other) noexcept = delete; // Disallow move assignment
		~GestureProcessor() = default; // Default destructor

	private:
		/**
		 * @brief Retrieves touch data from the given raw input handle.
		 * @param hRawInputHandle Handle to the raw input data.
		 * @return The retrieved touch data.
		 */
		static TouchInputData RetrieveTouchData(HRAWINPUT hRawInputHandle);

		/**
		 * \brief Starts the dragging process, performs left mouse down.
		 */
		void StartDragging();

		/**
		 * \brief Stops the dragging process, performs left mouse up.
		 */
		void StopDragging();

		/**
		 * \brief Simulates a windows click input event.
		 * \param flags Input flags type of mouse click.
		 */
		void SimulateClick(DWORD click_flags);

		/**
		 * \brief Moves the mouse pointer based on the given touch input data.
		 * \param delta_x Amount to move the cursor x position.
		 * \param delta_y Amount to move the cursor y position.
		 */
		void MoveCursor(int delta_x, int delta_y) const;

		/**
		 * \brief Moves the mouse pointer based on the given touch input data.
		 * \param data		   The latest input data.
		 * \param current_time The current time point.
		 */
		void PerformGestureMovement(const TouchInputData& data,
		                            const std::chrono::time_point<std::chrono::steady_clock>& current_time);

		TouchInputData previous_data_; 
		std::chrono::time_point<std::chrono::steady_clock> last_gesture_;

		int gesture_frames_skipped_ = 0;
		double precision_touch_cursor_speed_ = 0.5;
		double mouse_cursor_speed_ = 0.5;
		double accumulated_delta_x_ = 0;
		double accumulated_delta_y_ = 0;
		bool is_dragging_ = false;
	};
}
