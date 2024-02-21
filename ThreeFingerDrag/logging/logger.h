#pragma once

#include <fstream>
#include <chrono>
#include <string>
#include <filesystem>

/**
 * @brief A logger class for writing log messages to a file.
 */
class Logger
{
public:
    /**
     * @brief Writes an info log message to the file.
     * @param message The log message.
     */
    void Info(const std::string& message);

    /**
     * @brief Writes a debug log message to the file.
     * @param message The log message.
     */
    void Debug(const std::string& message);

    /**
     * @brief Writes a warning log message to the file.
     * @param message The log message.
     */
    void Warning(const std::string& message);

    /**
     * @brief Writes an error log message to the file.
     * @param message The log message.
     */
    void Error(const std::string& message);

    /**
     * @brief Destructor that closes the log file.
     */
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) noexcept = default;
    Logger& operator=(Logger&&) noexcept = default;

    /**
     * @brief Returns the singleton instance of the logger.
     * @return The logger instance.
     */
    static Logger& GetInstance();

private:
    /**
     * @brief Constructs the logger and opens the file for writing log lines to.
     *
     * If the folder or file do not exist, they will be created.
     */
    Logger(const std::string& logFileName);

    std::ofstream log_file_; ///< The log file stream.
    std::string log_file_path_; ///< The path to the log file.

    /**
     * @brief Writes a log message to the file with the specified log type.
     * @param type The log type, such as [INFO], [DEBUG], [WARNING], or [ERROR].
     * @param message The log message.
     */
    void WriteLog(const std::string& type, const std::string& message);
};

#define INFO(msg)       Logger::GetInstance().Info(msg)
#define DEBUG(msg)      Logger::GetInstance().Debug(msg)
#define WARNING(msg)    Logger::GetInstance().Warning(msg)
#define ERROR(msg)      Logger::GetInstance().Error(msg)
