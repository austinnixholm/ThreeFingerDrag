#pragma once
#include "framework.h"
#include "touch_listeners.h"
#include <mutex>
#include <vector>
#include <chrono>

namespace Gestures
{
	constexpr auto NUM_TOUCH_CONTACTS_REQUIRED = 3;
	constexpr auto MIN_VALID_TOUCH_CONTACTS = 1;
	constexpr auto INACTIVITY_THRESHOLD_MS = 50;

	constexpr auto INIT_VALUE = 65535;
	constexpr auto CONTACT_ID_MAXIMUM = 64;
	constexpr auto USAGE_PAGE_DIGITIZER_VALUES = 0x01;
	constexpr auto USAGE_PAGE_DIGITIZER_INFO = 0x0D;
	constexpr auto USAGE_DIGITIZER_SCAN_TIME = 0x56;
	constexpr auto USAGE_DIGITIZER_CONTACT_COUNT = 0x54;
	constexpr auto USAGE_DIGITIZER_CONTACT_ID = 0x51;
	constexpr auto USAGE_DIGITIZER_X_COORDINATE = 0x30;
	constexpr auto USAGE_DIGITIZER_Y_COORDINATE = 0x31;


	/**
	 * \brief Class that processes touch input data to enable three-finger drag functionality.
	 */
	class GestureProcessor
	{
	public:
		GestureProcessor();

		/**
		 * @brief Parses raw touch data received by the touch pad.
		 * @param lParam LPARAM containing the touch data.
		 */
		void ParseRawTouchData(LPARAM lParam);

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
		void InterpretRawInput(HRAWINPUT hRawInputHandle);

		bool TouchPointsAreValid(const std::vector<TouchPoint>& points);
		
		GestureListeners::TouchActivityListener activityListener;
		GestureListeners::TouchUpListener touchUpListener;

		Event<TouchActivityEventArgs> touchActivityEvent;
		Event<TouchUpEventArgs> touchUpEvent;

		TouchInputData previous_data_; 
		std::chrono::time_point<std::chrono::steady_clock> last_gesture_;

		int gesture_frames_skipped_ = 0;
		int skipped_frame_amount_ = 3;
		double gesture_speed_ = 25.0;
		double precision_touch_cursor_speed_ = 0.5;
		double mouse_cursor_speed_ = 0.5;
		double accumulated_delta_x_ = 0;
		double accumulated_delta_y_ = 0;
		bool is_dragging_ = false;
	};


}
