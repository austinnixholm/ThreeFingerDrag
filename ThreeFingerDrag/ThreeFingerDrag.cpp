#include <iostream>
#include <Windows.h>
#include "framework.h"
#include "ThreeFingerDrag.h"

// Constants

constexpr auto kStartupProgramRegistryKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr auto kProgramName = L"ThreeFingerDrag";
constexpr auto kUpdateSettingsPeriodMs = std::chrono::milliseconds(3000);
constexpr auto kTouchActivityPeriodMs = std::chrono::milliseconds(100);
constexpr auto kInactivityThresholdMs = 50;
constexpr auto kNumSkippedFrames = 8;
constexpr auto kMaxTouchMovementSpeed = 40;
constexpr auto kTouchMovementAcceleration = 0.03;
constexpr auto kTouchPow = 1.001;
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

HINSTANCE hInst; // current instance
HWND hWnd; // Tool window handle
WCHAR szTitle[kMaxLoadString]; // The title bar text
WCHAR szWindowClass[kMaxLoadString]; // The main window class name
NOTIFYICONDATA nid;
std::mutex mutex;
std::thread update_settings_thread;
std::thread touch_activity_thread;
std::chrono::time_point<std::chrono::steady_clock> last_detection; // Last time gesture was detected
std::vector<TouchPadContact> last_contacts; // Last detected touch pad contact points
DOUBLE precision_touch_cursor_speed; // Precision touch pad cursor speed
BOOL is_dragging = false; // True if a touch drag action has not yet been cancelled
BOOL run_loops = true;
INT num_frames_skipped = 0;

// Forward declarations

