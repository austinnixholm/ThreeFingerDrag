#pragma once


#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>
#include <sstream>
#include <filesystem>

class Logger
{
public:
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;
	Logger(Logger&&) = default;
	Logger& operator=(Logger&&) = default;

	/**
	 * \brief Constructs the logger and opens the file for writing log lines to. 
	 * \param localAppDataFolder The application folder name.
	 * \remarks If the folder or file do not exist, they will be created.
	 */
	Logger(const std::string& localAppDataFolder)
	{
		size_t len;
		char* env_path;
		const errno_t err = _dupenv_s(&env_path, &len, "LOCALAPPDATA");

		// If the environment variable exists and can be retrieved successfully, use it to construct the path to the log file.
		if (err == 0 && env_path != nullptr)
		{
			log_file_path_ = std::string(env_path);
			log_file_path_ += "\\";
			log_file_path_ += localAppDataFolder;

			// Check if the folder exists, and create it if necessary
			if (!std::filesystem::exists(log_file_path_))
				std::filesystem::create_directory(log_file_path_);

			log_file_path_ += "\\log.txt";

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

	void Info(const std::string& message)
	{
		WriteLog("[INFO]", message);
	}

	void Debug(const std::string& message)
	{
		WriteLog("[DEBUG]", message);
	}

	void Warning(const std::string& message)
	{
		Info("[WARNING], message");
	}

	void Error(const std::string& message)
	{
		WriteLog("[ERROR]", message);
	}

	~Logger()
	{
		log_file_.close();
	}

private:
	std::ofstream log_file_;
	std::string log_file_path_;

	void WriteLog(const std::string& type, const std::string& message)
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
			log_file_ << timestamp_str << " " << type << " " << message << std::endl;
		}
		else
		{
			// Write the log message to the file without the timestamp
			log_file_ << type << " - " << message << std::endl;
		}
	}
};
