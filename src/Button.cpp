#include "Button.hpp"
#include "EventManager.hpp"
#include "ClickEvent.hpp"
#include "SoundEvent.hpp"
#include "ShowEvent.hpp"

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
    // 1px white outline + filled body, then a 2px top highlight and
    // 2px bottom shade on the inner fill so the button reads as a
    // raised pseudo-3D pill instead of a flat slab. Same recipe as
    // Bar::draw uses for the active fill so the visual language stays
    // consistent across widgets.
    vec3 borderColor(1.0f, 1.0f, 1.0f);
    screen.drawBox(minPos, maxPos, borderColor);

    ivec2 innerMin(minPos.x + 1, minPos.y + 1);
    ivec2 innerMax(maxPos.x - 1, maxPos.y - 1);
    screen.drawBox(innerMin, innerMax, color);

    int innerHeight = innerMax.y - innerMin.y;
    if (innerHeight >= 8)
    {
        vec3 highlight(color.x * 1.20f, color.y * 1.20f, color.z * 1.20f);
        if (highlight.x > 1.0f) { highlight.x = 1.0f; }
        if (highlight.y > 1.0f) { highlight.y = 1.0f; }
        if (highlight.z > 1.0f) { highlight.z = 1.0f; }
        vec3 shade(color.x * 0.70f, color.y * 0.70f, color.z * 0.70f);
        ivec2 hiMin(innerMin.x, innerMin.y);
        ivec2 hiMax(innerMax.x, innerMin.y + 2);
        screen.drawBox(hiMin, hiMax, highlight);
        ivec2 shMin(innerMin.x, innerMax.y - 2);
        ivec2 shMax(innerMax.x, innerMax.y);
        screen.drawBox(shMin, shMax, shade);
    }
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
