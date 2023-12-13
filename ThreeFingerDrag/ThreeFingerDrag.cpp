#include "ThreeFingerDrag.h"

using namespace Gestures;
using namespace WinToastLib;

// Constants
namespace
{
    constexpr auto STARTUP_REGISTRY_KEY = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    constexpr auto PROGRAM_NAME = L"ThreeFingerDrag";
    constexpr auto UPDATE_SETTINGS_PERIOD_MS = std::chrono::milliseconds(2000);
    constexpr auto TOUCH_ACTIVITY_PERIOD_MS = std::chrono::milliseconds(1);
    constexpr auto MAX_LOAD_STRING_LENGTH = 100;

    constexpr auto SETTINGS_WINDOW_WIDTH = 456;
    constexpr auto SETTINGS_WINDOW_HEIGHT = 170;
    constexpr auto MIN_CANCELLATION_DELAY_MS = 100;
    constexpr auto MAX_CANCELLATION_DELAY_MS = 2000;
    constexpr auto MIN_GESTURE_SPEED = 1;
    constexpr auto MAX_GESTURE_SPEED = 100;
    constexpr auto ID_SETTINGS_MENUITEM = 10000;
    constexpr auto ID_QUIT_MENUITEM = 10001;
    constexpr auto ID_RUN_ON_STARTUP_CHECKBOX = 10002;
    constexpr auto ID_GESTURE_SPEED_TRACKBAR = 10003;
    constexpr auto ID_TEXT_BOX = 10004;
    constexpr auto ID_CANCELLATION_DELAY_SPINNER = 10005;
}

// Global Variables

HINSTANCE current_instance;
HWND tray_icon_hwnd;
HWND settings_hwnd;
HWND settings_trackbar_hwnd;
HWND settings_checkbox_hwnd;
WCHAR title_bar_text[MAX_LOAD_STRING_LENGTH];
WCHAR settings_title_text[MAX_LOAD_STRING_LENGTH];
WCHAR main_window_class_name[MAX_LOAD_STRING_LENGTH];
WCHAR settings_window_class_name[MAX_LOAD_STRING_LENGTH];
NOTIFYICONDATA tray_icon_data;
GestureProcessor gesture_processor;
std::thread update_settings_thread;
std::thread touch_activity_thread;
BOOL application_running = TRUE;
BOOL gui_initialized = FALSE;
HBRUSH white_brush = CreateSolidBrush(RGB(255, 255, 255));
HFONT normal_font = CreateFont(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS,
                               CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));
GlobalConfig* config = GlobalConfig::GetInstance();

// Forward declarations

ATOM RegisterWindowClass(HINSTANCE, WCHAR*, WNDPROC);
BOOL InitInstance(HINSTANCE);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SettingsWndProc(HWND, UINT, WPARAM, LPARAM);

void CreateTrayMenu(HWND hWnd);
void ShowSettingsWindow();
void AddStartupTask();
void RemoveStartupTask();
void RemoveStartupRegistryKey();
void ReadPrecisionTouchPadInfo();
void ReadCursorSpeed();
void StartPeriodicUpdateThreads();
void HandleUncaughtExceptions();
void PerformAdditionalSteps();
void PromptUserForStartupPreference();
void InitializeConfiguration();
bool InitializeWindowsNotifications();
bool StartupRegistryKeyExists();
bool RegisterRawInputDevices();
bool CheckSingleInstance();
bool InitializeGUI();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    current_instance = hInstance;
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    std::set_terminate(HandleUncaughtExceptions);

    if (!CheckSingleInstance())
    {
        return FALSE;
    }

    InitializeConfiguration();

    if (!InitInstance(current_instance))
    {
        ERROR("Application initialization failed.");
        return FALSE;
    }

    StartPeriodicUpdateThreads();
    PerformAdditionalSteps();

    MSG msg;
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

