#include "touch_gestures.h"

#include <future>

namespace
{
	constexpr auto NUM_SKIPPED_FRAMES = 3;
	constexpr auto INACTIVITY_THRESHOLD_MS = 50;
	constexpr auto ACCELERATION_FACTOR = 15.0;

	constexpr auto INIT_VALUE = 65535;
	constexpr auto USAGE_PAGE_DIGITIZER_VALUES = 0x01;
	constexpr auto USAGE_PAGE_DIGITIZER_INFO = 0x0D;
	constexpr auto USAGE_DIGITIZER_SCAN_TIME = 0x56;
	constexpr auto USAGE_DIGITIZER_CONTACT_COUNT = 0x54;
	constexpr auto USAGE_DIGITIZER_CONTACT_ID = 0x51;
	constexpr auto USAGE_DIGITIZER_X_COORDINATE = 0x30;
	constexpr auto USAGE_DIGITIZER_Y_COORDINATE = 0x31;
}

namespace Gestures
{
	void GestureProcessor::ParseRawTouchData(const LPARAM lParam)
	{
		auto a = std::async(std::launch::async, [&]
		{
			const auto hRawInputHandle = (HRAWINPUT)lParam;
			TouchInputData data = RetrieveTouchData(hRawInputHandle);

			// Three finger drag
			if (data.contact_count == 3)
			{
				const std::chrono::time_point<std::chrono::steady_clock> now =
					std::chrono::high_resolution_clock::now();

				PerformGestureMovement(data, now);

				// Update old information to current
				previous_data_ = data;
				last_gesture_ = now;
			}

			// Reset if an ongoing drag gesture was interrupted (ie: finger lifted)
			if (!previous_data_.can_perform_gesture || !is_dragging_ || data.contact_count >= 3)
				return;
			StopDragging();
			data.contacts.clear();
		});
	}

	void GestureProcessor::SimulateClick(const DWORD click_flags) 
	{
		INPUT input;
		input.type = INPUT_MOUSE;
		input.mi.dx = 0;
		input.mi.dy = 0;
		input.mi.mouseData = 0;
		input.mi.dwFlags = click_flags;
		input.mi.time = 0;
		input.mi.dwExtraInfo = 0;
		SendInput(1, &input, sizeof(INPUT));
	}

	void GestureProcessor::MoveCursor(const int delta_x, const int delta_y) const
	{
		INPUT input;
		input.type = INPUT_MOUSE;
		input.mi.dx = delta_x;
		input.mi.dy = delta_y;
		input.mi.mouseData = 0;
		input.mi.dwFlags = MOUSEEVENTF_MOVE;
		input.mi.time = 0;
		input.mi.dwExtraInfo = 0;
		auto a = std::async(std::launch::async, SendInput, 1, &input, sizeof(INPUT));
	}

	/**
	 * \brief Asynchronously calls a left click down mouse click & sets dragging state to true
	 */
	void GestureProcessor::StartDragging()
	{
		auto a1 = std::async(std::launch::async, [this] { SimulateClick(MOUSEEVENTF_LEFTDOWN); });
		is_dragging_ = true;
	}

	/**
	 * \brief Asynchronously calls a left click down mouse click & sets dragging state to true
	 */
	void GestureProcessor::StopDragging()
	{
		auto a = std::async(std::launch::async, [this] { SimulateClick(MOUSEEVENTF_LEFTUP); });
		is_dragging_ = false;
		gesture_frames_skipped_ = 0;
	}

	void GestureProcessor::SetCursorSpeed(const double speed)
	{
		precision_touch_cursor_speed_ = speed;
	}

	bool GestureProcessor::IsDragging() const
	{
		return is_dragging_;
	}

	void GestureProcessor::CheckDragInactivity()
	{
		const std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<float> duration = now - last_gesture_;
		const float ms_since_last = duration.count() * 1000.0f;

		// Sends mouse up event when inactivity occurs
		if (ms_since_last > INACTIVITY_THRESHOLD_MS)
			StopDragging();
	}

