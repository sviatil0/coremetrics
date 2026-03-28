#ifndef __POINT_HPP__
#define __POINT_HPP__

#include "GUIElement.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

class Point : public GUIElement
{
public:
    vec2 pos;
    vec3 color;

    Point() : pos(), color() {}
    Point(vec2 pos, vec3 color) : pos(pos), color(color) {}

    void draw(Screen &screen) override;
};

#endif