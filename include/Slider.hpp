#ifndef __SLIDER_HPP__
#define __SLIDER_HPP__

#include "Cloneable.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

class Slider : public Cloneable<Slider>
{
private:
    ivec2 minPos;
    ivec2 maxPos;
    float value;
    float minVal;
    float maxVal;
    int handleWidth;
    vec3 trackColor;
    vec3 fillColor;
    vec3 handleColor;

    int handleX() const;
    bool inHandleArea(int mouseX, int mouseY) const;

public:
    Slider(ivec2 minPos, ivec2 maxPos, vec3 trackColor, vec3 fillColor, vec3 handleColor,
           float minVal = 0.0f, float maxVal = 1.0f, int handleWidth = 8);

    void setValue(float v);
    float getValue() const;
    float getMinVal() const;
    float getMaxVal() const;
    int getHandleWidth() const;
    ivec2 getMinPos() const;
    ivec2 getMaxPos() const;

    void draw(Screen& screen) override;
    bool operator()(Event* event) override;
};

#endif
