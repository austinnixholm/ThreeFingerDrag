#include "touch_processor.h"
#include <future>
#include <sstream>

namespace Touchpad
{
    TouchProcessor::TouchProcessor()
    {
        config = GlobalConfig::GetInstance();

        touch_activity_event_.AddListener(std::bind(
            &EventListeners::TouchActivityListener::OnTouchActivity,
            &activity_listener_,
            std::placeholders::_1));

        touch_up_event_.AddListener(std::bind(
            &EventListeners::TouchUpListener::OnTouchUp,
            &touch_up_listener_,
            std::placeholders::_1));
    }

    void TouchProcessor::ParseRawTouchData(const LPARAM lParam)
    {
        auto a = std::async(std::launch::async, [&] { InterpretRawInput((HRAWINPUT)lParam); });
    }

    void TouchProcessor::ClearContacts()
    {
        parsed_contacts_.clear();
    }

    /**
     * \brief Retrieves touchpad input data from a raw input handle.
     * \param hRawInputHandle Handle to the raw input.
     * \return A struct containing the touchpad input data.
     */
    void TouchProcessor::InterpretRawInput(const HRAWINPUT hRawInputHandle)
    {
        const bool log_debug = config->LogDebug();

        // Initialize touchpad input data struct with default values.
        TouchInputData data{};

        // Initialize variable to hold size of raw input.
        UINT size = sizeof(RAWINPUT);

        // Get process heap handle.
        const HANDLE hHeap = GetProcessHeap();

        // Get size of raw input data.
        GetRawInputData(hRawInputHandle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));

        if (size == 0)
        {
            if (log_debug)
                DEBUG("No data present.");
            return;
        }

        // Allocate memory for raw input data.
        auto* raw_input = static_cast<RAWINPUT*>(HeapAlloc(hHeap, 0, size));

        if (raw_input == nullptr)
            return;

        // Get raw input data.
        if (GetRawInputData(hRawInputHandle, RID_INPUT, raw_input, &size, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-
            1))
        {
            HeapFree(hHeap, 0, raw_input);
            ERROR("Could not retrieve raw input data from the HID device.");
            return;
        }

        // Get size of pre-parsed data buffer.
        UINT buffer_size;
        GetRawInputDeviceInfo(raw_input->header.hDevice, RIDI_PREPARSEDDATA, nullptr, &buffer_size);

        if (buffer_size == 0)
        {
            HeapFree(hHeap, 0, raw_input);
            ERROR("Could not retrieve pre-parsed data buffer from the HID device.");
            return;
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
            ERROR("Could not retrieve capabilities from the HID device.");
            return;
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
            ERROR("Could not retrieve input value caps from the HID device.");
            return;
        }

        if (log_debug)
            DEBUG("Data Length = " + std::to_string(length));

        // Initialize vector to hold touchpad contact data.
        std::vector<TouchContact> received_contacts;

