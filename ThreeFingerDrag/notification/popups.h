#pragma once
#include "../logging/logger.h"
#include "../framework.h"

/**
 * \brief A class for displaying various popups in the application.
 */
class Popups
{
public:
    /**
     * \brief Displays an error message box with the given message and logs it as an error.
     * \param message The message to be displayed.
     * \param logger A pointer to the logger object used for logging the error message.
     */
    static void DisplayErrorMessage(const std::string& message);
    static void DisplayInfoMessage(const std::string& message);
    static bool DisplayPrompt(const std::string& message, const std::string& title);
    static bool DisplayWarningPrompt(const std::string& message, const std::string& title);
    static void ShowToastNotification(const std::wstring& message, const std::wstring& title);
};

