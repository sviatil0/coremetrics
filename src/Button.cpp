#include "Button.hpp"
#include "EventManager.hpp"
#include "ClickEvent.hpp"
#include "SoundEvent.hpp"
#include "ShowEvent.hpp"
#include "Theme.hpp"

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
    // Flat modern button: a single 1 px stroke in Theme::panelBorder()
    // wrapping a fill of the constructor color. The previous version
    // painted a pure-white outline plus a 2 px top highlight and 2 px
    // bottom shade to fake a raised pseudo-3D pill, but the white
    // outline clashed with the Tokyo Night palette (Pillar A1) and the
    // pseudo-3D fight against the flat 1 px panelBorder strokes that
    // the card system (Pillar A4) paints around metric cards. Keeping
    // the border color in lockstep with the cards means buttons read
    // as just-another-bordered-rect inside the same visual language.
    vec3 borderColor = Theme::panelBorder();
    screen.drawBox(minPos, maxPos, borderColor);

    ivec2 innerMin(minPos.x + 1, minPos.y + 1);
    ivec2 innerMax(maxPos.x - 1, maxPos.y - 1);
    screen.drawBox(innerMin, innerMax, color);
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
