#ifndef __CLICKEVENT_HPP__
#define __CLICKEVENT_HPP__

#include "Event.hpp"

class ClickEvent : public Event
{
private:
    int mouseX;
    int mouseY;

public:
    ClickEvent(int mouseX, int mouseY);
    int getMouseX() const;
    int getMouseY() const;
};

#endif
