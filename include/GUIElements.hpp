#ifndef __GUIELEMENTS_HPP__
#define __GUIELEMENTS_HPP__
#include "vec2.hpp"
#include "vec3.hpp"
#include "screen.hpp"

enum class GUIElementType
{
    POINT,
    LINE,
    BOX
};
class GUIElement
{
public:
    virtual void draw() = 0;
    virtual ~GUIElement() = default;
};

class Point : public GUIElement
{
private:
    vec2 pos;
    vec3 color;
    Screen &screen;

public:
    Point(vec2 pos, vec3 color, Screen &screen) : pos(pos), color(color), screen(screen)
    {
    }

    void draw() override;
};

class Line : public GUIElement
{
private:
    vec2 start;
    vec2 end;
    vec3 color;
    Screen &screen;

public:
    Line(vec2 start, vec2 end, vec3 color, Screen &screen) : start(start), end(end), color(color), screen(screen)
    {
    }
    void draw() override;
};

class Box : public GUIElement
{
private:
    vec2 minPos;
    vec2 maxPos;
    vec3 color;
    Screen &screen;

public:
    Box(vec2 minPos, vec2 maxPos, vec3 color, Screen &screen) : minPos(minPos), maxPos(maxPos), color(color), screen(screen)
    {
    }
    void draw() override;
};

#endif