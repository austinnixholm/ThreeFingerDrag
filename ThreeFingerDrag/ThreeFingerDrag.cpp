#include <iostream>
#include <Windows.h>
#include "framework.h"
#include "ThreeFingerDrag.h"

// Constants
constexpr auto kThresholdMs = 50;
constexpr auto kNumIgnoredFrames = 5;
constexpr auto kMaxLoadString = 100;
constexpr auto kInitValue = 65535;

// Global Variables:
HINSTANCE hInst; // current instance
WCHAR szTitle[kMaxLoadString]; // The title bar text
WCHAR szWindowClass[kMaxLoadString]; // the main window class name
NOTIFYICONDATA nid;
std::mutex mutex;

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void ReadPrecisionTouchPadRegistry();
void SimulateClick(DWORD flags);

// Last time since a valid three finger drag detection was made
std::chrono::time_point<std::chrono::steady_clock> last_detection;

// Last detected touch pad contact points
std::vector<TouchPadContact> last_contacts;

// Precision touch pad cursor speed
DOUBLE precision_touch_cursor_speed;

BOOL is_dragging = false;
INT num_frames = 0;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Read any user values from Windows precision touch pad settings from registry
	ReadPrecisionTouchPadRegistry();

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, kMaxLoadString);
	LoadStringW(hInstance, IDC_THREEFINGERDRAG, szWindowClass, kMaxLoadString);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
		return FALSE;

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_THREEFINGERDRAG));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	// Delete tray icon
	Shell_NotifyIcon(NIM_DELETE, &nid);
	return (int)msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THREEFINGERDRAG));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_THREEFINGERDRAG);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	                          CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd)
	{
		return FALSE;
	}

	BOOL result = RegisterTouchWindow(hWnd, 0);
	if (!result)
	{
		MessageBox(NULL, TEXT("Touch window couldn't be registered"), TEXT("Info"), MB_OK);
		return FALSE;
	}

	RAWINPUTDEVICE rid;

	rid.usUsagePage = 0x0D;
	rid.usUsage = 0x05;

	rid.dwFlags = RIDEV_INPUTSINK; // Receive input even when the application is in the background
	rid.hwndTarget = hWnd; // Handle to the application window

	if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
	{
		// Error handling code
		MessageBox(NULL, TEXT("Raw touch device couldn't be registered"), TEXT("Info"), MB_OK);
		return FALSE;
	}


	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_USER + 1;
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THREEFINGERDRAG));
	lstrcpy(nid.szTip, TEXT("Three Finger Drag"));

	Shell_NotifyIcon(NIM_ADD, &nid);

	return TRUE;
}


void ReadPrecisionTouchPadRegistry()
{
	// Open the Precision Touchpad key in the registry
	HKEY touchPadKey;
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad", 0,
	                           KEY_READ, &touchPadKey);
	if (result != ERROR_SUCCESS)
	{
		std::cerr << "Failed to open Precision Touchpad cursor speed key." << std::endl;
		return;
	}

	// Read the cursor speed value from the registry
	DWORD cursorSpeed = 0;
	DWORD dataSize = sizeof(DWORD);
	result = RegQueryValueEx(touchPadKey, L"CursorSpeed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&cursorSpeed),
	                         &dataSize);
	if (result != ERROR_SUCCESS)
	{
		std::cerr << "Failed to read cursor speed value from registry." << std::endl;
		RegCloseKey(touchPadKey);
		return;
	}
	precision_touch_cursor_speed = cursorSpeed * 5 / 100.0f;
}


