#include <vector>

struct TouchPoint
{
    int contact_id;
    int x;
    int y;
    bool onSurface;
};

struct TouchInputData
{
    std::vector<TouchPoint> contacts;
    int contact_count = 0;
    bool can_perform_gesture = false;
};