ATOM RegisterWindowClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT ParseRawInput(LPARAM lParam);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DOUBLE ClampDouble(double in, double min, double max);
TouchPadInputData GetTouchData(HRAWINPUT hRawInputHandle);
bool StartupRegistryKeyExists();
void RemoveStartupRegistryKey();
void AddStartupRegistryKey();
void ReadPrecisionTouchPadRegistry();
void CreateUpdateThread();
void CreateActivityThread();
void SimulateClick(DWORD flags);
void MoveMousePointer(const TouchPadInputData& data, const std::chrono::time_point<std::chrono::steady_clock>& now);
void StartDragging();
void StopDragging();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	const HANDLE hMutex = CreateMutex(nullptr, TRUE, kProgramName);

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBox(nullptr, TEXT("Another instance of Three Finger Drag is already running."), TEXT("Error"),
		           MB_OK | MB_ICONERROR);
		// Another instance of the program is already running
		CloseHandle(hMutex);
		return FALSE;
	}

	// Read any user values from Windows precision touch pad settings from registry
	ReadPrecisionTouchPadRegistry();

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, kMaxLoadString);
	LoadStringW(hInstance, IDC_THREEFINGERDRAG, szWindowClass, kMaxLoadString);
	RegisterWindowClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
		return FALSE;

	// Threads
	CreateUpdateThread();
	CreateActivityThread();

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

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, szWindowClass, szTitle, 0,
	                      0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd)
		return FALSE;

	// Register raw input touch device

	RAWINPUTDEVICE rid;

	rid.usUsagePage = HID_USAGE_PAGE_DIGITIZER;
	rid.usUsage = HID_USAGE_DIGITIZER_TOUCH_PAD;
	rid.dwFlags = RIDEV_INPUTSINK; // Receive input even when the application is in the background
	rid.hwndTarget = hWnd; // Handle to the application window

	if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
	{
		MessageBox(nullptr, TEXT("Raw device couldn't be registered"), TEXT("Info"), MB_OK);
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
	title.append(VERSION);
	const std::wstring w_title(title.begin(), title.end());
	lstrcpy(nid.szTip, w_title.c_str());

	Shell_NotifyIcon(NIM_ADD, &nid);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INPUT:
		{
			auto a = std::async(std::launch::async, ParseRawInput, lParam);
		}
		return 0;

	case WM_USER + 1:
		switch (lParam)
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			{
				POINT pt;
				GetCursorPos(&pt);
				const HMENU hMenu = CreatePopupMenu();

				if (StartupRegistryKeyExists())
					AppendMenu(hMenu, MF_STRING, kRemoveTaskMenuItemId, TEXT("Remove startup task"));
				else
					AppendMenu(hMenu, MF_STRING, kCreateTaskMenuItemId, TEXT("Run on startup"));

				AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
				AppendMenu(hMenu, MF_STRING, kQuitMenuItemId, TEXT("Quit"));

				SetForegroundWindow(hWnd);
				TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
				PostMessage(hWnd, WM_NULL, 0, 0);
				DestroyMenu(hMenu);
			}
			break;

		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;

		case WM_DESTROY:
			Shell_NotifyIcon(NIM_DELETE, &nid);
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_COMMAND:
		{
			const int wm_id = LOWORD(wParam);
			// Parse the menu selections:
			switch (wm_id)
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
		Shell_NotifyIcon(NIM_DELETE, &nid);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


LRESULT ParseRawInput(LPARAM lParam)
{
	const auto hRawInputHandle = (HRAWINPUT)lParam;
	TouchPadInputData data = GetTouchData(hRawInputHandle);

	if (data.valid_contact && data.contacts.size() > 2)
	{
		const std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::high_resolution_clock::now();

		MoveMousePointer(data, now);

		// Update old information to current
		last_contacts = data.contacts;
		last_detection = now;
	}

	if (data.contact_count < 3)
	{
		if (is_dragging)
			StopDragging();
		data.contacts.clear();
		num_frames_skipped = 0;
	}

	return 0;
}

/**
 * \brief Starts a thread that periodically updates setting info.
 */
void CreateUpdateThread()
{
	update_settings_thread = std::thread([&]
	{
		while (run_loops)
		{
			std::this_thread::sleep_for(kUpdateSettingsPeriodMs);
			ReadPrecisionTouchPadRegistry();
		}
	});
}

/**
 * \brief Starts a thread that periodically checks 
 */
void CreateActivityThread()
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

void SimulateClick(const DWORD flags)
{
	std::lock_guard<std::mutex> lk(mutex);
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
void MoveCursor(int delta_x, int delta_y)
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
	const std::vector<TouchPadContact> contacts = data.contacts;
	// Calculate the time elapsed since the last touchpad contact
	const std::chrono::duration<float> duration = now - last_detection;
	const float ms_since_last = duration.count() * 1000.0f;

	// Initialize the change in x and y coordinates of the mouse pointer
	int total_delta_x = 0;
	int total_delta_y = 0;

	// Ignore initial movement frames. This prevents unwanted movement in the beginning of the gesture.
	if (++num_frames_skipped > kNumSkippedFrames && !last_contacts.empty() && ms_since_last < kInactivityThresholdMs)
	{
		// Calculate the movement delta for each finger and add them up
		int valid_touch_movements = 0;
		for (int i = 0; i < 3; i++)
		{
			const auto& contact = contacts[i];

			// Calculate the movement delta for the current finger
			const int delta_x = (contact.x - last_contacts[i].x) * precision_touch_cursor_speed;
			const int delta_y = (contact.y - last_contacts[i].y) * precision_touch_cursor_speed;

			if (std::abs(delta_x) > 0 || std::abs(delta_y) > 0)
				valid_touch_movements++;

			// Add the movement delta for the current finger to the total
			total_delta_x += delta_x;
			total_delta_y += delta_y;
		}

		if (valid_touch_movements > 1)
		{
			total_delta_x /= valid_touch_movements;
			total_delta_y /= valid_touch_movements;
		}

		// Apply movement acceleration
		const double speed = ClampDouble(std::sqrt(total_delta_x * total_delta_x + total_delta_y * total_delta_y), 1,
		                                 kMaxTouchMovementSpeed);
		const double factor = pow(speed, kTouchPow);

		total_delta_x = static_cast<int>(total_delta_x * factor * kTouchMovementAcceleration);
		total_delta_y = static_cast<int>(total_delta_y * factor * kTouchMovementAcceleration);
	}

	// Move the mouse pointer based on the calculated vector
	MoveCursor(total_delta_x, total_delta_y);
	if (!is_dragging)
		StartDragging();
}

TouchPadInputData GetTouchData(HRAWINPUT hRawInputHandle)
{
	TouchPadInputData data{};
	std::vector<TouchPadContact> contacts;
	UINT size = sizeof(RAWINPUT);
	const HANDLE hHeap = GetProcessHeap();
	GetRawInputData(hRawInputHandle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
	if (size == 0)
		return data;

	auto* raw_input = static_cast<RAWINPUT*>(HeapAlloc(hHeap, 0, size));
	if (raw_input == nullptr)
		return data;

	if (GetRawInputData(hRawInputHandle, RID_INPUT, raw_input, &size, sizeof(RAWINPUTHEADER)) == -1)
	{
		HeapFree(hHeap, 0, raw_input);
		return data;
	}

	UINT buffer_size;
	GetRawInputDeviceInfo(raw_input->header.hDevice, RIDI_PREPARSEDDATA, nullptr, &buffer_size);
	if (buffer_size == 0)
	{
		HeapFree(hHeap, 0, raw_input);
		return data;
	}

	const auto pre_parsed_data = static_cast<PHIDP_PREPARSED_DATA>(HeapAlloc(hHeap, 0, buffer_size));
	GetRawInputDeviceInfo(raw_input->header.hDevice, RIDI_PREPARSEDDATA, pre_parsed_data, &buffer_size);
	HIDP_CAPS caps;
	if (HidP_GetCaps(pre_parsed_data, &caps) != HIDP_STATUS_SUCCESS)
	{
		HeapFree(hHeap, 0, raw_input);
		HeapFree(hHeap, 0, pre_parsed_data);
		return data;
	}

	USHORT length = caps.NumberInputValueCaps;
	const auto value_caps = static_cast<PHIDP_VALUE_CAPS>(HeapAlloc(hHeap, 0, sizeof(HIDP_VALUE_CAPS) * length));
	if (HidP_GetValueCaps(HidP_Input, value_caps, &length, pre_parsed_data) != HIDP_STATUS_SUCCESS)
	{
		HeapFree(hHeap, 0, raw_input);
		HeapFree(hHeap, 0, pre_parsed_data);
		HeapFree(hHeap, 0, value_caps);
		return data;
	}

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
				// If three fingers are touching the touchpad at any point, allow to start drag
				if (contact_count == 3)
					data.valid_contact = true;
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

		if (contact.contact_id != kInitValue && contact.x != kInitValue && contact.y != kInitValue)
		{
			contacts.emplace_back(contact);
			contact = {kInitValue, kInitValue, kInitValue};
		}
	}
	HeapFree(hHeap, 0, raw_input);
	HeapFree(hHeap, 0, pre_parsed_data);
	HeapFree(hHeap, 0, value_caps);

	data.contacts = contacts;
	data.scan_time = scan_time;
	data.contact_count = contact_count;
	return data;
}

void ReadPrecisionTouchPadRegistry()
{
	// Open the Precision Touchpad key in the registry
	HKEY touch_pad_key;
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad", 0,
	                           KEY_READ, &touch_pad_key);
	if (result != ERROR_SUCCESS)
	{
		std::cerr << "Failed to open Precision Touchpad cursor speed key." << std::endl;
		return;
	}

	// Read the cursor speed value from the registry
	DWORD cursor_speed = 0;
	DWORD data_size = sizeof(DWORD);
	result = RegQueryValueEx(touch_pad_key, L"CursorSpeed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&cursor_speed),
	                         &data_size);
	if (result != ERROR_SUCCESS)
	{
		std::cerr << "Failed to read cursor speed value from registry." << std::endl;
		RegCloseKey(touch_pad_key);
		return;
	}
	// Changes value from range [0, 20] to range [0.0, 1.0]
	precision_touch_cursor_speed = cursor_speed * 5 / 100.0f;
}

void StartDragging()
{
	auto a1 = std::async(std::launch::async, SimulateClick, MOUSEEVENTF_LEFTDOWN);
	is_dragging = true;
}

void StopDragging()
{
	auto a = std::async(std::launch::async, SimulateClick, MOUSEEVENTF_LEFTUP);
	is_dragging = false;
}

DOUBLE ClampDouble(double in, double min, double max)
{
	if (in > max)
		return max;
	if (in < min)
		return min;
	return in;
}

bool StartupRegistryKeyExists()
{
	HKEY hKey;
	const wchar_t* keyName = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, keyName, 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS)
		return false;
	DWORD valueSize = 0;
	result = RegQueryValueEx(hKey, kProgramName, 0, nullptr, nullptr, &valueSize);
	RegCloseKey(hKey);
	return result == ERROR_SUCCESS;
}

