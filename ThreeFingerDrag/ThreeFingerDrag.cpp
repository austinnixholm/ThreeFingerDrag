#include "ThreeFingerDrag.h"

using namespace Gestures;
using namespace WinToastLib;

// Constants
namespace
{
	constexpr auto STARTUP_REGISTRY_KEY = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
	constexpr auto PROGRAM_NAME = L"ThreeFingerDrag";
	constexpr auto UPDATE_SETTINGS_PERIOD_MS = std::chrono::milliseconds(2000);
	constexpr auto TOUCH_ACTIVITY_PERIOD_MS = std::chrono::milliseconds(50);
	constexpr auto MAX_LOAD_STRING_LENGTH = 100;
	constexpr auto QUIT_MENU_ITEM_ID = 1;
	constexpr auto CREATE_TASK_MENU_ITEM_ID = 2;
	constexpr auto REMOVE_TASK_MENU_ITEM_ID = 3;
}

// Global Variables

HINSTANCE current_instance;
HWND tool_window_handle; 
WCHAR title_bar_text[MAX_LOAD_STRING_LENGTH];
WCHAR main_window_class_name[MAX_LOAD_STRING_LENGTH];
NOTIFYICONDATA tray_icon_data;
GestureProcessor gesture_processor;
std::thread update_settings_thread;
std::thread touch_activity_thread;
BOOL application_running = true;

// Forward declarations

ATOM RegisterWindowClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool StartupRegistryKeyExists();
void CreateTrayMenu(HWND hWnd);
void RemoveStartupRegistryKey();
void AddStartupRegistryKey();
void ReadPrecisionTouchPadInfo();
void ReadCursorSpeed();
void StartPeriodicUpdateThreads();
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
	const HANDLE hMutex = CreateMutex(nullptr, TRUE, PROGRAM_NAME);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		Popups::DisplayErrorMessage("Another instance of Three Finger Drag is already running.");
		CloseHandle(hMutex);
		return FALSE;
	}

	// Read any user values from Windows precision touch pad settings from registry
	ReadPrecisionTouchPadInfo();

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, title_bar_text, MAX_LOAD_STRING_LENGTH);
	LoadStringW(hInstance, IDC_THREEFINGERDRAG, main_window_class_name, MAX_LOAD_STRING_LENGTH);
	RegisterWindowClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		ERROR("Application initialization failed.");
		return FALSE;
	}

	// Start threads
	StartPeriodicUpdateThreads();

	// First time running application
	if (Application::IsInitialStartup()) {
		bool result = Popups::DisplayPrompt("Would you like run ThreeFingerDrag on startup of Windows?", "ThreeFingerDrag");
		if (result)
			AddStartupRegistryKey();
		Popups::ShowToastNotification(L"You can access the program in the system tray.", L"Welcome to ThreeFingerDrag!");
	}

	MSG msg;

	// Enter message loop and process incoming messages until WM_QUIT is received
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Delete tray icon
	Shell_NotifyIcon(NIM_DELETE, &tray_icon_data);

	// Join threads
	application_running = false;
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
	wcex.lpszClassName = main_window_class_name;
	wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THREEFINGERDRAG));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(const HINSTANCE hInstance, int nCmdShow)
{
	current_instance = hInstance;

	tool_window_handle = CreateWindowEx(WS_EX_TOOLWINDOW, main_window_class_name, title_bar_text, 0,
	                      0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
	if (!tool_window_handle)
	{
		ERROR("Window handle could not be created!");
		return FALSE;
	}

	// Initialize WinToast notifications

	auto appName = L"ThreeFingerDrag";

	WinToast::WinToastError error;
	WinToast::instance()->setAppName(appName);

	auto str = Application::GetVersionString();
	const std::wstring w_version(str.begin(), str.end());
	const auto aumi = WinToast::configureAUMI(L"Austin Nixholm", appName, L"Tool", L"Current");

	WinToast::instance()->setAppUserModelId(aumi);

	if (!WinToast::instance()->initialize(&error)) {
		ERROR("Failed to initialize WinToast.");
		return FALSE;
	}

	// Register raw input touch device

	RAWINPUTDEVICE rid;

	rid.usUsagePage = HID_USAGE_PAGE_DIGITIZER;
	rid.usUsage = HID_USAGE_DIGITIZER_TOUCH_PAD;
	rid.dwFlags = RIDEV_INPUTSINK; // Receive input even when the application is in the background
	rid.hwndTarget = tool_window_handle; // Handle to the application window

	const auto message = std::string();
	if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
	{
		Popups::DisplayErrorMessage("Raw touch device couldn't be registered. Program will now exit.");
		return FALSE;
	}

	// Initialize tray icon data

	tray_icon_data.cbSize = sizeof(NOTIFYICONDATA);
	tray_icon_data.hWnd = tool_window_handle;
	tray_icon_data.uID = 1;
	tray_icon_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	tray_icon_data.uCallbackMessage = WM_USER + 1;
	tray_icon_data.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THREEFINGERDRAG));

	std::string title = "Three Finger Drag ";
	title.append(Application::GetVersionString());
	const std::wstring w_title(title.begin(), title.end());
	lstrcpy(tray_icon_data.szTip, w_title.c_str());

	Shell_NotifyIcon(NIM_ADD, &tray_icon_data);

	return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		{
			SendMessage(hWnd, WM_SETICON, ICON_BIG, LPARAM(LoadIcon(current_instance, MAKEINTRESOURCE(IDI_THREEFINGERDRAG))));
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, LPARAM(LoadIcon(current_instance, MAKEINTRESOURCE(IDI_SMALL))));
		}
		break;

	case WM_INPUT:
		gesture_processor.ParseRawTouchData(lParam);
		break;

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
			case QUIT_MENU_ITEM_ID:
				DestroyWindow(hWnd);
				break;
			case CREATE_TASK_MENU_ITEM_ID:
				AddStartupRegistryKey();
				break;
			case REMOVE_TASK_MENU_ITEM_ID:
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
 * \brief Starts a thread that periodically updates setting info.
 */