BOOL InitInstance(const HINSTANCE hInstance)
{
    // Initialize WinToast notifications
    if (!InitializeWindowsNotifications())
    {
        ERROR("Failed to initialize WinToast.");
        return FALSE;
    }

    // Initialize tray icon, settings window
    if (!InitializeGUI())
    {
        Popups::DisplayErrorMessage("GUI initialization failed!");
        return FALSE;
    }

    // Start listening to raw touchpad input.
    if (!RegisterRawInputDevices())
    {
        Popups::DisplayErrorMessage(
            "ThreeFingerDrag couldn't find a precision touchpad device on your system. The program will now exit.");
        return FALSE;
    }

    // Show the settings icon
    Shell_NotifyIcon(NIM_ADD, &tray_icon_data);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    // Raw touch device input
    case WM_INPUT:
        gesture_processor.ParseRawTouchData(lParam);
        break;

    // Notify Icon
    case WM_APP:
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
            // Parse menu selections
            switch (LOWORD(wParam))
            {
            case ID_QUIT_MENUITEM:
                DestroyWindow(hWnd);
                break;
            case ID_SETTINGS_MENUITEM:
                ShowSettingsWindow();
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_DESTROY:
        DeleteObject(white_brush);
        DeleteObject(normal_font);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK SettingsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    const auto loword_param = LOWORD(wParam);
    const auto hiword_param = HIWORD(wParam);
    switch (msg)
    {
    case WM_COMMAND:

        if (hiword_param == EN_CHANGE)
        {
            // Ignore change events until GUI has been initialized
            if (!gui_initialized)
                return TRUE;

            switch (loword_param)
            {
            // The text in the textbox has changed. Update the config value
            case ID_TEXT_BOX:
                wchar_t buffer[64];
                GetWindowText((HWND)lParam, buffer, 64); // get textbox text
                config->SetCancellationDelayMs(_wtoi(buffer)); // convert to integer (only numerical values are entered)
                Application::WriteConfiguration();
                break;
            }
        }

        switch (loword_param)
        {
        case ID_RUN_ON_STARTUP_CHECKBOX:
            // Run on startup checkbox clicked
            if (SendMessage(GetDlgItem(hWnd, ID_RUN_ON_STARTUP_CHECKBOX), BM_GETCHECK, 0, 0) == BST_CHECKED)
                AddStartupTask();
            else
                RemoveStartupTask();
            break;
        }
        break;

    case WM_HSCROLL:
        if (GetDlgCtrlID((HWND)lParam) == ID_GESTURE_SPEED_TRACKBAR)
        {
            if (loword_param < TB_THUMBTRACK)
            {
                // Trackbar value changed
                int tbPos = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);

                config->SetGestureSpeed(tbPos);
                Application::WriteConfiguration();
            }
        }
        break;

    case WM_CTLCOLORSTATIC:
        {
            HDC hdcStatic = (HDC)wParam;
            SetBkColor(hdcStatic, RGB(255, 255, 255));
            return (INT_PTR)white_brush;
        }

    case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT rect;
            GetClientRect(hWnd, &rect);
            FillRect(hdc, &rect, white_brush);
            return TRUE;
        }

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return TRUE;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/**
 * \brief Create the main application window and the settings window.
 */
bool InitializeGUI()
{
    tray_icon_hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        main_window_class_name,
        title_bar_text,
        0,
        0, 0,
        0, 0,
        NULL,
        NULL,
        current_instance,
        NULL);

    if (!tray_icon_hwnd)
    {
        ERROR("Tray icon window handle could not be created!");
        return FALSE;
    }

    // Initialize tray icon data
    tray_icon_data.cbSize = sizeof(NOTIFYICONDATA);
    tray_icon_data.hWnd = tray_icon_hwnd;
    tray_icon_data.uID = 1;
    tray_icon_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    tray_icon_data.uCallbackMessage = WM_APP;
    tray_icon_data.hIcon = LoadIcon(current_instance, MAKEINTRESOURCE(IDI_THREEFINGERDRAG));

    // Set tooltip to versioned title
    std::string title = "Three Finger Drag ";
    title.append(Application::GetVersionString());
    const std::wstring w_title(title.begin(), title.end());
    lstrcpy(tray_icon_data.szTip, w_title.c_str());

    // Create the settings window

    settings_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_COMPOSITED, // Always on top and enable double-buffering
        settings_window_class_name,
        settings_title_text,
        WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        SETTINGS_WINDOW_WIDTH, SETTINGS_WINDOW_HEIGHT,
        tray_icon_hwnd,
        NULL,
        current_instance,
        NULL
    );

    const int textbox_width = 56, textbox_height = 24;
    const int label_height = 20, trackbar_height = 46;
    const int margin = 32;
    int pos_x = 8, pos_y = 8;

    // Run on startup checkbox
    settings_checkbox_hwnd = CreateWindowW(L"button", L"Run ThreeFingerDrag on startup of Windows",
                                           WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                           pos_x, pos_y, SETTINGS_WINDOW_WIDTH - 32, 20, settings_hwnd,
                                           (HMENU)ID_RUN_ON_STARTUP_CHECKBOX, NULL, NULL);

    SendMessage(settings_checkbox_hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(normal_font), TRUE);

    pos_y += 24;

    // Numeric textbox
    HWND hwndTextBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", NULL,
                                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_NUMBER,
                                      pos_x, pos_y, textbox_width, textbox_height,
                                      settings_hwnd, (HMENU)ID_TEXT_BOX, current_instance, NULL);

    SendMessage(hwndTextBox, WM_SETFONT, reinterpret_cast<WPARAM>(normal_font), TRUE);

    // Spinner for numeric textbox
    HWND settings_spinner_hwnd = CreateWindowW(L"msctls_updown32", NULL,
                                               WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT
                                               | UDS_NOTHOUSANDS,
                                               pos_x, pos_y, 0, label_height, 
                                               settings_hwnd, (HMENU)ID_CANCELLATION_DELAY_SPINNER, current_instance,
                                               NULL);

    SendMessage(settings_spinner_hwnd, UDM_SETBUDDY, (WPARAM)hwndTextBox, 0);
    SendMessage(settings_spinner_hwnd, UDM_SETRANGE, 0, MAKELONG(MAX_CANCELLATION_DELAY_MS, MIN_CANCELLATION_DELAY_MS));
    SendMessage(settings_spinner_hwnd, UDM_SETPOS, 0, MAKELONG(config->GetCancellationDelayMs(), 0));

    pos_x += 60;

    // Label for numeric textbox
    HWND hwnd_spinner_label = CreateWindowW(L"STATIC", L"Cancellation Delay (milliseconds)",
                                            WS_CHILD | WS_VISIBLE | SS_LEFT,
                                            pos_x, pos_y, SETTINGS_WINDOW_WIDTH - margin, label_height,
                                            settings_hwnd, NULL, NULL, NULL);

    SendMessage(hwnd_spinner_label, WM_SETFONT, reinterpret_cast<WPARAM>(normal_font), TRUE);

    pos_x = 8; // Resetting the X position for the new line
    pos_y += 26;

    // Label for trackbar
    HWND hwnd_trackbar_label = CreateWindowW(WC_STATIC, L"Gesture speed (slower -> faster)",
                                             WS_CHILD | WS_VISIBLE | SS_LEFT,
                                             pos_x, pos_y, SETTINGS_WINDOW_WIDTH - margin, label_height, settings_hwnd,
                                             NULL, NULL, NULL);

    SendMessage(hwnd_trackbar_label, WM_SETFONT, reinterpret_cast<WPARAM>(normal_font), TRUE);

    pos_y += 20;

    // Gesture speed trackbar
    settings_trackbar_hwnd = CreateWindowW(TRACKBAR_CLASS, NULL,
                                           WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_TOOLTIPS,
                                           pos_x, pos_y, SETTINGS_WINDOW_WIDTH - margin, trackbar_height, settings_hwnd,
                                           (HMENU)ID_GESTURE_SPEED_TRACKBAR, current_instance, NULL);
    SendMessage(settings_trackbar_hwnd, TBM_SETRANGE, TRUE, MAKELONG(MIN_GESTURE_SPEED, MAX_GESTURE_SPEED));
    SendMessage(settings_trackbar_hwnd, TBM_SETPOS, TRUE, static_cast<int>(config->GetGestureSpeed()));

    // Remove its resizable border
    LONG_PTR style = GetWindowLongPtr(settings_hwnd, GWL_STYLE);
    style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
    SetWindowLongPtr(settings_hwnd, GWL_STYLE, style);

    if (settings_hwnd == NULL)
    {
        ERROR("Settings window handle could not be created!");
        return FALSE;
    }
    gui_initialized = TRUE;
    return TRUE;
}


