#include "screen.hpp"

Screen::Screen(unsigned int w, unsigned int h) : width(w), height(h), surface(nullptr)
{
    if (width == 0 || height == 0)
    {
        std::cerr << "Screen dimensions must be greater than 0" << '\n';
        return;
    }
    surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA8888);
    if (!surface)
    {
        std::cerr << "Failed to create surface: " << SDL_GetError() << '\n';
        return;
    }
    SDL_FillSurfaceRect(surface, nullptr, 0);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
}

Screen::~Screen()
{
    if (surface)
    {
        SDL_DestroySurface(surface);
        surface = nullptr;
    }
}

void Screen::blitTo(SDL_Surface *target)
{
    if (!surface || !target)
    {
        return;
    }
    SDL_BlitSurface(surface, nullptr, target, nullptr);
}

void Screen::plotLineLow(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<float> &color)
{
    int dx = b.x - a.x;
    int dy = b.y - a.y;
    int yi = 1;
    if (dy < 0)
    {
        yi = -yi;
        dy = -dy;
    }
    int D = (2 * dy) - dx;
    int y = a.y;
    for (int x = a.x; x <= b.x; x++)
    {
        drawPixel(ivec2(x, y), color);
        if (D > 0)
        {
            y += yi;
            D += (2 * (dy - dx));
        }
        else
        {
            D += (2 * dy);
        }
    }
}

void Screen::plotLineHigh(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<float> &color)
{
    int dx = b.x - a.x;
    int dy = b.y - a.y;
    int xi = 1;
    if (dx < 0)
    {
        xi = -xi;
        dx = -dx;
    }
    int D = (2 * dx) - dy;
    int x = a.x;
    for (int y = a.y; y <= b.y; y++)
    {
        drawPixel(ivec2(x, y), color);
        if (D > 0)
        {
            x += xi;
            D += (2 * (dx - dy));
        }
        else
        {
            D += (2 * dx);
        }
    }
}