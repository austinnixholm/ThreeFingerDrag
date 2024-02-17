#pragma once
#include <iostream>
#include <filesystem>
#include <string>
#include <sstream>
#include <fstream>
#include "gesture/touch_processor.h"
#include "data/ini.h"
#include "config/globalconfig.h"

namespace Application
{
    constexpr bool RELEASE_BUILD = false;

    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 2;
    constexpr int VERSION_PATCH = 6;

    constexpr int SNAPSHOT_VERSION = 1;
    
    constexpr char VERSION_FILE_NAME[] = "version.txt";

    inline std::string GetVersionString()
    {
        std::string version = std::to_string(VERSION_MAJOR) + "." +
                              std::to_string(VERSION_MINOR) + "." +
                              std::to_string(VERSION_PATCH);

        // If it's not a release build, then it's a snapshot
        if (!RELEASE_BUILD)
            version += "." + std::to_string(SNAPSHOT_VERSION) + "-SNAPSHOT";
        
        return version;
    }

    inline GlobalConfig* config = GlobalConfig::GetInstance();

    inline std::string config_folder_path;

    /**
     * @brief Constructs and returns the path to application configuration files. If required, the directory will be created.
     * @return The local app data folder path
     */
    inline std::string GetConfigurationFolderPath()
    {
        size_t len;
        char* env_path;
        const errno_t err = _dupenv_s(&env_path, &len, "LOCALAPPDATA");

        auto directory_path = std::string(env_path);
        directory_path += "\\";
        directory_path += "ThreeFingerDrag";

        config_folder_path = directory_path;

        // Create the application data directory if necessary
        if (!std::filesystem::exists(directory_path))
            std::filesystem::create_directory(directory_path);
        return directory_path;
    }

    /**
     * @brief Checks if a version.txt file exists in application data and updates it.
     * @return True if no version file was found
     */
    inline bool IsInitialStartup()
    {
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

    inline void WriteConfiguration()
    {
        std::stringstream ss;
        std::string file_path = GetConfigurationFolderPath();
        
        file_path += "\\";
        file_path += "\\config.ini";

        mINI::INIFile file(file_path);
        mINI::INIStructure ini;

        ss << std::fixed << std::setprecision(2) << config->GetGestureSpeed();

        ini["Configuration"]["gesture_speed"] = ss.str();
        ini["Configuration"]["cancellation_delay_ms"] = std::to_string(config->GetCancellationDelayMs());
        ini["Configuration"]["debug"] = config->LogDebug() ? "true" : "false";

        if (!file.generate(ini))
            ERROR("Error writing to config file.");
    }

    inline void ReadConfiguration()
    {
        std::string file_path = GetConfigurationFolderPath();
        
        file_path += "\\";
        file_path += "\\config.ini";

        if (!std::filesystem::exists(file_path))
        {
            WriteConfiguration();
            return;
        }


        mINI::INIFile file(file_path);
        mINI::INIStructure ini;

        if (!file.read(ini))
        {
            std::stringstream ss;
            ss << "Couldn't read '" << file_path << "'";
            ERROR(ss.str());
            return;
        }

        const auto config_section = ini["Configuration"];

        if (config_section.has("gesture_speed"))
            config->SetGestureSpeed(std::stof(config_section.get("gesture_speed")));

        if (config_section.has("cancellation_delay_ms"))
            config->SetCancellationDelayMs(std::stof(config_section.get("cancellation_delay_ms")));

        if (config_section.has("debug"))
            config->SetLogDebug(config_section.get("debug") == "true");
    }

    inline std::filesystem::path ExePath()
    {
        wchar_t path[FILENAME_MAX] = {0};
        GetModuleFileNameW(nullptr, path, FILENAME_MAX);
        return std::filesystem::path(path);
    }
}
