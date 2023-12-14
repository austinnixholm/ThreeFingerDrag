#include "touch_processor.h"
#include <future>

namespace Touchpad
{
    TouchProcessor::TouchProcessor()
    {
        touchActivityEvent.AddListener(std::bind(
            &EventListeners::TouchActivityListener::OnTouchActivity,
            &activityListener,
            std::placeholders::_1));
        
        touchUpEvent.AddListener(std::bind(
            &EventListeners::TouchUpListener::OnTouchUp,
            &touchUpListener,
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
            return;

        // Allocate memory for raw input data.
        auto* raw_input = static_cast<RAWINPUT*>(HeapAlloc(hHeap, 0, size));

        if (raw_input == nullptr)
            return;

        // Get raw input data.
        if (GetRawInputData(hRawInputHandle, RID_INPUT, raw_input, &size, sizeof(RAWINPUTHEADER)) == -1)
        {
            HeapFree(hHeap, 0, raw_input);
            return;
        }

        // Get size of pre-parsed data buffer.
        UINT buffer_size;
        GetRawInputDeviceInfo(raw_input->header.hDevice, RIDI_PREPARSEDDATA, nullptr, &buffer_size);

        if (buffer_size == 0)
        {
            HeapFree(hHeap, 0, raw_input);
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
            return;
        }

        // Initialize vector to hold touchpad contact data.
        std::vector<TouchPoint> contacts;
        std::vector<int> ids;

        // Loop through input value caps and retrieve touchpad data.
        ULONG value;
        UINT contact_count = 0;

        TouchPoint contact{INIT_VALUE, INIT_VALUE, INIT_VALUE, false};
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
                if (usage_page == USAGE_PAGE_DIGITIZER_INFO && usage == USAGE_DIGITIZER_CONTACT_COUNT)
                {
                    contact_count = value;
                    if (contact_count == EventListeners::NUM_TOUCH_CONTACTS_REQUIRED)
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
                const ULONG maxNumButtons = HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_DIGITIZER,
                                                                    pre_parsed_data);
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
                            contact.on_surface = true;
                            break;
                        }
                    }

                    free(buttonUsageArray);
                }

                contacts.emplace_back(contact);

                contact = {INIT_VALUE, INIT_VALUE, INIT_VALUE, false};
            }
        }
        // Free allocated memory.
        HeapFree(hHeap, 0, raw_input);
        HeapFree(hHeap, 0, pre_parsed_data);
        HeapFree(hHeap, 0, value_caps);

        // Populate TouchInputData struct and return.
        data.contacts = contacts;
        data.contact_count = contact_count;

        const auto config = GlobalConfig::GetInstance();
        const TouchInputData previous_data_ = config->GetPreviousTouchData();
        
        // Fire touch up event if there are no valid contact points on the touchpad surface on this report
        const bool previousHasContact = TouchPointsMadeContact(previous_data_.contacts);
        const bool hasContact = TouchPointsMadeContact(contacts);
        const auto time = std::chrono::high_resolution_clock::now();

        // Interpret the touch movement into events
        if (!hasContact && previousHasContact)
        {
            touchUpEvent.RaiseEvent(TouchUpEventArgs(time, &data, previous_data_));
            return;
        }
        touchActivityEvent.RaiseEvent(TouchActivityEventArgs(time, &data, previous_data_));
        config->SetLastGesture(time);
        config->SetPreviousTouchData(data);
    }

    /**
     * \returns true if any of the given touch points are contacting the surface of the touchpad.
     */
    bool TouchProcessor::TouchPointsMadeContact(const std::vector<TouchPoint>& points)
    {
        return std::any_of(points.begin(), points.end(), [](TouchPoint p)
        {
            return p.contact_id < CONTACT_ID_MAXIMUM && p.on_surface;
        });
    }
}
