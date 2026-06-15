#ifndef __SPARKLINE_HPP__
#define __SPARKLINE_HPP__

#include "RingBuffer.hpp"
#include "screen.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Small inline time-series chart, drawn into a rectangle as a polyline of
// the rolling-window samples. Uses Screen::drawLine (the Bresenham path)
// so the entire pipeline is the project's own code.
//
// Values outside [minValue, maxValue] are clamped, not stretched, so the
// chart stays a stable comparison surface across ticks.
//
// When thresholdMode is on, each polyline segment is recolored from the
// shared Thresholds palette (green to yellow at 60% to red at 80%) based
// on the segment's peak value as a percent of maxValue, matching the
// palette used by Bar and the per-core strip. Default off so existing
// single-color sparklines (network rx green, tx orange) keep their look.
class Sparkline
{
private:
    ivec2 minPos;
    ivec2 maxPos;
    vec3 color;
    float minValue;
    float maxValue;
    RingBuffer<float> samples;
    bool thresholdMode;

public:
    Sparkline(ivec2 minPos, ivec2 maxPos, vec3 color,
              float minValue, float maxValue, std::size_t capacity);

    void push(float value);
    void clear();

    std::size_t getCapacity() const;
    std::size_t getSize() const;
    ivec2 getMinPos() const;
    ivec2 getMaxPos() const;

    void setThresholdMode(bool enabled);
    bool getThresholdMode() const;

    void draw(Screen &screen) const;
};

#endif
