#include "Bar.hpp"
#include "Thresholds.hpp"

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
    if (ratio > Thresholds::RED_RATIO)
    {
        activeFill = Thresholds::colorRed();
    }
    else if (ratio > Thresholds::YELLOW_RATIO)
    {
        activeFill = Thresholds::colorYellow();
    }

    ivec2 fillMax(minPos.x + fillWidth, maxPos.y);
    screen.drawBox(minPos, fillMax, activeFill);

    // Cheap fake-depth: a 2px highlight strip along the top and a 2px shade
    // strip along the bottom of the filled portion. No new rasterizer
    // primitives, no shaders; just two scaled drawBox overlays that lift
    // the flat slab into a pseudo-3D pill.
    int barHeight = maxPos.y - minPos.y;
    if (barHeight >= 6)
    {
        vec3 highlight(activeFill.x * 1.15f, activeFill.y * 1.15f, activeFill.z * 1.15f);
        if (highlight.x > 1.0f) { highlight.x = 1.0f; }
        if (highlight.y > 1.0f) { highlight.y = 1.0f; }
        if (highlight.z > 1.0f) { highlight.z = 1.0f; }
        vec3 shade(activeFill.x * 0.75f, activeFill.y * 0.75f, activeFill.z * 0.75f);
        ivec2 hiMin(minPos.x, minPos.y);
        ivec2 hiMax(minPos.x + fillWidth, minPos.y + 2);
        screen.drawBox(hiMin, hiMax, highlight);
        ivec2 shMin(minPos.x, maxPos.y - 2);
        ivec2 shMax(minPos.x + fillWidth, maxPos.y);
        screen.drawBox(shMin, shMax, shade);
    }
}
