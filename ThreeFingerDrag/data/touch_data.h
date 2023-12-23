#pragma once
#include <vector>

struct TouchContact
{
    int contact_id;
    int x;
    int y;
    bool on_surface;
};

struct TouchInputData
{
    std::vector<TouchContact> contacts;
    int contact_count = 0;
    bool can_perform_gesture = false;
};
