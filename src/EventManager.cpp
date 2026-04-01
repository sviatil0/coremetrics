#include "EventManager.hpp"
#include "LayoutManager.hpp"
#include "SoundPlayer.hpp"
#include "ClickEvent.hpp"
#include "ShowEvent.hpp"
#include "SoundEvent.hpp"

EventManager& EventManager::getInstance()
{
    static EventManager instance;
    return instance;
}

void EventManager::pushEvent(std::unique_ptr<Event> event)
{
    eventQueue.push(std::move(event));
}

bool EventManager::trickleEvent(Tree<Layout>& node, Event* event, ivec2 parentStart, ivec2 parentEnd)
{
    Layout& layout = node.getData();
    if (!layout.isActive())
    {
        return false;
    }

    ivec2 absStart = layout.resolveAbsStart(parentStart, parentEnd);
    ivec2 absEnd = layout.resolveAbsEnd(parentStart, parentEnd);

    for (const auto& element : layout.elements)
    {
        if ((*element)(event))
        {
            return true;
        }
    }

    for (auto& child : node.getChildren())
    {
        if (trickleEvent(*child, event, absStart, absEnd))
        {
            return true;
        }
    }

    return false;
}

Tree<Layout>* EventManager::findLayoutByName(Tree<Layout>& node, const std::string& name)
{
    if (node.getData().getName() == name)
    {
        return &node;
    }
    for (auto& child : node.getChildren())
    {
        Tree<Layout>* found = findLayoutByName(*child, name);
        if (found != nullptr)
        {
            return found;
        }
    }
    return nullptr;
}

void EventManager::processEvents(ivec2 screenStart, ivec2 screenEnd)
{
    while (!eventQueue.empty())
    {
        std::unique_ptr<Event> event = std::move(eventQueue.front());
        eventQueue.pop();

        switch (event->getType())
        {
        case EVENT_CLICK:
        {
            trickleEvent(LayoutManager::getInstance().getRoot(), event.get(), screenStart, screenEnd);
            break;
        }
        case EVENT_SHOW:
        {
            ShowEvent* showEvent = static_cast<ShowEvent*>(event.get());
            Tree<Layout>* target = findLayoutByName(LayoutManager::getInstance().getRoot(), showEvent->getLayoutName());
            if (target != nullptr)
            {
                target->getData().setActive(showEvent->getShow());
            }
            break;
        }
        case EVENT_SOUND:
        {
            SoundEvent* soundEvent = static_cast<SoundEvent*>(event.get());
            SoundPlayer::getInstance().play(soundEvent->getFilePath());
            break;
        }
        }
    }
}
