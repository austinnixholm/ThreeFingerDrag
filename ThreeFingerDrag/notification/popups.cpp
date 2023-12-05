#include "popups.h"
#include "../logging/logger.h"
#include "wintoastlib.h"

using namespace WinToastLib;

class WinToastHandler : public WinToastLib::IWinToastHandler
{
public:
	WinToastHandler() {}
	// Public interfaces
	void toastActivated() const override {}
	void toastActivated(int actionIndex) const override {}
	void toastDismissed(WinToastDismissalReason state) const override {}
	void toastFailed() const override {}
};

/**
 * \brief Displays a message box popup with an error icon and message.
 * \param message The message to display.
 * \param logger A pointer to the logger object.
 * \remarks The sent message will be logged as an error.
 */
void Popups::DisplayErrorMessage(const std::string& message)
{
	const std::wstring temp = std::wstring(message.begin(), message.end());
	const LPCWSTR wstr_message = temp.c_str();

	MessageBox(nullptr, wstr_message, TEXT("Three Finger Drag"), MB_OK | MB_ICONERROR | MB_TOPMOST);
	ERROR(message);
}

/**
 * \brief Displays a message box popup with an info icon and message.
 * \param message The message to display.
 * \param logger A pointer to the logger object.
 * \remarks The sent message will be logged as an error.
 */
void Popups::DisplayInfoMessage(const std::string& message)
{
	const std::wstring temp = std::wstring(message.begin(), message.end());
	const LPCWSTR wstr_message = temp.c_str();

	MessageBox(nullptr, wstr_message, TEXT("Three Finger Drag"), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
	INFO(message);
}

/**
 * \brief Displays a message box popup with yes and no buttons and returns true if "yes" is clicked.
 * \param message The message to display.
 * \param title The title of the message box.
 * \return true if "yes" is clicked, false otherwise.
 */
bool Popups::DisplayPrompt(const std::string& message, const std::string& title)
{
	const std::wstring temp_message = std::wstring(message.begin(), message.end());
	const std::wstring temp_title = std::wstring(title.begin(), title.end());

	const int response = MessageBox(nullptr, temp_message.c_str(), temp_title.c_str(), MB_YESNO | MB_ICONQUESTION | MB_TOPMOST);
	return response == IDYES;
}

/**
 * \brief Displays a message box popup with yes and no buttons and returns true if "yes" is clicked.
 * \param message The message to display.
 * \param title The title of the message box.
 * \return true if "yes" is clicked, false otherwise.
 */
bool Popups::DisplayWarningPrompt(const std::string& message, const std::string& title)
{
	const std::wstring temp_message = std::wstring(message.begin(), message.end());
	const std::wstring temp_title = std::wstring(title.begin(), title.end());

	const int response = MessageBox(nullptr, temp_message.c_str(), temp_title.c_str(), MB_YESNO | MB_ICONWARNING | MB_TOPMOST);
	return response == IDYES;
}

void Popups::ShowToastNotification(const std::wstring& message, const std::wstring& title) {
	WinToastTemplate templ = WinToastTemplate(WinToastTemplate::Text02);
	templ.setTextField(title, WinToastTemplate::FirstLine);
	templ.setTextField(message, WinToastTemplate::SecondLine);
	WinToast::instance()->showToast(templ, new WinToastHandler());
}