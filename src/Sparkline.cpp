#include "Sparkline.hpp"
#include "Thresholds.hpp"
#include "font.hpp"
#include <algorithm>

Sparkline::Sparkline(ivec2 minPos, ivec2 maxPos, vec3 color,
                     float minValue, float maxValue, std::size_t capacity)
    : minPos(minPos),
      maxPos(maxPos),
      color(color),
      minValue(minValue),
      maxValue(maxValue),
      samples(capacity),
      thresholdMode(false),
      autoScale(false)
{
}

void Sparkline::push(float value)
{
    if (value < minValue)
    {
        value = minValue;
    }
    // In auto-scale mode, an over-ceiling sample widens the range instead
    // of being clamped, with 10% headroom so the chart does not redraw the
    // new peak pinned to the top edge on its very first frame. Existing
    // samples in the ring buffer are intentionally not rescaled: they keep
    // the value they were stored at and just sit lower on the new range.
    if (value > maxValue)
    {
        if (autoScale)
        {
            maxValue = value * 1.1f;
        }
        else
        {
            value = maxValue;
        }
    }
    // First sample fills the ring buffer with itself so the rendered
    // polyline spans the full width from frame 1 instead of starting
    // mid-strip and looking truncated.
    if (samples.getSize() == 0)
    {
        std::size_t cap = samples.getCapacity();
        for (std::size_t i = 0; i < cap; ++i)
        {
            samples.push(value);
        }
        return;
    }
    samples.push(value);
}

void Sparkline::clear()
{
    samples.clear();
}

std::size_t Sparkline::getCapacity() const
{
    return samples.getCapacity();
}

std::size_t Sparkline::getSize() const
{
    return samples.getSize();
}

ivec2 Sparkline::getMinPos() const
{
    return minPos;
}

ivec2 Sparkline::getMaxPos() const
{
    return maxPos;
}

void Sparkline::setThresholdMode(bool enabled)
{
    thresholdMode = enabled;
}

bool Sparkline::getThresholdMode() const
{
    return thresholdMode;
}

void Sparkline::setAutoScale(bool enabled)
{
    autoScale = enabled;
}

bool Sparkline::getAutoScale() const
{
    return autoScale;
}

