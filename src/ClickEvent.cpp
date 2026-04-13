#include "ClickEvent.hpp"

ClickEvent::ClickEvent(int mouseX, int mouseY) : Event(EVENT_CLICK), mouseX(mouseX), mouseY(mouseY)
{
}

int ClickEvent::getMouseX() const
{
    return mouseX;
}

int ClickEvent::getMouseY() const
{
    return mouseY;
}
