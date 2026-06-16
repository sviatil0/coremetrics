#include "Slider.hpp"
#include "ClickEvent.hpp"

Slider::Slider(ivec2 minPos, ivec2 maxPos, vec3 trackColor, vec3 fillColor, vec3 handleColor,
               float minVal, float maxVal, int handleWidth)
    : minPos(minPos), maxPos(maxPos),
      value(minVal), minVal(minVal), maxVal(maxVal),
      handleWidth(handleWidth),
      trackColor(trackColor), fillColor(fillColor), handleColor(handleColor)
{
}

void Slider::setValue(float v)
{
    if (v < minVal)
    {
        value = minVal;
    }
    else if (v > maxVal)
    {
        value = maxVal;
    }
    else
    {
        value = v;
    }
}

float Slider::getValue() const
{
    return value;
}

float Slider::getMinVal() const
{
    return minVal;
}

float Slider::getMaxVal() const
{
    return maxVal;
}

int Slider::getHandleWidth() const
{
    return handleWidth;
}

ivec2 Slider::getMinPos() const
{
    return minPos;
}

ivec2 Slider::getMaxPos() const
{
    return maxPos;
}

int Slider::handleX() const
{
    float range = maxVal - minVal;
    if (range <= 0.0f)
    {
        return minPos.x;
    }

    float ratio = (value - minVal) / range;
    int trackWidth = maxPos.x - minPos.x;
    return minPos.x + static_cast<int>(ratio * static_cast<float>(trackWidth));
}

bool Slider::inHandleArea(int mouseX, int mouseY) const
{
    // Whole track is the hit zone so a click anywhere on it sets the value to
    // that x; the handle itself is just a visual cue.
    if (mouseX < minPos.x || mouseX > maxPos.x)
    {
        return false;
    }
    if (mouseY < minPos.y - 2 || mouseY > maxPos.y + 2)
    {
        return false;
    }
    return true;
}

void Slider::draw(Screen& screen)
{
    screen.drawBox(minPos, maxPos, trackColor);

    int hx = handleX();

    if (hx > minPos.x)
    {
        ivec2 fillMax(hx, maxPos.y);
        screen.drawBox(minPos, fillMax, fillColor);
    }

    ivec2 handleMin(hx - handleWidth / 2, minPos.y - 2);
    ivec2 handleMax(hx + handleWidth / 2, maxPos.y + 2);
    screen.drawBox(handleMin, handleMax, handleColor);
}

bool Slider::operator()(Event* event)
{
    if (event->getType() != EVENT_CLICK)
    {
        return false;
    }

    ClickEvent* click = static_cast<ClickEvent*>(event);
    int mouseX = click->getMouseX();
    int mouseY = click->getMouseY();

    if (!inHandleArea(mouseX, mouseY))
    {
        return false;
    }

    int trackWidth = maxPos.x - minPos.x;
    if (trackWidth <= 0)
    {
        return true;
    }

    float ratio = static_cast<float>(mouseX - minPos.x) / static_cast<float>(trackWidth);
    if (ratio < 0.0f)
    {
        ratio = 0.0f;
    }
    else if (ratio > 1.0f)
    {
        ratio = 1.0f;
    }

    setValue(minVal + ratio * (maxVal - minVal));
    return true;
}
