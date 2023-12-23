#include "logger.h"
#include <sstream>
#include <iomanip>
#include <ctime>

Logger& Logger::GetInstance()
{
    static Logger instance("ThreeFingerDrag", "log.txt");
    return instance;
}

Logger::Logger(const std::string& localAppDataFolderName, const std::string& logFileName)
{
    size_t len;
    char* env_path;
    const errno_t err = _dupenv_s(&env_path, &len, "LOCALAPPDATA");

    // If the environment variable exists and can be retrieved successfully, use it to construct the path to the log file.
    if (err == 0 && env_path != nullptr)
    {
        log_file_path_ = std::string(env_path);
        log_file_path_ += "\\";
        log_file_path_ += localAppDataFolderName;

        // Check if the folder exists, and create it if necessary
        if (!std::filesystem::exists(log_file_path_))
            std::filesystem::create_directory(log_file_path_);

        log_file_path_ += "\\" + logFileName;

        // Check if the log file exists, and create it if necessary
        if (!std::filesystem::exists(log_file_path_))
        {
            std::ofstream file(log_file_path_, std::ios::out);
            file.close();
        }

        log_file_.open(log_file_path_, std::ios::out | std::ios::app);
        free(env_path);
    }
}

void Logger::WriteLog(const std::string& type, const std::string& message)
{
    // Check if the log file has exceeded the threshold size of 5 MB
    const std::streampos current_pos = log_file_.tellp();
    const std::streampos max_size = 5 * 1024 * 1024; // 5MB
    if (current_pos >= max_size)
    {
        // Truncate the log file to zero size
        log_file_.close();
        std::ofstream file(log_file_path_, std::ios::out | std::ios::trunc);
        file.close();
    }

    // Get the current system time
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    // Convert the time to a string
    std::tm time_info;
    if (localtime_s(&time_info, &now_time) == 0)
    {
        std::stringstream ss;
        ss << std::put_time(&time_info, "%y-%m-%d %H:%M:%S");
        const std::string timestamp_str = ss.str();

        // Write the log message to the file with the timestamp
        if (!type.empty())
            log_file_ << timestamp_str << " " << type << " " << message << '\n';
        else
            log_file_ << timestamp_str << ": " << message << '\n';

    }
    else
    {
        // Write the log message to the file without the timestamp
        if (!type.empty())
            log_file_ << type << " - " << message << '\n';
        else
            log_file_ << ": " <<  message << '\n';
    }
}

void Logger::Info(const std::string& message)
{
    WriteLog("[INFO]", message);
}

void Logger::Debug(const std::string& message)
{
    WriteLog("[DEBUG]", message);
}

void Logger::Warning(const std::string& message)
{
    WriteLog("[WARNING]", message);
}

void Logger::Error(const std::string& message)
{
    WriteLog("[ERROR]", message);
}

Logger::~Logger()
{
    log_file_.close();
}