void AddStartupRegistryKey()
{
	HKEY hKey;
	WCHAR app_path[MAX_PATH];
	const DWORD path_len = GetModuleFileName(nullptr, app_path, MAX_PATH);
	if (path_len == 0 || path_len == MAX_PATH)
	{
		// Error: could not retrieve the application path
		return;
	}
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, kStartupProgramRegistryKey, 0, KEY_WRITE, &hKey);
	if (result != ERROR_SUCCESS)
		return;
	result = RegSetValueEx(hKey, kProgramName, 0, REG_SZ, (BYTE*)app_path,
	                       (DWORD)(wcslen(app_path) + 1) * sizeof(wchar_t));
	if (result == ERROR_SUCCESS)
		MessageBox(nullptr, TEXT("Startup task has beeen created successfully."), L"Three Finger Drag", MB_OK);
	RegCloseKey(hKey);
}

void RemoveStartupRegistryKey()
{
	HKEY hKey;
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, kStartupProgramRegistryKey, 0, KEY_WRITE, &hKey);
	if (result != ERROR_SUCCESS)
		return;
	result = RegDeleteValue(hKey, kProgramName);
	if (result == ERROR_SUCCESS)
		MessageBox(nullptr, TEXT("Startup task has beeen removed successfully."), L"Three Finger Drag", MB_OK);
	RegCloseKey(hKey);
}
