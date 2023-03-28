#include <iostream>
#include <Windows.h>
#include "framework.h"
#include "ThreeFingerDrag.h"

// Constants

constexpr auto kStartupProgramRegistryKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr auto kProgramName = L"ThreeFingerDrag";
constexpr auto kUpdateSettingsPeriodMs = std::chrono::milliseconds(3000);
constexpr auto kTouchActivityPeriodMs = std::chrono::milliseconds(75);
constexpr auto kInactivityThresholdMs = 50;
constexpr auto kNumSkippedFrames = 3;
constexpr auto kTouchLogFactor = 15.0;
constexpr auto kMaxLoadString = 100;
constexpr auto kInitValue = 65535;
constexpr auto kDigitizerValueUsagePage = 0x01;
constexpr auto kDigitizerInfoUsagePage = 0x0D;
constexpr auto kDigitizerUsageScanTime = 0x56;
constexpr auto kDigitizerUsageContactCount = 0x54;
constexpr auto kDigitizerUsageContactId = 0x51;
constexpr auto kDigitizerUsageX = 0x30;
constexpr auto kDigitizerUsageY = 0x31;
constexpr auto kQuitMenuItemId = 1;
constexpr auto kCreateTaskMenuItemId = 2;
constexpr auto kRemoveTaskMenuItemId = 3;

// Global Variables

HINSTANCE hInst; // Current instance
HWND hWnd; // Tool window handle
WCHAR szTitle[kMaxLoadString]; // The title bar text
WCHAR szWindowClass[kMaxLoadString]; // The main window class name
NOTIFYICONDATA nid; // Tray icon
DOUBLE precision_touch_cursor_speed; // Precision touch pad cursor speed
BOOL is_dragging = false; // True if a touch drag action has not yet been cancelled
BOOL run_loops = true;
INT gesture_frames_skipped = 0;
std::mutex mutex;
std::thread update_settings_thread;
std::thread touch_activity_thread;
std::chrono::time_point<std::chrono::steady_clock> last_detection; // Last time gesture was detected
TouchPadInputData previous_data; // Last detected touch pad data
Logger logger("ThreeFingerDrag");

// Forward declarations

ATOM RegisterWindowClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT HandleRawTouchInput(LPARAM lParam);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DOUBLE ClampDouble(double in, double min, double max);
TouchPadInputData GetTouchData(HRAWINPUT hRawInputHandle);
bool StartupRegistryKeyExists();
void DisplayErrorMessage(const std::string& message);
void CreateTrayMenu(HWND hWnd);
void RemoveStartupRegistryKey();
void AddStartupRegistryKey();
void ReadPrecisionTouchPadInfo();
void StartUpdateThread();
void StartActivityThread();
void SimulateClick(DWORD flags);
void MoveMousePointer(const TouchPadInputData& data, const std::chrono::time_point<std::chrono::steady_clock>& now);
void StartDragging();
void StopDragging();
void HandleUncaughtExceptions();


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	std::set_terminate(HandleUncaughtExceptions);

	// Single application instance check
	const HANDLE hMutex = CreateMutex(nullptr, TRUE, kProgramName);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBox(nullptr, TEXT("Another instance of Three Finger Drag is already running."), TEXT("Error"),
		           MB_OK | MB_ICONERROR);
		CloseHandle(hMutex);
		return FALSE;
	}

	// Read any user values from Windows precision touch pad settings from registry
	ReadPrecisionTouchPadInfo();

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, kMaxLoadString);
	LoadStringW(hInstance, IDC_THREEFINGERDRAG, szWindowClass, kMaxLoadString);
	RegisterWindowClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		logger.Error("Application initialization failed.");
		return FALSE;
	}

	// Start threads
	StartUpdateThread();
	StartActivityThread();

	MSG msg;

	// Enter message loop and process incoming messages until WM_QUIT is received
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Delete tray icon
	Shell_NotifyIcon(NIM_DELETE, &nid);

	// Join threads
	run_loops = false;
	update_settings_thread.join();
	touch_activity_thread.join();

	return static_cast<int>(msg.wParam);
}

