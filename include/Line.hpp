#ifndef __LINE_HPP__
#define __LINE_HPP__

#include "GUIElement.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

class Line : public GUIElement
{
public:
    vec2 start;
    vec2 end;
    vec3 color;

    Line() : start(), end(), color() {}
    Line(vec2 start, vec2 end, vec3 color) : start(start), end(end), color(color) {}

    void draw(Screen &screen) override;
};

#endif