	/**
	 * \brief Moves the mouse pointer based on touchpad contacts and elapsed time since the last movement.
	 * \param data			The current touchpad input data.
	 * \param current_time  The current time point.
	 */
	void GestureProcessor::PerformGestureMovement(const TouchInputData& data,
	                                        const std::chrono::time_point<std::chrono::steady_clock>& current_time)
	{
		// Ignore initial frames of movement. This prevents unwanted/overlapped behavior in the beginning of the gesture.
		if (++gesture_frames_skipped_ <= NUM_SKIPPED_FRAMES)
			return;

		// Calculate the time elapsed since the last touchpad contact
		const std::chrono::duration<float> duration = current_time - last_gesture_;
		const float ms_since_last = duration.count() * 1000.0f;

		// Make sure this update is current
		if (ms_since_last >= INACTIVITY_THRESHOLD_MS)
			return;

		// Initialize the change in x and y coordinates of the mouse pointer
		double total_delta_x = 0;
		double total_delta_y = 0;

		if (!previous_data_.contacts.empty())
		{
			// Calculate the movement delta for each finger and add them up, keep track of how many fingers have moved
			int valid_touch_movements = 0;
			for (int i = 0; i < 3; i++)
			{
				const auto& contact = data.contacts[i];
				const auto& previous_contact = previous_data_.contacts[i];

				if (contact.contact_id != previous_contact.contact_id)
					continue;

				// Calculate the movement delta for the current finger
				const double x_diff = contact.x - previous_contact.x;
				const double y_diff = contact.y - previous_contact.y;

				const double delta_x = x_diff + x_diff * precision_touch_cursor_speed_;
				const double delta_y = y_diff + y_diff * precision_touch_cursor_speed_;

				// Check if any movement was present since the last received raw input
				if (std::abs(delta_x) > 0 || std::abs(delta_y) > 0)
					valid_touch_movements++;

				// Add the movement delta for the current finger to the total
				total_delta_x += delta_x;
				total_delta_y += delta_y;
			}

			// If there is more than one finger movement, use the average delta
			if (valid_touch_movements > 1)
			{
				total_delta_x /= static_cast<double>(valid_touch_movements);
				total_delta_y /= static_cast<double>(valid_touch_movements);
			}

			// Apply movement acceleration using a logarithmic function
			const double movement_mag = std::sqrt(total_delta_x * total_delta_x + total_delta_y * total_delta_y);
			const double factor = std::log(movement_mag + 1) / ACCELERATION_FACTOR;

			total_delta_x = std::round(total_delta_x * factor);
			total_delta_y = std::round(total_delta_y * factor);
		}

		// Move the mouse pointer based on the calculated vector
		MoveCursor(static_cast<int>(total_delta_x), static_cast<int>(total_delta_y));

		if (!is_dragging_)
			StartDragging();
	}

	/**
	 * \brief Retrieves touchpad input data from a raw input handle.
	 * \param hRawInputHandle Handle to the raw input.
	 * \return A struct containing the touchpad input data.
	 */
	TouchInputData GestureProcessor::RetrieveTouchData(const HRAWINPUT hRawInputHandle)
	{
		// Initialize touchpad input data struct with default values.
		TouchInputData data{};

		// Initialize variable to hold size of raw input.
		UINT size = sizeof(RAWINPUT);

		// Get process heap handle.
		const HANDLE hHeap = GetProcessHeap();

		// Get size of raw input data.
		GetRawInputData(hRawInputHandle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));

		// If size is 0, return default touchpad input data struct.
		if (size == 0)
			return data;

		// Allocate memory for raw input data.
		auto* raw_input = static_cast<RAWINPUT*>(HeapAlloc(hHeap, 0, size));

		if (raw_input == nullptr)
			return data;

		// Get raw input data.
		if (GetRawInputData(hRawInputHandle, RID_INPUT, raw_input, &size, sizeof(RAWINPUTHEADER)) == -1)
		{
			HeapFree(hHeap, 0, raw_input);
			return data;
		}

		// Get size of pre-parsed data buffer.
		UINT buffer_size;
		GetRawInputDeviceInfo(raw_input->header.hDevice, RIDI_PREPARSEDDATA, nullptr, &buffer_size);

		if (buffer_size == 0)
		{
			HeapFree(hHeap, 0, raw_input);
			return data;
		}