ATOM RegisterWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = 0;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THREEFINGERDRAG));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = nullptr;
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THREEFINGERDRAG));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(const HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, szWindowClass, szTitle, 0,
	                      0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd)
	{
		logger.Error("Window handle could not be created!");
		return FALSE;
	}

	// Register raw input touch device

	RAWINPUTDEVICE rid;

	rid.usUsagePage = HID_USAGE_PAGE_DIGITIZER;
	rid.usUsage = HID_USAGE_DIGITIZER_TOUCH_PAD;
	rid.dwFlags = RIDEV_INPUTSINK; // Receive input even when the application is in the background
	rid.hwndTarget = hWnd; // Handle to the application window

	const auto message = std::string();
	if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
	{
		DisplayErrorMessage("Raw touch device couldn't be registered. Program will now exit.");
		return FALSE;
	}

	// Initialize notify icon data
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_USER + 1;
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THREEFINGERDRAG));

	std::string title = "Three Finger Drag ";
	title.append(GetVersionString());
	const std::wstring w_title(title.begin(), title.end());
	lstrcpy(nid.szTip, w_title.c_str());

	Shell_NotifyIcon(NIM_ADD, &nid);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		{
			SendMessage(hWnd, WM_SETICON, ICON_BIG, LPARAM(LoadIcon(hInst, MAKEINTRESOURCE(IDI_THREEFINGERDRAG))));
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, LPARAM(LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL))));
		}
		break;

	case WM_INPUT:
		{
			auto a = std::async(std::launch::async, HandleRawTouchInput, lParam);
		}
		return 0;

	// Notify Icon
	case WM_USER + 1:
		switch (lParam)
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			CreateTrayMenu(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_COMMAND:
		{
			// Parse the menu selections
			switch (LOWORD(wParam))
			{
			case kQuitMenuItemId:
				DestroyWindow(hWnd);
				break;
			case kCreateTaskMenuItemId:
				AddStartupRegistryKey();
				break;
			case kRemoveTaskMenuItemId:
				RemoveStartupRegistryKey();
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


/**
 * \brief Handles raw touch input and performs actions based on the touch data.
 *
 * This function processes raw touch input from a touchpad and performs actions
 * based on the touch data. Specifically, it handles three-finger drag gestures
 * by moving the mouse pointer and updates the previous touch data and detection
 * time. If an ongoing drag gesture is interrupted (i.e. a finger is lifted),
 * the function resets the touch data and stops the drag gesture.
 *
 * \param lParam The lParam value of the raw input message.
 * \return Always returns 0 to indicate success.
 */
LRESULT HandleRawTouchInput(const LPARAM lParam)
{
	const auto hRawInputHandle = (HRAWINPUT)lParam;
	TouchPadInputData data = GetTouchData(hRawInputHandle);

	// Three finger drag
	if (data.contact_count == 3)
	{
		const std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::high_resolution_clock::now();

		MoveMousePointer(data, now);

		// Update old information to current
		previous_data = data;
		last_detection = now;
	}

	// Reset if an ongoing drag gesture was interrupted (ie: finger lifted)
	if (previous_data.can_perform_gesture && is_dragging && data.contact_count < 3)
	{
		StopDragging();
		data.contacts.clear();
	}
	return 0;
}

/**
 * \brief Starts a thread that periodically updates setting info.
 */
void StartUpdateThread()
{
	update_settings_thread = std::thread([&]
	{
		while (run_loops)
		{
			std::this_thread::sleep_for(kUpdateSettingsPeriodMs);
			ReadPrecisionTouchPadInfo();
		}
	});
}

/**
 * \brief Starts a thread that periodically checks 
 */
void StartActivityThread()
{
	touch_activity_thread = std::thread([&]
	{
		while (run_loops)
		{
			std::this_thread::sleep_for(kTouchActivityPeriodMs);
			if (!is_dragging)
				continue;

			const std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::high_resolution_clock::now();
			const std::chrono::duration<float> duration = now - last_detection;
			const float ms_since_last = duration.count() * 1000.0f;

			// Sends mouse up event when inactivity occurs
			if (ms_since_last > kInactivityThresholdMs)
				StopDragging();
		}
	});
}

/**
 * \brief A helper function to simulate a mouse click event. 
 * \param flags The mouse input flags
 */
void SimulateClick(const DWORD flags)
{
	std::lock_guard lk(mutex);
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.dx = 0;
	input.mi.dy = 0;
	input.mi.mouseData = 0;
	input.mi.dwFlags = flags;
	input.mi.time = 0;
	input.mi.dwExtraInfo = 0;
	SendInput(1, &input, sizeof(INPUT));
}

/**
 * \brief A helper function to move the mouse pointer based on the change in x and y coordinates.
 * \param delta_x The change in the x-coordinate of the mouse pointer.
 * \param delta_y The change in the y-coordinate of the mouse pointer.
 */
void MoveCursor(const int delta_x, const int delta_y)
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
 * \brief Moves the mouse pointer based on touchpad contacts and elapsed time since the last movement.
 * \param data The current touchpad input data.
 * \param now The current time point.
 */
void MoveMousePointer(const TouchPadInputData& data,
                      const std::chrono::time_point<std::chrono::steady_clock>& now)
{
	// Ignore initial frames of movement. This prevents unwanted/overlapped behavior in the beginning of the gesture.
	if (++gesture_frames_skipped <= kNumSkippedFrames)
		return;

	// Calculate the time elapsed since the last touchpad contact
	const std::chrono::duration<float> duration = now - last_detection;
	const float ms_since_last = duration.count() * 1000.0f;

	// Make sure this update is current
	if (ms_since_last >= kInactivityThresholdMs)
		return;

	// Initialize the change in x and y coordinates of the mouse pointer
	double total_delta_x = 0;
	double total_delta_y = 0;

	if (!previous_data.contacts.empty())
	{
		// Calculate the movement delta for each finger and add them up, keep track of how many fingers have moved
		int valid_touch_movements = 0;
		for (int i = 0; i < 3; i++)
		{
			const auto& contact = data.contacts[i];
			const auto& previous_contact = previous_data.contacts[i];

			if (contact.contact_id != previous_contact.contact_id)
				continue;

			// Calculate the movement delta for the current finger
			const double x_diff = contact.x - previous_contact.x;
			const double y_diff = contact.y - previous_contact.y;

			const double delta_x = x_diff + x_diff * precision_touch_cursor_speed;
			const double delta_y = y_diff + y_diff * precision_touch_cursor_speed;

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
		const double factor = std::log(movement_mag + 1) / kTouchLogFactor;

		total_delta_x = std::round(total_delta_x * factor);
		total_delta_y = std::round(total_delta_y * factor);
	}

	// Move the mouse pointer based on the calculated vector
	MoveCursor(static_cast<int>(total_delta_x), static_cast<int>(total_delta_y));

	if (!is_dragging)
		StartDragging();
}

/**
 * \brief Retrieves touchpad input data from a raw input handle.
 * \param hRawInputHandle Handle to the raw input. 
 * \return A struct containing the touchpad input data.
 */
TouchPadInputData GetTouchData(const HRAWINPUT hRawInputHandle)
{
	// Initialize touchpad input data struct with default values.
	TouchPadInputData data{};

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
	std::vector<TouchPadContact> contacts;

	// Loop through input value caps and retrieve touchpad data.
	ULONG value;
	UINT scan_time = 0;
	UINT contact_count = 0;

	TouchPadContact contact{kInitValue, kInitValue, kInitValue};
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
			if (usage_page == kDigitizerInfoUsagePage && usage == kDigitizerUsageScanTime)
				scan_time = value;
			else if (usage_page == kDigitizerInfoUsagePage && usage == kDigitizerUsageContactCount)
			{
				contact_count = value;

				// If three fingers are touching the touchpad at any point, allow start of gesture
				if (contact_count == 3)
					data.can_perform_gesture = true;
			}
			break;
		default:
			if (usage_page == kDigitizerInfoUsagePage && usage == kDigitizerUsageContactId)
				contact.contact_id = static_cast<int>(value);
			else if (usage_page == kDigitizerValueUsagePage && usage == kDigitizerUsageX)
				contact.x = static_cast<int>(value);
			else if (usage_page == kDigitizerValueUsagePage && usage == kDigitizerUsageY)
				contact.y = static_cast<int>(value);
			break;
		}

		// If all contact fields are populated, add contact to list and reset fields.
		if (contact.contact_id != kInitValue && contact.x != kInitValue && contact.y != kInitValue)
		{
			contacts.emplace_back(contact);
			contact = {kInitValue, kInitValue, kInitValue};
		}
	}
	// Free allocated memory.
	HeapFree(hHeap, 0, raw_input);
	HeapFree(hHeap, 0, pre_parsed_data);
	HeapFree(hHeap, 0, value_caps);

	// Populate TouchPadInputData struct and return.
	data.contacts = contacts;
	data.scan_time = scan_time;
	data.contact_count = contact_count;
	return data;
}

/**
 * \brief Reads and updates cursor speed information from the Precision Touchpad registry.
 *
 * This method opens the Precision Touchpad registry key, reads the cursor speed value,
 * and updates the internal cursor speed value used by the program. The cursor speed
 * value is scaled from the range of [0, 20] to [0.0, 1.0] to match the range used by
 * the gesture movement calculation. 
 */
void ReadPrecisionTouchPadInfo()
{
	// Open the Precision Touchpad key in the registry
	HKEY touch_pad_key;
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad", 0,
	                           KEY_READ, &touch_pad_key);
	if (result != ERROR_SUCCESS)
	{
		logger.Error("Failed to open Precision Touchpad registry key.");
		return;
	}

	// Read the cursor speed value from the registry
	DWORD cursor_speed = 0;
	DWORD data_size = sizeof(DWORD);
	result = RegQueryValueEx(touch_pad_key, L"CursorSpeed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&cursor_speed),
	                         &data_size);
	if (result != ERROR_SUCCESS)
	{
		logger.Error("Failed to read cursor speed value from registry.");
		RegCloseKey(touch_pad_key);
		return;
	}
	// Changes value from range [0, 20] to range [0.0 -> 1.0]
	precision_touch_cursor_speed = cursor_speed * 5 / 100.0f;
}

/**
 * \brief Creates a menu and displays it at the cursor position. The menu contains options
 * to run the application on startup or remove the startup task, and to quit the application.
 *
 * \param hWnd Handle to the window that will own the menu.
 */
void CreateTrayMenu(const HWND hWnd)
{
	// Get the cursor position to determine where to display the menu.
	POINT pt;
	GetCursorPos(&pt);

	// Create the popup menu.
	const HMENU hMenu = CreatePopupMenu();

	// Add the appropriate menu item depending on whether the application is set to run on startup.
	if (StartupRegistryKeyExists())
	{
		// Add the "Remove startup task" menu item.
		AppendMenu(hMenu, MF_STRING, kRemoveTaskMenuItemId, TEXT("Remove startup task"));
	}
	else
	{
		// Add the "Run on startup" menu item.
		AppendMenu(hMenu, MF_STRING, kCreateTaskMenuItemId, TEXT("Run on startup"));
	}

	// Add a separator and the "Quit" menu item.
	AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenu(hMenu, MF_STRING, kQuitMenuItemId, TEXT("Quit"));

	// Set the window specified by hWnd to the foreground, so that the menu will be displayed above it.
	SetForegroundWindow(hWnd);

	// Display the popup menu.
	TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);

	PostMessage(hWnd, WM_NULL, 0, 0);
	DestroyMenu(hMenu);
}