void Sparkline::draw(Screen &screen) const
{
    std::size_t n = samples.getSize();
    if (n < 2)
    {
        return;
    }
    float range = maxValue - minValue;
    if (range <= 0.0f)
    {
        return;
    }

    int width = maxPos.x - minPos.x;
    int height = maxPos.y - minPos.y;
    if (width <= 0 || height <= 0)
    {
        return;
    }

    // Pin the polyline to the right edge: the newest sample sits at maxPos.x,
    // older samples step back by stepX per index. This is the convention
    // every live-monitor sparkline uses (Activity Monitor, htop), so the
    // chart reads as "time flows left to right" without a legend.
    float stepX = static_cast<float>(width) / static_cast<float>(samples.getCapacity() - 1);

    auto plotY = [this, height](float value)
    {
        float ratio = (value - minValue) / (maxValue - minValue);
        if (ratio < 0.0f)
        {
            ratio = 0.0f;
        }
        if (ratio > 1.0f)
        {
            ratio = 1.0f;
        }
        return maxPos.y - static_cast<int>(ratio * static_cast<float>(height));
    };

    auto plotX = [this, stepX](std::size_t indexFromOldest, std::size_t total)
    {
        std::size_t indexFromNewest = (total - 1) - indexFromOldest;
        return maxPos.x - static_cast<int>(static_cast<float>(indexFromNewest) * stepX);
    };

    // Darker fill at 10% of the line color so the polyline pops clearly
    // against it instead of disappearing into a similarly-bright tint.
    // In threshold mode the fill is recomputed per segment from the
    // segment's stroke color so tint and stroke stay in sync as the line
    // passes 60% and 80%.
    vec3 fillColor(color.x * 0.10f, color.y * 0.10f, color.z * 0.10f);

    // 50% horizontal midline gives the polyline a reference plane so a
    // reviewer can tell a peak at 100% apart from one at 30% without
    // squinting at the rect bounds. Brighter than before (0.28 vs 0.18)
    // so the midline is actually visible against the fill region.
    int midY = (minPos.y + maxPos.y) / 2;
    vec3 midlineColor(0.28f, 0.28f, 0.28f);
    ivec2 midA(minPos.x, midY);
    ivec2 midB(maxPos.x, midY);
    screen.drawLine(midA, midB, midlineColor);

    // Two-pass render: first the fill region (triangles down to the
    // baseline), then the stroke. Doing fill first means the stroke
    // overdraws the fill's top edge so the line stays crisp.
    for (std::size_t i = 0; i + 1 < n; ++i)
    {
        ivec2 a(plotX(i, n),     plotY(samples.at(i)));
        ivec2 b(plotX(i + 1, n), plotY(samples.at(i + 1)));
        ivec2 aBase(a.x, maxPos.y);
        ivec2 bBase(b.x, maxPos.y);
        vec3 segFill = fillColor;
        if (thresholdMode)
        {
            float peak = samples.at(i);
            if (samples.at(i + 1) > peak)
            {
                peak = samples.at(i + 1);
            }
            float peakPct = peak / maxValue;
            vec3 segStroke = Thresholds::colorForRatio(peakPct);
            segFill = vec3(segStroke.x * 0.18f, segStroke.y * 0.18f, segStroke.z * 0.18f);
        }
        screen.drawTriangle(a, b, bBase, segFill);
        screen.drawTriangle(a, bBase, aBase, segFill);
    }

    // Three parallel polylines (y-1, y, y+1) fake a 3px stroke without
    // pulling in a real wide-line rasterizer. The middle line is the
    // exact data; the offsets are decorative thickness. In threshold mode
    // each segment picks its color from Thresholds::colorForRatio using
    // the segment's peak value, so the polyline shifts green -> yellow
    // (>60%) -> red (>80%) the same way Bar fills and the per-core strip
    // shift, giving the user one consistent pressure palette.
    for (std::size_t i = 0; i + 1 < n; ++i)
    {
        ivec2 a(plotX(i, n),     plotY(samples.at(i)));
        ivec2 b(plotX(i + 1, n), plotY(samples.at(i + 1)));
        vec3 segColor = color;
        if (thresholdMode)
        {
            float peak = samples.at(i);
            if (samples.at(i + 1) > peak)
            {
                peak = samples.at(i + 1);
            }
            float peakPct = peak / maxValue;
            segColor = Thresholds::colorForRatio(peakPct);
        }
        // 5px stroke (y-2..y+2) so the polyline is unmistakably visible
        // against the dark fill region. The middle line is the exact
        // data; the four offsets are decorative thickness.
        screen.drawLine(a, b, segColor);
        ivec2 aUp1(a.x, a.y - 1);
        ivec2 bUp1(b.x, b.y - 1);
        ivec2 aUp2(a.x, a.y - 2);
        ivec2 bUp2(b.x, b.y - 2);
        ivec2 aDn1(a.x, a.y + 1);
        ivec2 bDn1(b.x, b.y + 1);
        ivec2 aDn2(a.x, a.y + 2);
        ivec2 bDn2(b.x, b.y + 2);
        screen.drawLine(aUp1, bUp1, segColor);
        screen.drawLine(aUp2, bUp2, segColor);
        screen.drawLine(aDn1, bDn1, segColor);
        screen.drawLine(aDn2, bDn2, segColor);
    }

    // Axis tick labels at the right edge anchor the rect to absolute
    // values (0..100). Brighter color (0.55 vs 0.30) so the labels are
    // clearly readable rather than fading into the panel background.
    // "100" sits at chart top, "0" at chart bottom, both outside the
    // polyline fill area.
    vec3 labelColor(0.55f, 0.55f, 0.55f);
    ivec2 hiPos(maxPos.x + 4, minPos.y - 4);
    ivec2 loPos(maxPos.x + 4, maxPos.y - 16);
    Font::drawText(screen, "100", hiPos, labelColor);
    Font::drawText(screen, "0", loPos, labelColor);
}
