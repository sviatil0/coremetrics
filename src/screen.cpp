#include "screen.hpp"

constexpr int RESX = 960;
constexpr int RESY = 540;

Screen::Screen() 
{
    //default width and height if no args passed to constructor
    width = RESX;
    height = RESY;
}
Screen::Screen(unsigned int w, unsigned int h) 
{
    width = w;
    height = h;
}

/*
class Screen
{
private:
    uint width;
    uint height;
    SDL_Surface* window;

public:
    Screen() = default;
    Screen(uint width, uint height); //constructor
    void drawPixel();
    void blitTo(SDL_Surface* toWindow);
    void drawLine(vec2 a, vec2 b); //Bresenham line implementation
    void drawBox(vec2 min, vec2 max); //determine which is min and which is max within method
};*/