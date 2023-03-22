#pragma once

#include "resource.h"

#include <vector>
#include <chrono>
#include <future>
#include <mutex>

struct TouchPadContact
{
	int contactId;
	int x;
	int y;
};