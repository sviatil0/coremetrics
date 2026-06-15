#include "Sparkline.hpp"
#include "font.hpp"
#include <algorithm>

Sparkline::Sparkline(ivec2 minPos, ivec2 maxPos, vec3 color,
                     float minValue, float maxValue, std::size_t capacity)
    : minPos(minPos),
      maxPos(maxPos),
      color(color),
      minValue(minValue),
      maxValue(maxValue),
      samples(capacity)
{
}

void Sparkline::push(float value)
{
    if (value < minValue)
    {
        value = minValue;
    }
    if (value > maxValue)
    {
        value = maxValue;
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

    // Dim fill color sits at 18% of the line color so the area under the
    // curve reads as a tinted region without overpowering the polyline or
    // any UI sitting on top of the sparkline rect.
    vec3 fillColor(color.x * 0.18f, color.y * 0.18f, color.z * 0.18f);

    // 50% horizontal midline gives the polyline a reference plane so a
    // reviewer can tell a peak at 100% apart from one at 30% without
    // squinting at the rect bounds. Drawn under the fill so the data
    // overlays it cleanly.
    int midY = (minPos.y + maxPos.y) / 2;
    vec3 midlineColor(0.18f, 0.18f, 0.18f);
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
        screen.drawTriangle(a, b, bBase, fillColor);
        screen.drawTriangle(a, bBase, aBase, fillColor);
    }

    // Three parallel polylines (y-1, y, y+1) fake a 3px stroke without
    // pulling in a real wide-line rasterizer. The middle line is the
    // exact data; the offsets are decorative thickness.
    for (std::size_t i = 0; i + 1 < n; ++i)
    {
        ivec2 a(plotX(i, n),     plotY(samples.at(i)));
        ivec2 b(plotX(i + 1, n), plotY(samples.at(i + 1)));
        screen.drawLine(a, b, color);
        ivec2 aUp(a.x, a.y - 1);
        ivec2 bUp(b.x, b.y - 1);
        ivec2 aDn(a.x, a.y + 1);
        ivec2 bDn(b.x, b.y + 1);
        screen.drawLine(aUp, bUp, color);
        screen.drawLine(aDn, bDn, color);
    }

    // Axis tick labels at the right edge anchor the rect to absolute
    // values (0..100). Typical sparkline rects are (24,*)..(864,*) inside
    // a 960px window, leaving ~96px of room past maxPos.x, so the labels
    // fit comfortably outside the data area. The "100" label sits at the
    // chart's top edge and "0" at the bottom edge, both above the chart's
    // visual midline so they read as endpoint markers without bleeding
    // into the next chart's space.
    vec3 labelColor(0.30f, 0.30f, 0.30f);
    ivec2 hiPos(maxPos.x + 4, minPos.y - 4);
    ivec2 loPos(maxPos.x + 4, maxPos.y - 16);
    Font::drawText(screen, "100", hiPos, labelColor);
    Font::drawText(screen, "0", loPos, labelColor);
}
