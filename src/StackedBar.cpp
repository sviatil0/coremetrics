#include "StackedBar.hpp"

StackedBar::StackedBar(ivec2 minPos, ivec2 maxPos, vec3 bgColor, float total)
    : minPos(minPos), maxPos(maxPos), segments(), total(total), bgColor(bgColor)
{
}

void StackedBar::addSegment(float value, vec3 color)
{
    segments.emplace_back(value, color);
}

void StackedBar::clearSegments()
{
    segments.clear();
}

void StackedBar::setTotal(float t)
{
    total = t;
}

float StackedBar::getTotal() const
{
    return total;
}

std::size_t StackedBar::getSegmentCount() const
{
    return segments.size();
}

ivec2 StackedBar::getMinPos() const
{
    return minPos;
}

ivec2 StackedBar::getMaxPos() const
{
    return maxPos;
}

void StackedBar::draw(Screen &screen)
{
    screen.drawBox(minPos, maxPos, bgColor);

    if (total <= 0.0f)
    {
        return;
    }

    int totalWidth = maxPos.x - minPos.x;
    if (totalWidth <= 0)
    {
        return;
    }

    int cursorX = minPos.x;
    int rightEdge = maxPos.x;
    for (std::size_t i = 0; i < segments.size(); ++i)
    {
        float value = segments[i].first;
        if (value <= 0.0f)
        {
            continue;
        }

        float ratio = value / total;
        int segWidth = static_cast<int>(ratio * static_cast<float>(totalWidth));
        if (segWidth <= 0)
        {
            continue;
        }

        int segEnd = cursorX + segWidth;
        if (segEnd > rightEdge)
        {
            segEnd = rightEdge;
        }

        ivec2 segMin(cursorX, minPos.y);
        ivec2 segMax(segEnd, maxPos.y);
        screen.drawBox(segMin, segMax, segments[i].second);

        cursorX = segEnd;
        if (cursorX >= rightEdge)
        {
            break;
        }
    }
}