		// Allocate memory for pre-parsed data buffer.
		const auto pre_parsed_data = static_cast<PHIDP_PREPARSED_DATA>(HeapAlloc(hHeap, 0, buffer_size));

		// Get pre-parsed data buffer.
		GetRawInputDeviceInfo(raw_input->header.hDevice, RIDI_PREPARSEDDATA, pre_parsed_data, &buffer_size);

		// Get capabilities of HID device.
		HIDP_CAPS caps;
		if (HidP_GetCaps(pre_parsed_data, &caps) != HIDP_STATUS_SUCCESS)
		{
			HeapFree(hHeap, 0, raw_input);
			HeapFree(hHeap, 0, pre_parsed_data);
			return data;
		}

		// Get number of input value caps.
		USHORT length = caps.NumberInputValueCaps;

		// Allocate memory for input value caps.
		const auto value_caps = static_cast<PHIDP_VALUE_CAPS>(HeapAlloc(hHeap, 0, sizeof(HIDP_VALUE_CAPS) * length));

		// Get input value caps.
		if (HidP_GetValueCaps(HidP_Input, value_caps, &length, pre_parsed_data) != HIDP_STATUS_SUCCESS)
		{
			HeapFree(hHeap, 0, raw_input);
			HeapFree(hHeap, 0, pre_parsed_data);
			HeapFree(hHeap, 0, value_caps);
			return data;
		}

		// Initialize vector to hold touchpad contact data.
		std::vector<TouchPoint> contacts;

		// Loop through input value caps and retrieve touchpad data.
		ULONG value;
		UINT scan_time = 0;
		UINT contact_count = 0;

		TouchPoint contact{INIT_VALUE, INIT_VALUE, INIT_VALUE};
		for (USHORT i = 0; i < length; i++)
		{
			if (HidP_GetUsageValue(
				HidP_Input,
				value_caps[i].UsagePage,
				value_caps[i].LinkCollection,
				value_caps[i].NotRange.Usage,
				&value,
				pre_parsed_data,
				(PCHAR)raw_input->data.hid.bRawData,
				raw_input->data.hid.dwSizeHid
			) != HIDP_STATUS_SUCCESS)
			{
				continue;
			}
			const USAGE usage_page = value_caps[i].UsagePage;
			const USAGE usage = value_caps[i].Range.UsageMin;
			switch (value_caps[i].LinkCollection)
			{
			case 0:
				if (usage_page == USAGE_PAGE_DIGITIZER_INFO && usage == USAGE_DIGITIZER_SCAN_TIME)
					scan_time = value;
				else if (usage_page == USAGE_PAGE_DIGITIZER_INFO && usage == USAGE_DIGITIZER_CONTACT_COUNT)
				{
					contact_count = value;

					// If three fingers are touching the touchpad at any point, allow start of gesture
					if (contact_count == 3)
						data.can_perform_gesture = true;
				}
				break;
			default:
				if (usage_page == USAGE_PAGE_DIGITIZER_INFO && usage == USAGE_DIGITIZER_CONTACT_ID)
					contact.contact_id = static_cast<int>(value);
				else if (usage_page == USAGE_PAGE_DIGITIZER_VALUES && usage == USAGE_DIGITIZER_X_COORDINATE)
					contact.x = static_cast<int>(value);
				else if (usage_page == USAGE_PAGE_DIGITIZER_VALUES && usage == USAGE_DIGITIZER_Y_COORDINATE)
					contact.y = static_cast<int>(value);
				break;
			}

			// If all contact fields are populated, add contact to list and reset fields.
			if (contact.contact_id != INIT_VALUE && contact.x != INIT_VALUE && contact.y != INIT_VALUE)
			{
				contacts.emplace_back(contact);
				contact = {INIT_VALUE, INIT_VALUE, INIT_VALUE};
			}
		}
		// Free allocated memory.
		HeapFree(hHeap, 0, raw_input);
		HeapFree(hHeap, 0, pre_parsed_data);
		HeapFree(hHeap, 0, value_caps);

		// Populate TouchInputData struct and return.
		data.contacts = contacts;
		data.scan_time = scan_time;
		data.contact_count = contact_count;
		return data;
	}
}