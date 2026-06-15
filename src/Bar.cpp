#include "Bar.hpp"

Bar::Bar(ivec2 minPos, ivec2 maxPos, vec3 fillColor, vec3 bgColor,
         float minVal, float maxVal, std::string metricName)
    : minPos(minPos), maxPos(maxPos), fillColor(fillColor), bgColor(bgColor),
      value(minVal), minVal(minVal), maxVal(maxVal), metricName(metricName)
{
}

void Bar::setValue(float v)
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

void Bar::setFillColor(vec3 color)
{
    fillColor = color;
}

float Bar::getValue() const
{
    return value;
}

const std::string& Bar::getMetricName() const
{
    return metricName;
}

ivec2 Bar::getMinPos() const
{
    return minPos;
}

ivec2 Bar::getMaxPos() const
{
    return maxPos;
}

void Bar::draw(Screen &screen)
{
    screen.drawBox(minPos, maxPos, bgColor);

    float range = maxVal - minVal;
    if (range <= 0.0f)
    {
        return;
    }

    float ratio = (value - minVal) / range;
    int totalWidth = maxPos.x - minPos.x;
    int fillWidth = static_cast<int>(ratio * static_cast<float>(totalWidth));
    if (fillWidth <= 0)
    {
        return;
    }

    vec3 activeFill = fillColor;
    if (ratio > 0.8f)
    {
        activeFill = vec3(0.95f, 0.35f, 0.35f);
    }
    else if (ratio > 0.6f)
    {
        activeFill = vec3(0.95f, 0.82f, 0.40f);
    }

    ivec2 fillMax(minPos.x + fillWidth, maxPos.y);
    screen.drawBox(minPos, fillMax, activeFill);
}
