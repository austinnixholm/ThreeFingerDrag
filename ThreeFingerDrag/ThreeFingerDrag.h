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


constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 3;

constexpr char kVersionFileName[] = "version.txt";

inline std::string GetVersionString() {
	return std::to_string(VERSION_MAJOR) + "." + std::to_string(VERSION_MINOR) + "." + std::to_string(VERSION_PATCH);
}

inline std::string GetVersionedTitle() {
    return "ThreeFingerDrag " + GetVersionString();
}

inline bool IsInitialStartup() {

    size_t len;
    char* env_path;
    const errno_t err = _dupenv_s(&env_path, &len, "LOCALAPPDATA" );

    if (err == 0 && env_path != nullptr) {
        std::string log_file_path_ = std::string(env_path);
        log_file_path_ += "\\";
        log_file_path_ += "ThreeFingerDrag";
        log_file_path_ += "\\";
        log_file_path_ += kVersionFileName;

        if (std::filesystem::exists(log_file_path_))
            return false;

        std::ofstream versionFile(log_file_path_);
        if (!versionFile)
            return false;

        versionFile << GetVersionString();
        versionFile.close();
    }

    return true;
}