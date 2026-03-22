#ifndef __BOX_HPP__
#define __BOX_HPP__

#include "GUIElement.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

class Box : public GUIElement
{
public:
    vec2 minPos;
    vec2 maxPos;
    vec3 color;

    Box() : minPos(), maxPos(), color() {}
    Box(vec2 minPos, vec2 maxPos, vec3 color) : minPos(minPos), maxPos(maxPos), color(color) {}

    void draw(Screen &screen) override;
};

#endif