        // Loop through input value caps and retrieve touchpad data.
        ULONG value;
        TouchContact parsed_contact{INIT_VALUE, INIT_VALUE, INIT_VALUE, false};
        for (USHORT i = 0; i < length; i++)
        {
            auto current_cap = value_caps[i];
            if (HidP_GetUsageValue(
                HidP_Input,
                current_cap.UsagePage,
                current_cap.LinkCollection,
                current_cap.NotRange.Usage,
                &value,
                pre_parsed_data,
                (PCHAR)raw_input->data.hid.bRawData,
                raw_input->data.hid.dwSizeHid
            ) != HIDP_STATUS_SUCCESS)
            {
                continue;
            }

            const USAGE usage_page = current_cap.UsagePage;
            const USAGE usage = current_cap.Range.UsageMin;
            switch (current_cap.LinkCollection)
            {
            default:
                switch (usage_page)
                {
                case HID_USAGE_PAGE_GENERIC:
                    if (current_cap.NotRange.Usage == HID_USAGE_GENERIC_X)
                    {
                        parsed_contact.minimum_x = current_cap.LogicalMin;
                        parsed_contact.maximum_x = current_cap.LogicalMax;
                        parsed_contact.has_x_bounds = true;
                    }
                    else if (current_cap.NotRange.Usage == HID_USAGE_GENERIC_Y)
                    {
                        parsed_contact.minimum_y = current_cap.LogicalMin;
                        parsed_contact.maximum_y = current_cap.LogicalMax;
                        parsed_contact.has_y_bounds = true;
                    }
                    switch (usage)
                    {
                    case USAGE_DIGITIZER_X_COORDINATE:
                        parsed_contact.x = static_cast<int>(value);
                        break;
                    case USAGE_DIGITIZER_Y_COORDINATE:
                        parsed_contact.y = static_cast<int>(value);
                        break;
                    default: break;
                    }
                    break;
                case HID_USAGE_PAGE_DIGITIZER:
                    if (usage == USAGE_DIGITIZER_CONTACT_ID)
                        parsed_contact.contact_id = static_cast<int>(value);
                    break;
                default: break;
                }
                break;
            }

            // If all contact fields are populated, add contact to list and reset fields.
            if (parsed_contact.contact_id != INIT_VALUE && parsed_contact.x != INIT_VALUE && parsed_contact.y !=
                INIT_VALUE)
            {
                const ULONG maxNumButtons =
                    HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_DIGITIZER, pre_parsed_data);
                USAGE* buttonUsageArray = (USAGE*)malloc(sizeof(USAGE) * maxNumButtons);
                ULONG _maxNumButtons = maxNumButtons;

                if (HidP_GetUsages(
                    HidP_Input,
                    HID_USAGE_PAGE_DIGITIZER,
                    current_cap.LinkCollection,
                    buttonUsageArray,
                    &_maxNumButtons,
                    pre_parsed_data,
                    (PCHAR)raw_input->data.hid.bRawData,
                    raw_input->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS)
                {
                    for (ULONG usageIdx = 0; usageIdx < maxNumButtons; usageIdx++)
                    {
                        // Determine if this contact point is on the touchpad surface
                        if (buttonUsageArray[usageIdx] == HID_USAGE_DIGITIZER_TIP_SWITCH)
                        {
                            parsed_contact.on_surface = true;
                            break;
                        }
                    }

                    free(buttonUsageArray);
                }

                received_contacts.emplace_back(parsed_contact);

                parsed_contact = {INIT_VALUE, INIT_VALUE, INIT_VALUE, false};
            }
        }
        // Free allocated memory.
        HeapFree(hHeap, 0, raw_input);
        HeapFree(hHeap, 0, pre_parsed_data);
        HeapFree(hHeap, 0, value_caps);

        const auto interval = EventListeners::CalculateElapsedTimeMs(
            config->GetLastEvent(), std::chrono::high_resolution_clock::now());

        // Clear any old contact data if enough time has passed
        if (interval > config->GetCancellationDelayMs())
            ClearContacts();

        if (log_debug)
        {
            std::ostringstream debug;
            debug << "[RAW REPORTED DATA]\n\n";
            debug << "Interval: " << std::to_string(interval) << "ms\n";
            debug << DebugPoints(received_contacts);
            DEBUG(debug.str());
        }

