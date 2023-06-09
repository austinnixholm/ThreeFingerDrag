#pragma once
#include <iostream>
#include <filesystem>
#include <string>
#include <sstream>
#include <fstream>
#include "touch_gestures.h"
#include "ini.h"
#include "globalconfig.h"

namespace Application {

    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 1;
    constexpr int VERSION_PATCH = 0;

    constexpr char VERSION_FILE_NAME[] = "version.txt";

    std::string GetVersionString() {
        return std::to_string(VERSION_MAJOR) + "." + std::to_string(VERSION_MINOR) + "." + std::to_string(VERSION_PATCH);
    }

    GlobalConfig* config = GlobalConfig::GetInstance();

    /**
     * @brief Constructs and returns the path to application configuration files. If required, the directory will be created.
     * @return The local app data folder path
     */
    std::string GetConfigurationFolderPath() {
        size_t len;
        char* env_path;
        const errno_t err = _dupenv_s(&env_path, &len, "LOCALAPPDATA");

        std::string directory_path = std::string(env_path);
        directory_path += "\\";
        directory_path += "ThreeFingerDrag";

        // Create the application data directory if necessary
        if (!std::filesystem::exists(directory_path))
            std::filesystem::create_directory(directory_path);
        return directory_path;
    }

    /**
     * @brief Checks if a version.txt file exists in application data and updates it.
     * @return True if no version file was found
     */
    bool IsInitialStartup() {

        std::string version_file_path_ = GetConfigurationFolderPath();

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

        return true;
    }

    void WriteConfiguration() {
        std::stringstream ss;
        std::string file_path = GetConfigurationFolderPath();
        file_path += "\\";
        file_path += "\\config.ini";


        mINI::INIFile file(file_path);
        mINI::INIStructure ini;

        ss << std::fixed << std::setprecision(2) << config->GetGestureSpeed();

        ini["Configuration"]["gesture_speed"] = ss.str();
        ini["Configuration"]["skipped_gesture_frames"] = std::to_string(config->GetSkippedGestureFrames());

        file.generate(ini);
    }

    void ReadConfiguration() {

        std::string file_path = GetConfigurationFolderPath();
        file_path += "\\";
        file_path += "\\config.ini";

        if (!std::filesystem::exists(file_path)) {
            WriteConfiguration();
            return;
        }

        mINI::INIFile file(file_path);
        mINI::INIStructure ini;

        if (!file.read(ini))
            return;

        const auto config_section = ini["Configuration"];

        if (config_section.has("gesture_speed")) 
            config->SetGestureSpeed(std::stof(config_section.get("gesture_speed")));
        
        if (config_section.has("skipped_gesture_frames"))
            config->SetSkippedGestureFrames(std::stof(config_section.get("skipped_gesture_frames")));
    }

}