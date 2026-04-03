#ifndef __EVENTMANAGER_HPP__
#define __EVENTMANAGER_HPP__

#include <queue>
#include <memory>
#include "Event.hpp"
#include "Tree.hpp"
#include "Layout.hpp"
#include "vec2.hpp"

class EventManager
{
private:
    std::queue<std::unique_ptr<Event>> eventQueue;

    EventManager() = default;
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    static bool trickleEvent(Tree<Layout>& node, Event* event, ivec2 parentStart, ivec2 parentEnd);
    static Tree<Layout>* findLayoutByName(Tree<Layout>& node, const std::string& name);

public:
    static EventManager& getInstance();

    void pushEvent(std::unique_ptr<Event> event);
    void processEvents(ivec2 screenStart, ivec2 screenEnd);
};

#endif
