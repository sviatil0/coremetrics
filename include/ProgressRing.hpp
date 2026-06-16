#ifndef __PROGRESS_RING_HPP__
#define __PROGRESS_RING_HPP__

#include "Cloneable.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Circular progress display. The widget rasterizes a flat donut centred on
// `center` between `innerRadius` and `outerRadius`, then splits the donut
// into a filled ring arc (the value) and an unfilled track arc (the
// remainder). The arc starts at the top (-pi/2) and sweeps clockwise so
// the visual matches every dashboard ring users have seen on watches,
// activity rings, and gauge tiles, with no extra legend needed.
class ProgressRing : public Cloneable<ProgressRing>
{
private:
    ivec2 center;
    int outerRadius;
    int innerRadius;
    float value;
    float minValue;
    float maxValue;
    vec3 ringColor;
    vec3 trackColor;

public:
    ProgressRing(ivec2 center, int outerRadius, int innerRadius,
                 vec3 ringColor, vec3 trackColor,
                 float minValue = 0.0f, float maxValue = 100.0f);

    void setValue(float v);
    void setRingColor(vec3 color);
    void setTrackColor(vec3 color);

    float getValue() const;
    ivec2 getCenter() const;
    int getOuterRadius() const;
    int getInnerRadius() const;

    void draw(Screen &screen) override;
};

#endif
