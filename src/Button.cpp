<<<<<<< HEAD:src/Button.cpp
#include "Button.hpp"
=======
#include "button.hpp"
#include "EventManager.hpp"
#include "ClickEvent.hpp"
#include "SoundEvent.hpp"
#include "ShowEvent.hpp"
>>>>>>> 4f9eeea89c76feba4db6dc7aad8a164f3b50c71d:src/button.cpp

Button::Button(ivec2 minP, ivec2 maxP, vec3 col,
               std::string soundFile, std::string targetLayout, std::string targetLayoutHide)
    : minPos(minP), maxPos(maxP), color(col),
      soundFile(std::move(soundFile)),
      targetLayout(std::move(targetLayout)),
      targetLayoutHide(std::move(targetLayoutHide))
{
}

void Button::draw(Screen& screen)
{
    //like selection element, draw white border around button element
    vec3 borderColor = { 1.0f, 1.0f, 1.0f }; // White border
    screen.drawBox(minPos, maxPos, borderColor);
    
    ivec2 innerMin = { minPos.x + 1, minPos.y + 1 };
    ivec2 innerMax = { maxPos.x - 1, maxPos.y - 1 };
    screen.drawBox(innerMin, innerMax, color);

    //probably add label implementation here later
}

bool Button::checkToggle(int mouseX, int mouseY)
{
    if (((mouseX >= minPos.x) && (mouseX <= maxPos.x))
        && ((mouseY >= minPos.y) && (mouseY <= maxPos.y)))
    {
        return true;
    }
    return false;
}

bool Button::operator()(Event* event)
{
    if (event->getType() != EVENT_CLICK)
    {
        return false;
    }

    ClickEvent* click = static_cast<ClickEvent*>(event);
    if (!checkToggle(click->getMouseX(), click->getMouseY()))
    {
        return false;
    }

    if (!soundFile.empty())
    {
        EventManager::getInstance().pushEvent(std::make_unique<SoundEvent>(soundFile));
    }
    if (!targetLayoutHide.empty())
    {
        EventManager::getInstance().pushEvent(std::make_unique<ShowEvent>(targetLayoutHide, false));
    }
    if (!targetLayout.empty())
    {
        EventManager::getInstance().pushEvent(std::make_unique<ShowEvent>(targetLayout, true));
    }

    return true;
}
