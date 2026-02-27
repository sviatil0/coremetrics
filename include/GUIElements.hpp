#ifndef __GUIELEMENTS_HPP__
#define __GUIELEMENTS_HPP__
#include "vec2.hpp"
#include "vec3.hpp"

struct Point {
    vec2 pos;
    vec3 color;
};

struct Line {
    vec2 start;
    vec2 end;
    vec3 color;
};

struct Box {
    vec2 minPos;
    vec2 maxPos;
    vec3 color;
};

#endif