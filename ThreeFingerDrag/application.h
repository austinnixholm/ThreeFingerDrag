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
    constexpr bool RELEASE_BUILD = true;

    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 2;
    constexpr int VERSION_PATCH = 6;

    constexpr int VERSION_REVISION = 0;
    
    constexpr char VERSION_FILE_NAME[] = "version.txt";

    inline std::string GetVersionString()
    {
        std::string version = std::to_string(VERSION_MAJOR) + "." +
                              std::to_string(VERSION_MINOR) + "." +
                              std::to_string(VERSION_PATCH);

        // If it's not a release build, then it's a snapshot
        if (!RELEASE_BUILD)
            version += "." + std::to_string(VERSION_REVISION) + "-SNAPSHOT";
        
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
        if (!config_folder_path.empty())
        {
            return config_folder_path;
        }
        
        if (config->IsPortableMode())
        {
            config_folder_path = std::filesystem::current_path().u8string();
            return config_folder_path;
        }
        
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
        ini["Configuration"]["automatic_timeout_delay_ms"] = std::to_string(config->GetAutomaticTimeoutDelayMs());
        ini["Configuration"]["one_finger_transition_delay_ms"] = std::to_string(config->GetOneFingerTransitionDelayMs());
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

        if (config_section.has("automatic_timeout_delay_ms"))
            config->SetAutomaticTimeoutDelayMs(std::stof(config_section.get("automatic_timeout_delay_ms")));
        
        if (config_section.has("one_finger_transition_delay_ms"))
            config->SetOneFingerTransitionDelayMs(std::stof(config_section.get("one_finger_transition_delay_ms")));
        
        if (config_section.has("debug"))
            config->SetLogDebug(config_section.get("debug") == "true");
    }

    inline std::filesystem::path ExePath()
    {
        wchar_t path[FILENAME_MAX] = {0};
        GetModuleFileNameW(nullptr, path, FILENAME_MAX);
        return std::filesystem::path(path);
    }

    inline void CheckPortableMode()
    {
        size_t len;
        char* env_path;
        const errno_t err = _dupenv_s(&env_path, &len, "LOCALAPPDATA");

        auto file_path = std::string(env_path);
        file_path += "\\";
        file_path += "tfd_install_location.txt";

        std::ifstream file;
        file.open(file_path);

        // No file was found, so the program probably wasn't installed
        if (!file.good())
        {
            config->SetPortableMode(true);
            return;
        }

        std::string firstLine;
        std::getline(file, firstLine);

        if (firstLine.empty())
        {
            return;
        }
        // Determine if the program is not running from its installed location
        config->SetPortableMode(ExePath().u8string().find(firstLine) == std::string::npos);
    }
    
}
