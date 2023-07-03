#pragma once

#include <thread>
#include <iostream>
#include <sstream>
#include <Windows.h>

#include "resource.h"
#include "logger.h"
#include "touch_gestures.h"
#include "wintoastlib.h"
#include "popups.h"


namespace Application {

    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 0;
    constexpr int VERSION_PATCH = 5;

    constexpr char VERSION_FILE_NAME[] = "version.txt";

    std::string GetVersionString() {
        return std::to_string(VERSION_MAJOR) + "." + std::to_string(VERSION_MINOR) + "." + std::to_string(VERSION_PATCH);
    }

    /**
     * @brief Checks if a version.txt file exists in application data and updates it.
     * @return True if no version file was found
     */
    bool IsInitialStartup() {
        size_t len;
        char* env_path;
        const errno_t err = _dupenv_s(&env_path, &len, "LOCALAPPDATA");

        if (err == 0 && env_path != nullptr) {
            std::string version_file_path_ = std::string(env_path);
            version_file_path_ += "\\";
            version_file_path_ += "ThreeFingerDrag";

            // Create the application data directory if necessary
            if (!std::filesystem::exists(version_file_path_)) 
                std::filesystem::create_directory(version_file_path_);

            version_file_path_ += "\\";
            version_file_path_ += VERSION_FILE_NAME;

            // File contents do not need to update after initial creation
            if (std::filesystem::exists(version_file_path_))
                return false;

            // Create the file if it has not been created yet, and write the current version to it
            std::ofstream versionFile(version_file_path_);
            if (!versionFile)
                return false;

            versionFile << GetVersionString();
            versionFile.close();
        }

        return true;
    }
}

