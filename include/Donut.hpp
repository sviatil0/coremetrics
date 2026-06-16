#ifndef __DONUT_HPP__
#define __DONUT_HPP__

#include <string>
#include "Cloneable.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Donut : circular percent display with a centered text label.
// Renders a ring (between innerRadius and outerRadius) whose filled arc
// sweeps clockwise from 12 o'clock proportionally to value within
// [minVal, maxVal]. The center text defaults to the formatted percent
// when centerLabel is empty.
class Donut : public Cloneable<Donut>
{
private:
    ivec2 center;
    int outerRadius;
    int innerRadius;
    float value;
    float minVal;
    float maxVal;
    vec3 fillColor;
    vec3 trackColor;
    vec3 labelColor;
    std::string centerLabel;

    std::string formatPct(float v) const;

public:
    Donut(ivec2 center, int outerRadius, int innerRadius,
          vec3 fillColor, vec3 trackColor, vec3 labelColor,
          float minVal = 0.0f, float maxVal = 100.0f);

    void setValue(float v);
    void setLabel(std::string label);

    float getValue() const;
    const std::string& getLabel() const;
    ivec2 getCenter() const;
    int getOuterRadius() const;
    int getInnerRadius() const;

    void draw(Screen &screen) override;
};

#endif
