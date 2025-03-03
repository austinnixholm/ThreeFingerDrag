#pragma once
#include "../framework.h"
#include "event_listeners.h"
#include <vector>
#include <mutex>

namespace Touchpad
{
    constexpr auto INIT_VALUE = 65535;
    constexpr auto CONTACT_ID_MAXIMUM = 64;
    constexpr auto CONTACT_ID_MINIMUM = 0;
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
    class TouchProcessor
    {
    public:
        TouchProcessor();

        /**
         * @brief Retrieves touch data from the given raw input handle.
         * @param hRawInputHandle Handle to the raw input data.
         * @return The retrieved touch data.
         */
        void ProcessRawInput(HRAWINPUT hRawInputHandle);
        void ClearContacts();

        TouchProcessor(const TouchProcessor& other) = delete; // Disallow copy constructor
        TouchProcessor(TouchProcessor&& other) noexcept = delete; // Disallow move constructor
        TouchProcessor& operator=(const TouchProcessor& other) = delete; // Disallow copy assignment
        TouchProcessor& operator=(TouchProcessor&& other) noexcept = delete; // Disallow move assignment
        ~TouchProcessor() = default; // Default destructor

    private:
        void UpdateTouchContactsState(const std::vector<TouchContact>& received_contacts);
        void RaiseEventsIfNeeded();
        void LogEventDetails(bool touch_up_event, const std::chrono::high_resolution_clock::time_point& time) const;
        
        static bool ValueWithinRange(int value, int minimum, int maximum);
        static std::string DebugPoints(const std::vector<TouchContact>& data);
        static int CountTouchPointsMakingContact(const std::vector<TouchContact>& points);

        EventListeners::TouchActivityListener activity_listener_;
        EventListeners::TouchUpListener touch_up_listener_;

        Event<TouchActivityEventArgs> touch_activity_event_;
        Event<TouchUpEventArgs> touch_up_event_;
        std::vector<TouchContact> parsed_contacts_;
        mutable std::mutex contacts_mutex_;

        GlobalConfig* config;
    };
}
