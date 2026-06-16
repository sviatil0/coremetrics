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
//
// When autoScale is on, push() widens maxValue to fit any incoming sample
// that would otherwise be clamped, adding 10% headroom so the chart does
// not pin to its new ceiling. Older samples are not rescaled: they keep
// the value they were stored with and just sit lower on the new range.
// Default off so the fixed 0..100 percent sparklines (CPU, RAM, GPU) keep
// their stable comparison surface; the network sparkline opts in because
// its KB/s scale has no natural ceiling.
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
    bool autoScale;

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

    void setAutoScale(bool enabled);
    bool getAutoScale() const;

    void draw(Screen &screen) const;
};

#endif
