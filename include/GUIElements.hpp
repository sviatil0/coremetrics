#ifndef __GUIELEMENTS_HPP__
#define __GUIELEMENTS_HPP__

#include "GUIElement.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

enum class GUIElementType
{
    POINT,
    LINE,
    BOX
};

class Point : public GUIElement
{
public:
    vec2 pos;
    vec3 color;

    Point() : pos(), color() {}
    Point(vec2 pos, vec3 color) : pos(pos), color(color) {}

    void draw(Screen &screen) override;
};

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
