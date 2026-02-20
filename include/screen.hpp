#ifndef __SCREEN_HPP__
#define __SCREEN_HPP__

#include <iostream>
#include <cstring>
#include <SDL3/SDL.h>
#include "vec2.hpp"
#include "vec3.hpp"

class Screen
{
public:
    Screen() = delete; //if no args passed to constructor, cannot create object
    Screen(unsigned int w, unsigned int h) : width(w), height(h), surface(nullptr)
    {
        if (width == 0 || height == 0)
        {
            std::cerr << "Dimensions must be >0" << '\n';
        }
        surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA8888);
        if (!surface)
        {
            std::cerr << "Failed to create surface: " << SDL_GetError() << '\n';
        };
    }

    ~Screen() 
    {
        if (surface) {
            SDL_DestroySurface(surface);
            surface = nullptr;
        }
    }

    //need templated functions (below) in order to provide support for both int and float vectors
    //also means whole implementation must be within header file (like for vec2 and vec3)
    template<typename T, typename U>
    void drawPixel(const Tvec2<T>& pos, const Tvec3<U>& color)
    {
        if (pos.x < 0 || pos.x >= width || pos.y < 0 || pos.y >= height)
        {
            std::cerr << "Invalid position" << "\n";
            return;
        }

        uint8_t* pixels = static_cast<uint8_t*>(surface->pixels);
        
        pixels = pixels + (pos.y * surface->pitch) + (pos.x * SDL_BYTESPERPIXEL(surface->format));
        pixels[0] = 255;
        pixels[1] = static_cast<uint8_t>(color[2]);
        pixels[2] = static_cast<uint8_t>(color[1]);
        pixels[3] = static_cast<uint8_t>(color[0]);
    }

    template<typename T, typename U, typename V>
    void drawLine(const Tvec2<T>& a, const Tvec2<U>& b, const Tvec3<V>& color)
    {
        //convert a, b to ivec2 and color to float vec3?
        //Bresenham line implementation, using private helper methods
        if (abs(b.y - a.y) < abs(b.x - a.x))
        {
            if (a.x > b.x)
                plotLineLow(b, a, color);
            else
                plotLineLow(a, b, color);
        }
        else
        {
            if (a.y > b.y)
                plotLineHigh(b, a, color);
            else
                plotLineHigh(a, b, color);
        }
    }

    template<typename T, typename U, typename V> // should be non-templated (ivec2, ivec3 as per instructions)
    void drawBox(const Tvec2<T>& a, const Tvec2<U>& b, const Tvec3<V>& color)
    {
        //determine which vec is min and which is max within method
        T minX = std::min(a.x, static_cast<T>(b.x));
        T minY = std::min(a.y, static_cast<T>(b.y));
        T maxX = std::max(a.x,static_cast<T>(b.x));
        T maxY = std::max(a.y,static_cast<T>(b.y));

        for (T y = minY; y <= maxY; y++)
        {
            for (T x = minX; x <= maxX; x++)
            {
                drawPixel(Tvec2(x,y),color);
            }
        }

    }

    void blitTo(SDL_Surface* target)
    {
        if (target==nullptr)
        {
            std::cerr << "Invalid Target" << '\n';
            return;
        } else if (surface==nullptr)
        {
            std::cerr << "Invalid Surface" << '\n';
            return;
        }

        uint8_t* src = static_cast<uint8_t*>(surface->pixels);
        uint8_t* destination = static_cast<uint8_t*>(target->pixels);

        for (unsigned int y = 0; y < height; y++)
        {
            memcpy(destination + y*target->pitch, src + y*surface->pitch, surface->pitch);
        }
    }

private:
    unsigned int width;
    unsigned int height;
    SDL_Surface* surface = nullptr;

    //helper methods to implement Bresenham line drawing
    //source: https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
    void plotLineLow(const Tvec2<int>& a, const Tvec2<int>& b, const Tvec3<float>& color)
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
    void plotLineHigh(const Tvec2<int>& a, const Tvec2<int>& b, const Tvec3<float>& color)
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
        for (int y = a.x; y <= b.y; y++)
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
};

#endif