/**
 * \brief Checks if the registry key for starting the program at system startup exists.
 * \return True if the registry key exists, false otherwise.
 */
bool StartupRegistryKeyExists()
{
	HKEY hKey;
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, kStartupProgramRegistryKey, 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS)
		return false;
	DWORD valueSize = 0;
	result = RegQueryValueEx(hKey, kProgramName, 0, nullptr, nullptr, &valueSize);
	RegCloseKey(hKey);
	return result == ERROR_SUCCESS;
}

/**
 * \brief Adds the registry key for starting the program at system startup.
 */
void AddStartupRegistryKey()
{
	HKEY hKey;
	WCHAR app_path[MAX_PATH];
	const DWORD path_len = GetModuleFileName(nullptr, app_path, MAX_PATH);
	if (path_len == 0 || path_len == MAX_PATH)
	{
		logger.Error("Could not retrieve the application path!");
		return;
	}
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, kStartupProgramRegistryKey, 0, KEY_WRITE, &hKey);
	if (result != ERROR_SUCCESS)
		return;
	result = RegSetValueEx(hKey, kProgramName, 0, REG_SZ, (BYTE*)app_path,
	                       (DWORD)(wcslen(app_path) + 1) * sizeof(wchar_t));
	if (result == ERROR_SUCCESS)
	{
		MessageBox(nullptr, TEXT("Startup task has been created successfully."), L"Three Finger Drag", MB_OK);
		logger.Info("Startup task removed.");
	}
	else
		DisplayErrorMessage("An error occurred while trying to set the registry value.");

	RegCloseKey(hKey);
}

