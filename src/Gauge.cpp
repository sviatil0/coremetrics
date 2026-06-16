#include "Gauge.hpp"
#include "Thresholds.hpp"
#include <cmath>

namespace
{
    // Angle convention. We use the math convention theta in [-pi, 0] for
    // the visible (top) half of the dial:
    //   theta = -pi   -> left tip    (cos = -1, sin =  0)
    //   theta = -pi/2 -> top apex    (cos =  0, sin = -1)
    //   theta =  0    -> right tip   (cos =  1, sin =  0)
    // In screen space y grows downward, so sin(theta) being negative on
    // (-pi, 0) places the endpoint above the center, which is exactly
    // where the top of the dial lives. setValue's clamped output is
    // linearly remapped from [minValue, maxValue] to [-pi, 0].
    constexpr float PI_F = 3.14159265358979323846f;
    constexpr float ARC_START = -PI_F;
    constexpr float ARC_END = 0.0f;
}

Gauge::Gauge(ivec2 center, int radius, int arcThickness,
             vec3 arcColor, vec3 needleColor, vec3 bgColor,
             float minValue, float maxValue)
    : center(center),
      radius(radius),
      arcThickness(arcThickness),
      value(minValue),
      minValue(minValue),
      maxValue(maxValue),
      arcColor(arcColor),
      needleColor(needleColor),
      bgColor(bgColor)
{
}

void Gauge::setValue(float v)
{
    if (v < minValue)
    {
        value = minValue;
    }
    else if (v > maxValue)
    {
        value = maxValue;
    }
    else
    {
        value = v;
    }
}

float Gauge::getValue() const
{
    return value;
}

ivec2 Gauge::getCenter() const
{
    return center;
}

int Gauge::getRadius() const
{
    return radius;
}

int Gauge::getArcThickness() const
{
    return arcThickness;
}

void Gauge::draw(Screen &screen)
{
    if (radius <= 0 || arcThickness <= 0)
    {
        return;
    }

    float range = maxValue - minValue;
    if (range <= 0.0f)
    {
        return;
    }

    float ratio = (value - minValue) / range;
    if (ratio < 0.0f)
    {
        ratio = 0.0f;
    }
    if (ratio > 1.0f)
    {
        ratio = 1.0f;
    }

    float pct = ratio * 100.0f;
    vec3 filledColor = Thresholds::colorForPct(pct);

    // Linear lerp from the left tip (theta = -pi) to the right tip
    // (theta = 0). The arc sweeps clockwise across the top of the screen
    // as value rises, matching how a real speedometer reads.
    float theta = ARC_START + ratio * (ARC_END - ARC_START);

    int outer = radius;
    int inner = radius - arcThickness;
    if (inner < 0)
    {
        inner = 0;
    }
    int outerSq = outer * outer;
    int innerSq = inner * inner;

    // Pixel-scan the bounding box of the top semicircle. For each pixel
    // inside the annulus [inner, outer], decide arc-vs-background by
    // comparing its polar angle to the value's angle. Pixels below the
    // diameter (dy > 0) are skipped so only the top half is drawn; the
    // bottom half is left open the way a real dashboard speedometer is.
    // This mirrors the ProgressRing approach: no SDL circle primitive,
    // just per-pixel distance + angle predicates fed into
    // Screen::drawPixel.
    for (int dy = -outer; dy <= 0; ++dy)
    {
        for (int dx = -outer; dx <= outer; ++dx)
        {
            int distSq = dx * dx + dy * dy;
            if (distSq > outerSq || distSq < innerSq)
            {
                continue;
            }

            // atan2 returns (-pi, pi]; for the top half (dy <= 0) the
            // result lies in [-pi, 0], which is exactly our arc range.
            float pixelTheta = std::atan2(static_cast<float>(dy), static_cast<float>(dx));
            if (pixelTheta < ARC_START || pixelTheta > ARC_END)
            {
                continue;
            }

            vec3 pixelColor = bgColor;
            if (pixelTheta <= theta)
            {
                pixelColor = filledColor;
            }

            ivec2 p(center.x + dx, center.y + dy);
            screen.drawPixel(p, pixelColor);
        }
    }
    // arcColor is retained as a configured-but-currently-unused stylistic
    // override for future stroked-edge support; reference it once so
    // -Wunused-private-field stays quiet without changing behavior.
    (void)arcColor;

    // Needle. Endpoint sits on the arc's centerline at the value's angle.
    // 3px thickness is faked by drawing three parallel Bresenham lines at
    // perpendicular offsets +/-1px from the spine, the same trick the
    // sparkline uses for its 3px stroke. Perpendicular direction in
    // screen space is (-sin(theta), cos(theta)).
    float cosT = std::cos(theta);
    float sinT = std::sin(theta);
    int endX = center.x + static_cast<int>(cosT * static_cast<float>(radius));
    int endY = center.y + static_cast<int>(sinT * static_cast<float>(radius));

    int perpX = -static_cast<int>(std::round(sinT));
    int perpY = static_cast<int>(std::round(cosT));

    ivec2 a(center.x, center.y);
    ivec2 b(endX, endY);
    screen.drawLine(a, b, needleColor);

    ivec2 aPlus(center.x + perpX, center.y + perpY);
    ivec2 bPlus(endX + perpX, endY + perpY);
    screen.drawLine(aPlus, bPlus, needleColor);

    ivec2 aMinus(center.x - perpX, center.y - perpY);
    ivec2 bMinus(endX - perpX, endY - perpY);
    screen.drawLine(aMinus, bMinus, needleColor);
}
