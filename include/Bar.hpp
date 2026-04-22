#ifndef __BAR_HPP__
#define __BAR_HPP__

#include <string>
#include "GUIElement.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

class Bar : public GUIElement
{
private:
    ivec2 minPos;
    ivec2 maxPos;
    vec3 fillColor;
    vec3 bgColor;
    float value;
    float minVal;
    float maxVal;
    std::string metricName;

public:
    Bar(ivec2 minPos, ivec2 maxPos, vec3 fillColor, vec3 bgColor,
        float minVal = 0.0f, float maxVal = 100.0f, std::string metricName = "");

    void setValue(float value);
    float getValue() const;
    const std::string& getMetricName() const;
    ivec2 getMinPos() const;
    ivec2 getMaxPos() const;

    void draw(Screen &screen) override;
};

#endif
