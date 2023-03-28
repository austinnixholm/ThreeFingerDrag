#pragma once

#include "resource.h"
#include "Logger.h"

#include <vector>
#include <chrono>
#include <future>
#include <mutex>
#include <thread>
#include <cstring>

constexpr int kVersionMajor = 1;
constexpr int kVersionMinor = 0;
constexpr int kVersionPatch = 2;

inline std::string GetVersionString() {
	return std::to_string(kVersionMajor) + "." + std::to_string(kVersionMinor) + "." + std::to_string(kVersionPatch);
}

struct TouchPadContact
{
	int contact_id;
	int x;
	int y;
};

struct TouchPadInputData
{
	std::vector<TouchPadContact> contacts;
	int scan_time{};
	int contact_count{};
	bool can_perform_gesture{};
};