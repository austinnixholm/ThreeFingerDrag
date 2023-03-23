#pragma once

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define VERSION STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_PATCH)

#include "resource.h"

#include <vector>
#include <chrono>
#include <future>
#include <mutex>
#include <thread>
#include <cstring>


struct TouchPadContact
{
	int contact_id;
	int x;
	int y;
};

struct TouchPadInputData
{
	std::vector<TouchPadContact> contacts;
	int scan_time;
	int contact_count;
	bool valid_contact;
};