/**
 * \brief Removes the registry key for starting the program at system startup. Displays a message box on success.
 */
void RemoveStartupRegistryKey()
{
	HKEY hKey;
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, kStartupProgramRegistryKey, 0, KEY_WRITE, &hKey);
	if (result != ERROR_SUCCESS)
	{
		DisplayErrorMessage("Registry key could not be found.");
		return;
	}
	result = RegDeleteValue(hKey, kProgramName);
	if (result == ERROR_SUCCESS)
	{
		MessageBox(nullptr, TEXT("Startup task has been removed successfully."), L"Three Finger Drag", MB_OK);
		logger.Info("Startup task removed.");
	}
	RegCloseKey(hKey);
}

/**
 * \brief Displays a message box popup with an error icon and message.
 * \param message The message to display
 * \remarks The sent message will be logged as an error.
 */
void DisplayErrorMessage(const std::string& message)
{
	const std::wstring temp = std::wstring(message.begin(), message.end());
	const LPCWSTR wstr_message = temp.c_str();

	MessageBox(nullptr, wstr_message,TEXT("Three Finger Drag"), MB_OK | MB_ICONERROR);
	logger.Error(message);
}

/**
 * \brief Asynchronously calls a left click down mouse click & sets dragging state to true
 */
void StartDragging()
{
	auto a1 = std::async(std::launch::async, SimulateClick, MOUSEEVENTF_LEFTDOWN);
	is_dragging = true;
}

/**
 * \brief Asynchronously calls a left click down mouse click & sets dragging state to true
 */
void StopDragging()
{
	auto a = std::async(std::launch::async, SimulateClick, MOUSEEVENTF_LEFTUP);
	is_dragging = false;
	gesture_frames_skipped = 0;
}

double ClampDouble(const double in, const double min, const double max)
{
	return in < min ? min : in > max ? max : in;
}

void HandleUncaughtExceptions()
{
	std::stringstream ss;
	try
	{
		throw; // Rethrow the exception to get the exception type and message
	}
	catch (const std::exception& e)
	{
		ss << "Unhandled exception: " << typeid(e).name() << ": " << e.what() << "\n";
	}
	catch (...)
	{
		ss << "Unhandled exception: Unknown exception type\n";
	}
	logger.Error(ss.str());
	std::terminate();
}
