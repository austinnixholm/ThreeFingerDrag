// ThreeFingerDrag.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <Windows.h>
#include "framework.h"
#include "ThreeFingerDrag.h"
#include <vector>
#include <chrono>
#include <cassert>
#include <future>
#include <mutex>

#define ASSERT assert
#define THRESHOLD_MS 50
#define IGNORED_FRAMES 5
#define MAX_LOADSTRING 100


// Global Variables:
HINSTANCE hInst; // current instance
WCHAR szTitle[MAX_LOADSTRING]; // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING]; // the main window class name

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void ReadRegistry();

NOTIFYICONDATA nid;

struct TouchPadContact
{
	int contactId;
	int x;
	int y;
};

std::chrono::time_point<std::chrono::steady_clock> lastContactUpdate;
std::vector<TouchPadContact> lastContacts;
std::mutex m;

INT frames = 0;
DOUBLE touchCursorSpeed;
// BOOL ignoreFirstMovement = true;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Read any required values from registry
	ReadRegistry();

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_THREEFINGERDRAG, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

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

	//
	rid.usUsagePage = 0x0D;
	rid.usUsage = 0x05; // Mouse

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

bool isTouching = false;
bool isDragging = false;

POINT lastTouchPos = {0, 0};

void ReadRegistry()
{
	// Open the Precision Touchpad key in the registry
	HKEY touchPadKey;
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad", 0, KEY_READ, &touchPadKey);
	if (result != ERROR_SUCCESS) {
		std::cerr << "Failed to open Precision Touchpad cursor speed key." << std::endl;
		return;
	}

	// Read the cursor speed value from the registry
	DWORD cursorSpeed = 0;
	DWORD dataSize = sizeof(DWORD);
	result = RegQueryValueEx(touchPadKey, L"CursorSpeed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&cursorSpeed), &dataSize);
	if (result != ERROR_SUCCESS) {
		std::cerr << "Failed to read cursor speed value from registry." << std::endl;
		RegCloseKey(touchPadKey);
		return;
	}
	touchCursorSpeed = cursorSpeed * 5 / 100.0f;
}


void SimulateClick(DWORD flags)
{
	std::lock_guard<std::mutex> lk(m);
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
			{
				return 0;
			}
			RAWINPUT* rawInput = (RAWINPUT*)HeapAlloc(hHeap, 0, size);

			if (rawInput == NULL)
			{
				return 0;
			}

			if (GetRawInputData(hRawInput, RID_INPUT, rawInput, &size, sizeof(RAWINPUTHEADER)) == -1)
			{
				HeapFree(hHeap, 0, rawInput);
				return 0;
			}

			UINT bufferSize;
			GetRawInputDeviceInfo(rawInput->header.hDevice, RIDI_PREPARSEDDATA, NULL, &bufferSize);
			if (bufferSize == 0)
			{
				return 0;
			}

			PHIDP_PREPARSED_DATA preParsedData = (PHIDP_PREPARSED_DATA)HeapAlloc(hHeap, 0, bufferSize);
			GetRawInputDeviceInfo(rawInput->header.hDevice, RIDI_PREPARSEDDATA, preParsedData, &bufferSize);
			HIDP_CAPS caps;
			if (HidP_GetCaps(preParsedData, &caps) != HIDP_STATUS_SUCCESS)
			{
				HeapFree(hHeap, 0, rawInput);
				HeapFree(hHeap, 0, preParsedData);
				return 0;
			}

			USHORT length = caps.NumberInputValueCaps;
			PHIDP_VALUE_CAPS valueCaps = (PHIDP_VALUE_CAPS)HeapAlloc(hHeap, 0, sizeof(HIDP_VALUE_CAPS) * length);
			if (HidP_GetValueCaps(HidP_Input, valueCaps, &length, preParsedData) != HIDP_STATUS_SUCCESS)
			{
				HeapFree(hHeap, 0, rawInput);
				HeapFree(hHeap, 0, preParsedData);
				HeapFree(hHeap, 0, valueCaps);
				return 0;
			}

			ULONG value;
			UINT scanTime = 0;
			UINT contactCount = 0;

			TouchPadContact contact{-9999, -9999, -9999};
			bool attemptDrag = false;
			std::vector<TouchPadContact> contacts;
			for (USHORT i = 0; i < length; i++)
			{
				if (HidP_GetUsageValue(
					HidP_Input,
					valueCaps[i].UsagePage,
					valueCaps[i].LinkCollection,
					valueCaps[i].NotRange.Usage,
					&value,
					preParsedData,
					(PCHAR)rawInput->data.hid.bRawData,
					rawInput->data.hid.dwSizeHid
				) != HIDP_STATUS_SUCCESS)
				{
					continue;
				}
				// PHIDP_VALUE_CAPS valueCap = &valueCaps[i];
				USAGE usagePage = valueCaps[i].UsagePage, usage = valueCaps[i].Range.UsageMin;
				switch (valueCaps[i].LinkCollection)
				{
				case 0:
					if (usagePage == 0x0D && usage == 0x56)
						scanTime = value;
					else if (usagePage == 0x0D && usage == 0x54) {
						contactCount = value;
						// If three fingers are touching the touchpad, start drag
						if (contactCount == 3) 
							attemptDrag = true;
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
				
				if (contact.contactId != -9999 && contact.x != -9999 && contact.y != -9999)
				{
					contacts.emplace_back(contact);
					contact = {-9999, -9999, -9999};
				}

			}

			if (attemptDrag && contacts.size() > 2)
			{
				std::chrono::time_point<std::chrono::steady_clock> current = std::chrono::high_resolution_clock::now();
				std::chrono::duration<float> duration = current - lastContactUpdate;

				float ms = duration.count() * 1000.0f;

				// Calculate the vector between the last known contacts and the current contacts
				int dx = 0;
				int dy = 0;

				if (++frames > IGNORED_FRAMES && !lastContacts.empty() && ms < THRESHOLD_MS) {

					// Use the average position of the current contacts and the last known contacts
					int currentX = (contacts[0].x + contacts[1].x + contacts[2].x) / 3;
					int currentY = (contacts[0].y + contacts[1].y + contacts[2].y) / 3;


					int lastX = (lastContacts[0].x + lastContacts[1].x + lastContacts[2].x) / 3;
					int lastY = (lastContacts[0].y + lastContacts[1].y + lastContacts[2].y) / 3;
					
					dx = (currentX - lastX) * touchCursorSpeed;
					dy = (currentY - lastY) * touchCursorSpeed;
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
				if (!isDragging) {
					auto a1 = std::async(std::launch::async, SimulateClick, MOUSEEVENTF_LEFTDOWN);
					isDragging = true;
				}

				// Replace the last known contacts with the current contacts
				lastContacts = contacts;
				lastContactUpdate = current;
			}

			if (contactCount < 3)
			{
				// End drag when all fingers are lifted from the touchpad
				if (isDragging)
				{
					auto a = std::async(std::launch::async, SimulateClick, MOUSEEVENTF_LEFTUP);
					isDragging = false;
				}
				contacts.clear();
				frames = 0;
			}

			HeapFree(hHeap, 0, rawInput);
			HeapFree(hHeap, 0, preParsedData);
			HeapFree(hHeap, 0, valueCaps);
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
		if (!UnregisterTouchWindow(hWnd))
		{
			MessageBox(NULL, TEXT("Cannot unregister application window for touch input"), TEXT("Error"), MB_OK);
		}
		ASSERT(!IsTouchWindow(hWnd, NULL));
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