void SimulateClick(DWORD flags)
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


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INPUT:
		{
			HRAWINPUT hRawInput = (HRAWINPUT)lParam;
			UINT size = sizeof(RAWINPUT);
			HANDLE hHeap = GetProcessHeap();
			GetRawInputData(hRawInput, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
			if (size == 0)
				return 0;

			RAWINPUT* raw_input = (RAWINPUT*)HeapAlloc(hHeap, 0, size);
			if (raw_input == NULL)
				return 0;

			if (GetRawInputData(hRawInput, RID_INPUT, raw_input, &size, sizeof(RAWINPUTHEADER)) == -1)
			{
				HeapFree(hHeap, 0, raw_input);
				return 0;
			}

			UINT buffer_size;
			GetRawInputDeviceInfo(raw_input->header.hDevice, RIDI_PREPARSEDDATA, NULL, &buffer_size);
			if (buffer_size == 0)
				return 0;

			PHIDP_PREPARSED_DATA pre_parsed_data = (PHIDP_PREPARSED_DATA)HeapAlloc(hHeap, 0, buffer_size);
			GetRawInputDeviceInfo(raw_input->header.hDevice, RIDI_PREPARSEDDATA, pre_parsed_data, &buffer_size);
			HIDP_CAPS caps;
			if (HidP_GetCaps(pre_parsed_data, &caps) != HIDP_STATUS_SUCCESS)
			{
				HeapFree(hHeap, 0, raw_input);
				HeapFree(hHeap, 0, pre_parsed_data);
				return 0;
			}

			USHORT length = caps.NumberInputValueCaps;
			PHIDP_VALUE_CAPS value_caps = (PHIDP_VALUE_CAPS)HeapAlloc(hHeap, 0, sizeof(HIDP_VALUE_CAPS) * length);
			if (HidP_GetValueCaps(HidP_Input, value_caps, &length, pre_parsed_data) != HIDP_STATUS_SUCCESS)
			{
				HeapFree(hHeap, 0, raw_input);
				HeapFree(hHeap, 0, pre_parsed_data);
				HeapFree(hHeap, 0, value_caps);
				return 0;
			}

			ULONG value;
			UINT scan_time = 0;
			UINT contact_count = 0;

			TouchPadContact contact{ kInitValue, kInitValue, kInitValue };
			bool valid_contact_count = false;
			std::vector<TouchPadContact> contacts;
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
				// PHIDP_VALUE_CAPS valueCap = &valueCaps[i];
				USAGE usagePage = value_caps[i].UsagePage, usage = value_caps[i].Range.UsageMin;
				switch (value_caps[i].LinkCollection)
				{
				case 0:
					if (usagePage == 0x0D && usage == 0x56)
						scan_time = value;
					else if (usagePage == 0x0D && usage == 0x54)
					{
						contact_count = value;
						// If three fingers are touching the touchpad, start drag
						if (contact_count == 3)
							valid_contact_count = true;
					}
					break;
				default:
					if (usagePage == 0x0D && usage == 0x51)
						contact.contactId = static_cast<int>(value);
					else if (usagePage == 0x01 && usage == 0x30)
						contact.x = static_cast<int>(value);
					else if (usagePage == 0x01 && usage == 0x31)
						contact.y = static_cast<int>(value);
					break;
				}

				if (contact.contactId != kInitValue && contact.x != kInitValue && contact.y != kInitValue)
				{
					contacts.emplace_back(contact);
					contact = { kInitValue, kInitValue, kInitValue };
				}
			}

			if (valid_contact_count && contacts.size() > 2)
			{
				std::chrono::time_point<std::chrono::steady_clock> current = std::chrono::high_resolution_clock::now();
				std::chrono::duration<float> duration = current - last_detection;

				float ms = duration.count() * 1000.0f;

				// Calculate the vector between the last known contacts and the current contacts
				int dx = 0;
				int dy = 0;

				// Ignore initial movement frames. This prevents unwanted movement in the beginning of the gesture.
				if (++num_frames > kNumIgnoredFrames && !last_contacts.empty() && ms < kThresholdMs)
				{
					// Use the average position of the current contacts and the last known contacts
					int current_x = (contacts[0].x + contacts[1].x + contacts[2].x) / 3;
					int current_y = (contacts[0].y + contacts[1].y + contacts[2].y) / 3;
					int last_x = (last_contacts[0].x + last_contacts[1].x + last_contacts[2].x) / 3;
					int last_y = (last_contacts[0].y + last_contacts[1].y + last_contacts[2].y) / 3;

					dx = (current_x - last_x) * precision_touch_cursor_speed;
					dy = (current_y - last_y) * precision_touch_cursor_speed;
				}

				// Move the mouse pointer based on the calculated vector
				INPUT input;
				input.type = INPUT_MOUSE;
				input.mi.dx = dx;
				input.mi.dy = dy;
				input.mi.mouseData = 0;
				input.mi.dwFlags = MOUSEEVENTF_MOVE;
				input.mi.time = 0;
				input.mi.dwExtraInfo = 0;
				auto a = std::async(std::launch::async, SendInput, 1, &input, sizeof(INPUT));
				if (!is_dragging)
				{
					auto a1 = std::async(std::launch::async, SimulateClick, MOUSEEVENTF_LEFTDOWN);
					is_dragging = true;
				}

				// Update old information to current
				last_contacts = contacts;
				last_detection = current;
			}

			if (contact_count < 3)
			{
				if (is_dragging)
				{
					auto a = std::async(std::launch::async, SimulateClick, MOUSEEVENTF_LEFTUP);
					is_dragging = false;
				}
				contacts.clear();
				num_frames = 0;
			}

			HeapFree(hHeap, 0, raw_input);
			HeapFree(hHeap, 0, pre_parsed_data);
			HeapFree(hHeap, 0, value_caps);
			return 0;
		}


	case WM_USER + 1:
		switch (lParam)
		{
		case WM_RBUTTONDOWN:
			{
				POINT pt;
				GetCursorPos(&pt);
				HMENU hMenu = CreatePopupMenu();
				AppendMenu(hMenu, MF_STRING, 1, TEXT("Quit"));
				SetForegroundWindow(hWnd);
				TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
				PostMessage(hWnd, WM_NULL, 0, 0);
				DestroyMenu(hMenu);
			}
			break;

		case WM_LBUTTONDOWN:
			MessageBox(NULL, TEXT("Tray Icon Clicked"), TEXT("Info"), MB_OK);
			DestroyWindow(hWnd);
			break;
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
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

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