/**
 * \brief Creates a menu and displays it at the cursor position. The menu contains options
 * to run the application on startup or remove the startup task, and to quit the application.
 *
 * \loword_param hWnd Handle to the window that will own the menu.
 */
void CreateTrayMenu(const HWND hWnd)
{
    // Get the cursor position to determine where to display the menu.
    POINT pt;
    GetCursorPos(&pt);

    // Create the popup menu.
    const HMENU hMenu = CreatePopupMenu();

    // Add the "Settings" menu item
    AppendMenu(hMenu, MF_STRING, ID_SETTINGS_MENUITEM, TEXT("Settings"));

    // Add a separator and the "Exit" menu item.
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, ID_QUIT_MENUITEM, TEXT("Exit"));

    // Set the window specified by tray_icon_hwnd to the foreground, so that the menu will be displayed above it.
    SetForegroundWindow(hWnd);

    // Display the popup menu.
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);

    PostMessage(hWnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

/**
 * \brief Makes the settings window visible and positions it in the center of the screen.
 */
void ShowSettingsWindow()
{
    // Update run on startup checkbox
    if (TaskScheduler::TaskExists("ThreeFingerDrag"))
        SendMessage(settings_checkbox_hwnd, BM_SETCHECK, BST_CHECKED, 0);
    else
        SendMessage(settings_checkbox_hwnd, BM_SETCHECK, BST_UNCHECKED, 0);

    // Update trackbar position
    SendMessage(settings_trackbar_hwnd, TBM_SETPOS, TRUE, static_cast<int>(config->GetGestureSpeed()));

    RECT rect_client, rect_window;
    GetClientRect(settings_hwnd, &rect_client);
    GetWindowRect(settings_hwnd, &rect_window);
    const int x = GetSystemMetrics(SM_CXSCREEN) / 2 - (rect_window.right - rect_window.left) / 2;
    const int y = GetSystemMetrics(SM_CYSCREEN) / 2 - (rect_window.bottom - rect_window.top) / 2;
    MoveWindow(settings_hwnd, x, y, SETTINGS_WINDOW_WIDTH, SETTINGS_WINDOW_HEIGHT, TRUE);

    // Show and update the window
    ShowWindow(settings_hwnd, SW_SHOW);
    UpdateWindow(settings_hwnd);
}

/**
 * \brief Starts any threads required for periodic updates throughout the application.
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
            if (!config->IsCancellationStarted())
            {
                continue;
            }
            const auto now = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<float> duration = now - config->GetCancellationTime();
            const float ms_since_cancellation = duration.count() * 1000.0f;
            if (ms_since_cancellation < config->GetCancellationDelayMs())
            {
                continue;
            }
            Cursor::LeftMouseUp();
            config->SetDragging(false);
            config->SetCancellationStarted(false);
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
    config->SetPrecisionTouchCursorSpeed(touch_speed * 5 / 100.0f);
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
    const bool result = SystemParametersInfo(SPI_GETMOUSESPEED, 0, &mouse_speed, 0);

    if (!result)
    {
        ERROR("Failed to read cursor speed from system.");
        return;
    }

    // Changes value from range [0, 20] to range [0.0 -> 1.0]
    config->SetMouseCursorSpeed(mouse_speed * 5 / 100.0f);
}


/**
 * \brief Registers the first available precision touchpad as a raw input device, to listen for WM_INPUT events.
 * \return True if the raw input device was successfully registered.
 */
bool RegisterRawInputDevices()
{
    // Register precision touchpad device

    RAWINPUTDEVICE rid;

    rid.usUsagePage = HID_USAGE_PAGE_DIGITIZER;
    rid.usUsage = HID_USAGE_DIGITIZER_TOUCH_PAD;
    rid.dwFlags = RIDEV_INPUTSINK; // Receive input even when the application is in the background
    rid.hwndTarget = tray_icon_hwnd; // Handle to the application window

    return RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));
}

