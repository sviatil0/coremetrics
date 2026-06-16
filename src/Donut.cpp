#include "Donut.hpp"

#include <cmath>
#include <sstream>
#include <iomanip>

namespace
{
    // pi as a float constant; std::numbers::pi is double in <numbers> but
    // explicit constexpr keeps this header-light and avoids implicit narrowing.
    constexpr float DONUT_PI = 3.14159265358979323846f;

    // Approximate glyph width used by the bitmap font in screen drawText.
    // 8px per character matches the Bar/Label visual cadence and keeps the
    // center label visually centered without measuring real font metrics.
    constexpr int DONUT_GLYPH_WIDTH = 8;

    // Glyph height approximation for vertical centering of the label.
    constexpr int DONUT_GLYPH_HEIGHT = 8;
}

Donut::Donut(ivec2 center, int outerRadius, int innerRadius,
             vec3 fillColor, vec3 trackColor, vec3 labelColor,
             float minVal, float maxVal)
    : center(center), outerRadius(outerRadius), innerRadius(innerRadius),
      value(minVal), minVal(minVal), maxVal(maxVal),
      fillColor(fillColor), trackColor(trackColor), labelColor(labelColor),
      centerLabel("")
{
}

void Donut::setValue(float v)
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

void Donut::setLabel(std::string label)
{
    centerLabel = label;
}

float Donut::getValue() const
{
    return value;
}

const std::string& Donut::getLabel() const
{
    return centerLabel;
}

ivec2 Donut::getCenter() const
{
    return center;
}

int Donut::getOuterRadius() const
{
    return outerRadius;
}

int Donut::getInnerRadius() const
{
    return innerRadius;
}

std::string Donut::formatPct(float v) const
{
    float range = maxVal - minVal;
    float pct = 0.0f;
    if (range > 0.0f)
    {
        pct = (v - minVal) / range * 100.0f;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0) << pct;
    return oss.str();
}

void Donut::draw(Screen &screen)
{
    if (outerRadius <= 0)
    {
        return;
    }

    int inner = innerRadius;
    if (inner < 0)
    {
        inner = 0;
    }
    if (inner > outerRadius)
    {
        inner = outerRadius;
    }

    int outerSq = outerRadius * outerRadius;
    int innerSq = inner * inner;

    float range = maxVal - minVal;
    float ratio = 0.0f;
    if (range > 0.0f)
    {
        ratio = (value - minVal) / range;
    }
    if (ratio < 0.0f)
    {
        ratio = 0.0f;
    }
    if (ratio > 1.0f)
    {
        ratio = 1.0f;
    }
    float sweep = ratio * 2.0f * DONUT_PI;

    // Walk the axis-aligned bounding box of the donut once. For every pixel
    // inside the ring band, decide whether it lies within the filled arc
    // (measured clockwise from 12 o'clock) and color accordingly. This is
    // the same per-pixel discipline Bar uses for its slab; nothing here
    // needs a new rasterizer primitive.
    for (int dy = -outerRadius; dy <= outerRadius; ++dy)
    {
        for (int dx = -outerRadius; dx <= outerRadius; ++dx)
        {
            int distSq = dx * dx + dy * dy;
            if (distSq > outerSq)
            {
                continue;
            }
            if (distSq < innerSq)
            {
                continue;
            }

            // atan2 with these args yields angle measured clockwise from
            // the top: angle 0 is straight up, angle increases as we go
            // right. Range is [0, 2*pi).
            float angle = std::atan2(static_cast<float>(dx), static_cast<float>(-dy));
            if (angle < 0.0f)
            {
                angle += 2.0f * DONUT_PI;
            }

            ivec2 pos(center.x + dx, center.y + dy);
            if (angle <= sweep && sweep > 0.0f)
            {
                screen.drawPixel(pos, fillColor);
            }
            else
            {
                screen.drawPixel(pos, trackColor);
            }
        }
    }

    std::string text = centerLabel;
    if (text.empty())
    {
        text = formatPct(value) + "%";
    }

    int textWidth = DONUT_GLYPH_WIDTH * static_cast<int>(text.size());
    ivec2 textPos(center.x - textWidth / 2, center.y - DONUT_GLYPH_HEIGHT / 2);
    screen.drawText(textPos, labelColor, text);
}
