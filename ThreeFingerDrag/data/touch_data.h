#pragma once
#include <vector>

struct TouchContact
{
    int contact_id;
    int x;
    int y;
    mutable bool on_surface;
    bool has_x_bounds;
    bool has_y_bounds;
    int minimum_x;
    int maximum_x;
    int minimum_y;
    int maximum_y;
};

struct TouchInputData
{
    std::vector<TouchContact> contacts;
    int contact_count = 0;
    bool can_perform_gesture = false;
};