/**
 * \brief Initializes WinToast to allow sending windows desktop notifications
 * \return True if the registry key exists, false otherwise.
 */
bool InitializeWindowsNotifications()
{
    auto appName = L"ThreeFingerDrag";

    WinToast::WinToastError error;
    WinToast::instance()->setAppName(appName);

    auto str = Application::GetVersionString();
    const std::wstring w_version(str.begin(), str.end());
    const auto aumi = WinToast::configureAUMI(L"Austin Nixholm", appName, L"Tool", L"Current");

    WinToast::instance()->setAppUserModelId(aumi);

    if (!WinToast::instance()->initialize(&error))
    {
        ERROR("WinToast could not be initialized.");
        return FALSE;
    }
    return true;
}

/**
 * \brief Adds the registry key for starting the program at system startup.
 */
void AddStartupTask()
{
    // Remove possible existing registry key from previous version of application
    if (StartupRegistryKeyExists())
        RemoveStartupRegistryKey();

    if (TaskScheduler::TaskExists("ThreeFingerDrag"))
    {
        Popups::DisplayInfoMessage("Startup task already exists!");
        return;
    }
    
    if (TaskScheduler::CreateLoginTask("ThreeFingerDrag", Application::ExePath().u8string()))
        Popups::DisplayInfoMessage("Startup task has been created successfully.");
    else
        Popups::DisplayErrorMessage("An error occurred while trying to create the login task.");
}

