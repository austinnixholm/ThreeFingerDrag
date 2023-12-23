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
        std::vector<TouchContact> contacts;
        std::vector<int> ids;

        // Loop through input value caps and retrieve touchpad data.
        ULONG value;
        TouchContact parsed_contact{INIT_VALUE, INIT_VALUE, INIT_VALUE, false};
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
            default:
                if (usage_page == USAGE_PAGE_DIGITIZER_INFO && usage == USAGE_DIGITIZER_CONTACT_ID)
                    parsed_contact.contact_id = static_cast<int>(value);
                else if (usage_page == USAGE_PAGE_DIGITIZER_VALUES && usage == USAGE_DIGITIZER_X_COORDINATE)
                    parsed_contact.x = static_cast<int>(value);
                else if (usage_page == USAGE_PAGE_DIGITIZER_VALUES && usage == USAGE_DIGITIZER_Y_COORDINATE)
                    parsed_contact.y = static_cast<int>(value);
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
                    value_caps[i].LinkCollection,
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

                contacts.emplace_back(parsed_contact);
                ids.push_back(parsed_contact.contact_id);

                parsed_contact = {INIT_VALUE, INIT_VALUE, INIT_VALUE, false};
            }
        }
        // Free allocated memory.
        HeapFree(hHeap, 0, raw_input);
        HeapFree(hHeap, 0, pre_parsed_data);
        HeapFree(hHeap, 0, value_caps);

        std::sort(ids.begin(), ids.end());
        std::vector<TouchContact> sorted_contacts;

        if (log_debug)
        {
            const auto interval = std::to_string(EventListeners::CalculateElapsedTimeMs(
                config->GetLastEvent(), std::chrono::high_resolution_clock::now()));
            std::ostringstream debug;
            debug << "[RAW REPORTED DATA]\n\n";
            debug << "Interval: " << interval << "ms\n";
            debug << DebugPoints(contacts);
            DEBUG(debug.str());
        }

        // Reorder the 'contacts' vector based on the filtered and sorted 'ids' vector
        // This ensures that 'contacts' is sorted by 'contact_id' in ascending order
        // and only includes 'contact_id's that are less than or equal to CONTACT_ID_MAXIMUM
        ids.erase(std::remove_if(ids.begin(), ids.end(), [](const int id)
        {
            return id > CONTACT_ID_MAXIMUM || id <= 0;
        }), ids.end());

        for (const auto& id : ids)
        {
            auto it = std::find_if(contacts.begin(), contacts.end(), [&](const TouchContact& tp)
            {
                return tp.contact_id == id;
            });
            if (it != contacts.end())
                sorted_contacts.push_back(*it);
        }

        data.contacts = sorted_contacts;
        data.contact_count = GetContactCount(sorted_contacts);
        data.can_perform_gesture = data.contact_count == EventListeners::NUM_TOUCH_CONTACTS_REQUIRED;

        for (const TouchContact contact : sorted_contacts)
        {
            for (const TouchContact previous_contact : parsed_contacts_)
            {
                if (previous_contact.contact_id != contact.contact_id)
                    continue;

                data.contacts = parsed_contacts_;

                // Fire touch up event if there are no valid contact points on the touchpad surface on this report
                const bool previous_has_contact = TouchPointsMadeContact(config->GetPreviousTouchContacts());
                const bool has_contact = TouchPointsMadeContact(contacts);
                const auto time = std::chrono::high_resolution_clock::now();
                const bool touch_up_event = !has_contact && previous_has_contact;

                if (log_debug)
                {
                    const auto interval = std::to_string(EventListeners::CalculateElapsedTimeMs(config->GetLastEvent(), time));
                    std::stringstream debug;
                    debug << "[RAISED EVENT]\n\n";
                    debug << "TYPE: ";
                    if (touch_up_event)
                        debug << "TouchUpEvent";
                    else
                        debug << "TouchActivityEvent";
                    debug << "\n";
                    debug << "Interval: " << interval <<
                        "ms\n";
                    debug << DebugPoints(data.contacts);
                    DEBUG(debug.str());
                }
                // Interpret the touch movement into events
                if (touch_up_event)
                    touch_up_event_.RaiseEvent(TouchUpEventArgs(time, &data, config->GetPreviousTouchContacts()));
                else
                    touch_activity_event_.RaiseEvent(
                        TouchActivityEventArgs(time, &data, config->GetPreviousTouchContacts()));

                config->SetLastEvent(time);
                parsed_contacts_.clear();
                break;
            }
            parsed_contacts_.emplace_back(contact);
        }
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
                << ", On Surface: " << (contact.on_surface ? "Yes" : "No") << "]\n";
        }
        return oss.str();
    }


    int TouchProcessor::GetContactCount(const std::vector<TouchContact>& data)
    {
        int count = 0;
        for (const TouchContact point : data)
        {
            if (point.contact_id > CONTACT_ID_MAXIMUM || point.x == 0 || point.y == 0)
                continue;
            count++;
        }
        return count;
    }

    /**
     * \returns true if any of the given touch points are contacting the surface of the touchpad.
     */
    bool TouchProcessor::TouchPointsMadeContact(const std::vector<TouchContact>& points)
    {
        return std::any_of(points.begin(), points.end(), [](TouchContact p)
        {
            return p.contact_id < CONTACT_ID_MAXIMUM && p.on_surface;
        });
    }
}
