#include "Toggle.hpp"
#include "ClickEvent.hpp"

// Inset between the pill outer edge and the knob, in pixels. The knob is
// centered vertically inside the pill and slides horizontally between
// the left and right edge with this much padding on every side.
static constexpr int TOGGLE_INSET = 2;

Toggle::Toggle(ivec2 minP, ivec2 maxP, bool initialState,
               vec3 onCol, vec3 offCol, vec3 knobCol)
    : minPos(minP), maxPos(maxP), state(initialState),
      onColor(onCol), offColor(offCol), knobColor(knobCol)
{
}

bool Toggle::getState() const
{
    return state;
}

void Toggle::setState(bool newState)
{
    state = newState;
}

void Toggle::draw(Screen& screen)
{
    // Pill background: solid block in the active color so the widget
    // reads as on/off at a glance even without the knob position cue.
    vec3 bodyColor = state ? onColor : offColor;
    screen.drawBox(minPos, maxPos, bodyColor);

    // Knob: a smaller square that slides between the right edge (on)
    // and the left edge (off), with TOGGLE_INSET pixels of padding on
    // every side of the pill so the knob never touches the rim.
    int pillHeight = maxPos.y - minPos.y;
    int knobSize = pillHeight - (2 * TOGGLE_INSET);
    if (knobSize < 1)
    {
        knobSize = 1;
    }

    int knobMinX;
    if (state)
    {
        knobMinX = maxPos.x - knobSize - TOGGLE_INSET;
    }
    else
    {
        knobMinX = minPos.x + TOGGLE_INSET;
    }
    int knobMinY = minPos.y + TOGGLE_INSET;

    ivec2 knobMin(knobMinX, knobMinY);
    ivec2 knobMax(knobMinX + knobSize, knobMinY + knobSize);
    screen.drawBox(knobMin, knobMax, knobColor);
}

bool Toggle::checkToggle(int mouseX, int mouseY) const
{
    if (((mouseX >= minPos.x) && (mouseX <= maxPos.x))
        && ((mouseY >= minPos.y) && (mouseY <= maxPos.y)))
    {
        return true;
    }
    return false;
}

bool Toggle::operator()(Event* event)
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

    state = !state;
    return true;
}