        UpdateTouchContactsState(received_contacts);
        RaiseEventsIfNeeded();
    }

    void TouchProcessor::UpdateTouchContactsState(const std::vector<TouchContact>& received_contacts)
    {
        std::lock_guard lock(contacts_mutex_);
        std::unordered_map<int, TouchContact> id_to_contact_map;
        for (const auto& contact : parsed_contacts_)
        {
            id_to_contact_map[contact.contact_id] = contact;
        }

        for (const auto& received_contact : received_contacts)
        {
            // Is this a valid contact ID?
            if (received_contact.contact_id > CONTACT_ID_MAXIMUM || received_contact.contact_id < 0)
                continue;
            if (received_contact.x == 0 || received_contact.y == 0)
                continue;
            if (received_contact.has_x_bounds)
            {
                const auto min_x = received_contact.minimum_x;
                const auto max_x = received_contact.maximum_x;
                if (!ValueWithinRange(received_contact.x, min_x, max_x))
                    continue;
            }
            if (received_contact.has_y_bounds)
            {
                const auto min_y = received_contact.minimum_y;
                const auto max_y = received_contact.maximum_y;
                if (!ValueWithinRange(received_contact.y, min_y, max_y))
                    continue;
            }
            id_to_contact_map[received_contact.contact_id] = received_contact;
        }

        parsed_contacts_.clear();
        for (const auto& pair : id_to_contact_map)
        {
            parsed_contacts_.push_back(pair.second);
        }

        std::sort(parsed_contacts_.begin(), parsed_contacts_.end(), [](const TouchContact& a, const TouchContact& b)
        {
            return a.contact_id < b.contact_id;
        });
    }

    void TouchProcessor::RaiseEventsIfNeeded()
    {
        std::lock_guard lock(contacts_mutex_);
        const int current_contact_count = CountTouchPointsMakingContact(parsed_contacts_);
        const bool has_contact = current_contact_count > 0;
        const auto time = std::chrono::high_resolution_clock::now();

        // Construct the TouchInputData object
        TouchInputData touchInputData;
        touchInputData.contacts = parsed_contacts_;
        touchInputData.contact_count = parsed_contacts_.size();
        touchInputData.can_perform_gesture = current_contact_count ==
            EventListeners::NUM_TOUCH_CONTACTS_REQUIRED;

        // Determine if a touch up event should be raised
        const int previous_contact_count = CountTouchPointsMakingContact(config->GetPreviousTouchContacts());
        const bool previous_has_contact = previous_contact_count > 0;
        const bool touch_up_event = !has_contact && previous_has_contact || !has_contact && !previous_has_contact;

        if (touch_up_event)
        {
            touch_up_event_.RaiseEvent(TouchUpEventArgs(time, &touchInputData, config->GetPreviousTouchContacts()));
        }
        else if (has_contact)
        {
            touch_activity_event_.RaiseEvent(
                TouchActivityEventArgs(time, &touchInputData, config->GetPreviousTouchContacts()));
        }

        // Optionally, log the event details for debugging
        if (config->LogDebug())
        {
            LogEventDetails(touch_up_event, time);
        }

        config->SetPreviousTouchContacts(parsed_contacts_);
        config->SetLastEvent(time);

        const auto it = std::remove_if(parsed_contacts_.begin(), parsed_contacts_.end(),
                                       [](const TouchContact& tc) { return !tc.on_surface; });
        parsed_contacts_.erase(it, parsed_contacts_.end());
    }

    void TouchProcessor::LogEventDetails(bool touch_up_event,
                                         const std::chrono::high_resolution_clock::time_point& time) const
    {
        std::stringstream debug;
        debug << "[RAISED EVENT]\n\n";
        debug << "TYPE: ";
        if (touch_up_event)
            debug << "TouchUpEvent";
        else
            debug << "TouchActivityEvent";
        debug << "\n";
        debug << "Interval: " << std::to_string(EventListeners::CalculateElapsedTimeMs(config->GetLastEvent(), time)) <<
            "ms\n";
        debug << DebugPoints(parsed_contacts_);
        DEBUG(debug.str());
    }

    std::string TouchProcessor::DebugPoints(const std::vector<TouchContact>& data)
    {
        std::ostringstream oss;
        oss << "Contacts: (size = " << data.size() << ")\n";
        for (const auto& contact : data)
        {
            oss << "[ID: " << contact.contact_id
                << ", X: " << contact.x
                << ", Y: " << contact.y
                << ", On Surface: " << (contact.on_surface ? "Yes" : "No") << "]";
            if (contact.has_x_bounds)
                oss << ", X Boundary: (min=" << contact.minimum_x << ", max=" << contact.maximum_x << ")";
            if (contact.has_y_bounds)
                oss << ", Y Boundary: (min=" << contact.minimum_y << ", max=" << contact.maximum_y << ")";
            oss << "\n";
        }
        return oss.str();
    }

    /**
     * \returns true if any of the given touch points are contacting the surface of the touchpad.
     */
    bool TouchProcessor::TouchPointsMadeContact(const std::vector<TouchContact>& points)
    {
        return std::any_of(points.begin(), points.end(), [](const TouchContact& p)
        {
            return p.on_surface;
        });
    }

    int TouchProcessor::CountTouchPointsMakingContact(const std::vector<TouchContact>& points)
    {
        return std::count_if(points.begin(), points.end(), [](const TouchContact& p)
        {
            return p.on_surface;
        });
    }


    bool TouchProcessor::ValueWithinRange(const int value, const int minimum, const int maximum)
    {
        return value < maximum && value > minimum;
    }
}
