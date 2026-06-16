#include "ProgressRing.hpp"
#include <cmath>

ProgressRing::ProgressRing(ivec2 center, int outerRadius, int innerRadius,
                           vec3 ringColor, vec3 trackColor,
                           float minValue, float maxValue)
    : center(center),
      outerRadius(outerRadius),
      innerRadius(innerRadius),
      value(minValue),
      minValue(minValue),
      maxValue(maxValue),
      ringColor(ringColor),
      trackColor(trackColor)
{
}

void ProgressRing::setValue(float v)
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

void ProgressRing::setRingColor(vec3 color)
{
    ringColor = color;
}

void ProgressRing::setTrackColor(vec3 color)
{
    trackColor = color;
}

float ProgressRing::getValue() const
{
    return value;
}

ivec2 ProgressRing::getCenter() const
{
    return center;
}

int ProgressRing::getOuterRadius() const
{
    return outerRadius;
}

int ProgressRing::getInnerRadius() const
{
    return innerRadius;
}

void ProgressRing::draw(Screen &screen)
{
    // Guard against degenerate geometry. A non-positive outerRadius, or an
    // innerRadius that meets or exceeds outerRadius, leaves no donut band
    // to fill, so the widget renders nothing rather than churn pixels.
    if (outerRadius <= 0)
    {
        return;
    }
    int innerR = innerRadius;
    if (innerR < 0)
    {
        innerR = 0;
    }
    if (innerR >= outerRadius)
    {
        return;
    }

    float range = maxValue - minValue;
    float ratio = 0.0f;
    if (range > 0.0f)
    {
        ratio = (value - minValue) / range;
    }
    if (ratio < 0.0f)
    {
        ratio = 0.0f;
    }
    if (ratio > 1.0f)
    {
        ratio = 1.0f;
    }

    const float twoPi = 6.28318530717958647692f;
    const float halfPi = 1.57079632679489661923f;
    float sweepLimit = ratio * twoPi;

    int outerSq = outerRadius * outerRadius;
    int innerSq = innerR * innerR;

    // Iterate every pixel in the donut's axis-aligned bounding box. For
    // each pixel we test the squared distance against the inner/outer
    // radii to keep the donut crisp without a per-pixel sqrt for the
    // hit test, then take atan2 only for pixels that survived the band
    // check. Pixels outside the band are skipped entirely so we never
    // overdraw whatever was already on the surface around the ring.
    for (int y = center.y - outerRadius; y <= center.y + outerRadius; ++y)
    {
        for (int x = center.x - outerRadius; x <= center.x + outerRadius; ++x)
        {
            int dx = x - center.x;
            int dy = y - center.y;
            int rSq = dx * dx + dy * dy;
            if (rSq > outerSq)
            {
                continue;
            }
            if (rSq < innerSq)
            {
                continue;
            }

            // Screen coordinates put y downward, so atan2(dy, dx) returns
            // -pi/2 at the top of the ring and increases as we sweep
            // visually clockwise (top -> right -> bottom -> left). Adding
            // pi/2 shifts the start angle to the top of the ring, and
            // taking fmod to twoPi keeps phi in [0, 2pi).
            float theta = std::atan2(static_cast<float>(dy), static_cast<float>(dx));
            float phi = theta + halfPi;
            if (phi < 0.0f)
            {
                phi += twoPi;
            }
            if (phi >= twoPi)
            {
                phi -= twoPi;
            }

            ivec2 pos(x, y);
            if (phi < sweepLimit)
            {
                screen.drawPixel(pos, ringColor);
            }
            else
            {
                screen.drawPixel(pos, trackColor);
            }
        }
    }
}
