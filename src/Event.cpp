#include "Event.hpp"

Event::Event(EventType type) : type(type)
{
}

EventType Event::getType() const
{
    return type;
}
