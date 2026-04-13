#ifndef __EVENT_HPP__
#define __EVENT_HPP__

enum EventType
{
    EVENT_CLICK,
    EVENT_SHOW,
    EVENT_SOUND
};

class Event
{
protected:
    EventType type;

public:
    Event(EventType type);
    virtual ~Event() = default;
    EventType getType() const;
};

#endif
