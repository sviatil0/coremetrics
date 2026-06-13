#include "Sparkline.hpp"
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

    for (std::size_t i = 0; i + 1 < n; ++i)
    {
        ivec2 a(plotX(i, n),     plotY(samples.at(i)));
        ivec2 b(plotX(i + 1, n), plotY(samples.at(i + 1)));
        screen.drawLine(a, b, color);
    }
}