void StartPeriodicUpdateThreads()
{
	// Check for any updates to the user's precision touchpad settings every few seconds
	update_settings_thread = std::thread([&]
	{
		while (application_running)
		{
			std::this_thread::sleep_for(UPDATE_SETTINGS_PERIOD_MS);
			ReadPrecisionTouchPadInfo();
			ReadCursorSpeed();
		}
	});

	// Check if the dragging action needs to be completed
	touch_activity_thread = std::thread([&]
	{
		while (application_running)
		{
			std::this_thread::sleep_for(TOUCH_ACTIVITY_PERIOD_MS);
			if (!gesture_processor.IsDragging())
				continue;

			gesture_processor.CheckDragInactivity();
		}
	});
}

/**
 * \brief Reads and updates touchpad cursor speed information from the Precision Touchpad registry.
 *
 * This method opens the Precision Touchpad registry key, reads the cursor speed value,
 * and updates the internal cursor speed value used by the program. The cursor speed
 * value is scaled from the initial range of [0, 20] to [0.0, 1.0] to match the range used by
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
		ERROR("Failed to open Precision Touchpad registry key.");
		return;
	}

	// Read the cursor speed value from the registry
	DWORD touch_speed = 0;
	DWORD data_size = sizeof(DWORD);
	result = RegQueryValueEx(touch_pad_key, L"CursorSpeed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&touch_speed),
	                         &data_size);
	if (result != ERROR_SUCCESS)
	{
		ERROR("Failed to read cursor speed value from registry.");
		RegCloseKey(touch_pad_key);
		return;
	}
	// Changes value from range [0, 20] to range [0.0 -> 1.0]
	gesture_processor.SetTouchSpeed(touch_speed * 5 / 100.0f);
}

/**
 * \brief Reads and updates mouse cursor speed information from Windows settings.
 *
 * The cursor speed value is scaled from the initial range of [0, 20] to [0.0, 1.0] 
 * to match the range used by the gesture movement calculation.
 */
void ReadCursorSpeed() 
{
	int mouse_speed;
	BOOL result = SystemParametersInfo(SPI_GETMOUSESPEED, 0, &mouse_speed, 0);

	if (!result) {
		ERROR("Failed to read cursor speed from system.");
		return;
	}

	// Changes value from range [0, 20] to range [0.0 -> 1.0]
	gesture_processor.SetMouseSpeed(mouse_speed * 5 / 100.0f);
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
		AppendMenu(hMenu, MF_STRING, REMOVE_TASK_MENU_ITEM_ID, TEXT("Remove startup task"));
	}
	else
	{
		// Add the "Run on startup" menu item.
		AppendMenu(hMenu, MF_STRING, CREATE_TASK_MENU_ITEM_ID, TEXT("Run on startup"));
	}

	// Add a separator and the "Exit" menu item.
	AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenu(hMenu, MF_STRING, QUIT_MENU_ITEM_ID, TEXT("Exit"));

	// Set the window specified by tool_window_handle to the foreground, so that the menu will be displayed above it.
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
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, STARTUP_REGISTRY_KEY, 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS)
		return false;
	DWORD valueSize = 0;
	result = RegQueryValueEx(hKey, PROGRAM_NAME, 0, nullptr, nullptr, &valueSize);
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
		ERROR("Could not retrieve the application path!");
		return;
	}
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, STARTUP_REGISTRY_KEY, 0, KEY_WRITE, &hKey);
	if (result != ERROR_SUCCESS)
		return;
	result = RegSetValueEx(hKey, PROGRAM_NAME, 0, REG_SZ, (BYTE*)app_path,
	                       (DWORD)(wcslen(app_path) + 1) * sizeof(wchar_t));
	if (result == ERROR_SUCCESS)
		Popups::DisplayInfoMessage("Startup task has been created successfully.");
	else
		Popups::DisplayErrorMessage("An error occurred while trying to set the registry value.");

	RegCloseKey(hKey);
}

/**
 * \brief Removes the registry key for starting the program at system startup. Displays a message box on success.
 */
void RemoveStartupRegistryKey()
{
	HKEY hKey;
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, STARTUP_REGISTRY_KEY, 0, KEY_WRITE, &hKey);
	if (result != ERROR_SUCCESS)
	{
		Popups::DisplayErrorMessage("Registry key could not be found.");
		return;
	}
	result = RegDeleteValue(hKey, PROGRAM_NAME);
	if (result == ERROR_SUCCESS)
		Popups::DisplayInfoMessage("Startup task has been removed successfully.");

	RegCloseKey(hKey);
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
	ERROR(ss.str());
	std::terminate();
}

