#ifndef __SCREEN_HPP__
#define __SCREEN_HPP__

#include <SDL3/SDL.h>
#include "vec2.hpp"
#include "vec3.hpp"

class Screen
{
private:
    unsigned int width;
    unsigned int height;
    SDL_Surface* window;

public:
    Screen() = default;
    Screen(unsigned int w, unsigned int h); //constructors,

    //always use floats: if user wants to use ints it still works?
    //otherwise need 3 overloads for all combo of int and float?
    void drawPixel(const vec2<float>& pos, const vec3<float>& color); 
    void blitTo(SDL_Surface* toWindow);
    void drawLine(const vec2<float>& a, const vec2<float>& b); //Bresenham line implementation
    void drawBox(const vec2<float>& min, const vec2<float>& max); //determine which is min and which is max within method
};
#endif