#pragma once
#include "../logging/logger.h"

/**
 * \brief A class for displaying various popups in the application.
 */
class Popups
{
public:
    /**
     * \brief Displays an error message box with the given message and logs it as an error.
     * \param message The message to be displayed.
     */
    static void DisplayErrorMessage(const std::string& message);
    static void DisplayInfoMessage(const std::string& message);
    static bool DisplayPrompt(const std::string& message, const std::string& title);
    static bool DisplayWarningPrompt(const std::string& message, const std::string& title);
    static void ShowToastNotification(const std::wstring& message, const std::wstring& title);
};