/**
 * \brief Removes the registry key for starting the program at system startup. Displays a message box on success.
 */
void RemoveStartupTask()
{
    // Remove possible existing registry key from previous version of application
    if (StartupRegistryKeyExists())
        RemoveStartupRegistryKey();

    TaskScheduler::DeleteTask("ThreeFingerDrag");
    if (!TaskScheduler::TaskExists("ThreeFingerDrag"))
        Popups::DisplayInfoMessage("Startup task has been removed successfully.");
}


/**
 * \return True if the registry key for the startup program name exists.
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
 * \brief Removes the registry key for starting the program at system startup. Displays a message box on success.
 */
void RemoveStartupRegistryKey()
{
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, STARTUP_REGISTRY_KEY, 0, KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS)
        return;
    RegDeleteValue(hKey, PROGRAM_NAME);
    RegCloseKey(hKey);
}

bool CheckSingleInstance()
{
    const HANDLE hMutex = CreateMutex(nullptr, TRUE, PROGRAM_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        Popups::DisplayErrorMessage("Another instance of Three Finger Drag is already running.");
        CloseHandle(hMutex);
        return false;
    }
    return true;
}

void InitializeConfiguration()
{
    // Read user configuration values
    Application::ReadConfiguration();
    ReadPrecisionTouchPadInfo();
    ReadCursorSpeed();

    // Initialize global strings
    LoadStringW(current_instance, IDS_APP_TITLE, title_bar_text, MAX_LOAD_STRING_LENGTH);
    LoadStringW(current_instance, IDS_SETTINGS_TITLE, settings_title_text, MAX_LOAD_STRING_LENGTH);
    LoadStringW(current_instance, IDC_THREEFINGERDRAG, main_window_class_name, MAX_LOAD_STRING_LENGTH);
    LoadStringW(current_instance, IDC_SETTINGS, settings_window_class_name, MAX_LOAD_STRING_LENGTH);

    // Register window classes
    RegisterWindowClass(current_instance, main_window_class_name, WndProc);
    RegisterWindowClass(current_instance, settings_window_class_name, SettingsWndProc);
}

void PromptUserForStartupPreference()
{
    if (Popups::DisplayPrompt("Would you like run ThreeFingerDrag on startup of Windows?", "ThreeFingerDrag"))
        AddStartupTask();
    Popups::ShowToastNotification(L"To change your sensitivity, access your settings via the tray icon.", L"Welcome to ThreeFingerDrag!");
}

void PerformAdditionalSteps()
{
    // First time running application
    if (Application::IsInitialStartup())
        PromptUserForStartupPreference();

    // Replace legacy startup registry key with login task automatically for previous users
    if (StartupRegistryKeyExists())
    {
        RemoveStartupRegistryKey();
        TaskScheduler::CreateLoginTask("ThreeFingerDrag", Application::ExePath().u8string());
    }
}

ATOM RegisterWindowClass(HINSTANCE hInstance, WCHAR* className, WNDPROC wndProc)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = 0;
    wcex.lpfnWndProc = wndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THREEFINGERDRAG));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = className;
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THREEFINGERDRAG));

    return RegisterClassExW(&wcex);
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
