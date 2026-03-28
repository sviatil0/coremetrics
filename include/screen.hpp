#ifndef __SCREEN_HPP__
#define __SCREEN_HPP__

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <SDL3/SDL.h>
#include "linear.hpp"

class Screen
{
private:
    static constexpr int MAX_CHANNEL_VALUE = 255;

    unsigned int width;
    unsigned int height;
    SDL_Surface *surface;

    void plotLineLow(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<float> &color);
    void plotLineHigh(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<float> &color);
    int crossEdge(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec2<int> &c);

public:
    Screen(unsigned int w, unsigned int h);
    ~Screen();

    void clear();
    void blitTo(SDL_Surface *target);
    void drawPixel(const Tvec2<int> &pos, const Tvec3<float> &color);
    void drawLine(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<float> &color);
    void drawLine(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<int> &color);
    void drawBox(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<float> &color);
    void drawTriangle(const Tvec2<int> &v1, const Tvec2<int> &v2, const Tvec2<int> &v3, const Tvec3<float> &color);
};

#endif