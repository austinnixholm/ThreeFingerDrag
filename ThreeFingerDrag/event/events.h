#pragma once
#include <functional>
#include <vector>

class EventArgs
{
public:
    virtual ~EventArgs()
    {
    }
};

template <typename T>
class Event
{
public:
    using EventHandler = std::function<void(T)>;

    void AddListener(EventHandler listener)
    {
        eventHandlers.push_back(listener);
    }

    void RemoveListener(EventHandler listener)
    {
        eventHandlers.erase(std::remove(eventHandlers.begin(), eventHandlers.end(), listener), eventHandlers.end());
    }

    void RaiseEvent(T args)
    {
        for (auto& handler : eventHandlers)
            handler(args);
    }

private:
    std::vector<EventHandler> eventHandlers;
};
