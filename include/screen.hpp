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

public:
    Screen(unsigned int w, unsigned int h);
    ~Screen();

    void blitTo(SDL_Surface *target);

    template <typename T>
    void drawPixel(const Tvec2<T> &pos, const Tvec3<float> &color)
    {
        if (!surface)
        {
            return;
        }
        int x = static_cast<int>(pos.x);
        int y = static_cast<int>(pos.y);
        if (x < 0 || x >= static_cast<int>(width) || y < 0 || y >= static_cast<int>(height))
        {
            return;
        }
        Uint8 r = static_cast<Uint8>(std::clamp(color.x, 0.0f, 1.0f) * static_cast<float>(MAX_CHANNEL_VALUE));
        Uint8 g = static_cast<Uint8>(std::clamp(color.y, 0.0f, 1.0f) * static_cast<float>(MAX_CHANNEL_VALUE));
        Uint8 b = static_cast<Uint8>(std::clamp(color.z, 0.0f, 1.0f) * static_cast<float>(MAX_CHANNEL_VALUE));

        Uint8* row = static_cast<Uint8 *>(surface->pixels) + y * surface->pitch;
        Uint32 *pixels = reinterpret_cast<Uint32 *>(row);
        pixels[x] = SDL_MapSurfaceRGBA(surface, r, g, b, static_cast<Uint8>(MAX_CHANNEL_VALUE));
    }

    template <typename T, typename U>
    void drawLine(const Tvec2<T> &a, const Tvec2<U> &b, const Tvec3<float> &color)
    {
        ivec2 aInt(static_cast<int>(a.x), static_cast<int>(a.y));
        ivec2 bInt(static_cast<int>(b.x), static_cast<int>(b.y));
        if (std::abs(bInt.y - aInt.y) < std::abs(bInt.x - aInt.x))
        {
            if (aInt.x > bInt.x)
            {
                plotLineLow(bInt, aInt, color);
            }
            else
            {
                plotLineLow(aInt, bInt, color);
            }
        }
        else
        {
            if (aInt.y > bInt.y)
            {
                plotLineHigh(bInt, aInt, color);
            }
            else
            {
                plotLineHigh(aInt, bInt, color);
            }
        }
    }

    template <typename T, typename U>
    void drawLine(const Tvec2<T> &a, const Tvec2<U> &b, const Tvec3<int> &color)
    {
        vec3 floatColor(
            static_cast<float>(color.x) / static_cast<float>(MAX_CHANNEL_VALUE),
            static_cast<float>(color.y) / static_cast<float>(MAX_CHANNEL_VALUE),
            static_cast<float>(color.z) / static_cast<float>(MAX_CHANNEL_VALUE)
        );
        drawLine(a, b, floatColor);
    }

    template <typename T, typename U, typename V>
    void drawBox(const Tvec2<T> &a, const Tvec2<U> &b, const Tvec3<V> &color)
    {
        int xMin = std::min(static_cast<int>(a.x), static_cast<int>(b.x));
        int xMax = std::max(static_cast<int>(a.x), static_cast<int>(b.x));
        int yMin = std::min(static_cast<int>(a.y), static_cast<int>(b.y));
        int yMax = std::max(static_cast<int>(a.y), static_cast<int>(b.y));
        for (int row = yMin; row <= yMax; row++)
        {
            for (int col = xMin; col <= xMax; col++)
            {
                drawPixel(ivec2(col, row), color);
            }
        }
    }
        
    int crossEdge(const ivec2& a, const ivec2& b, const ivec2& c)
    {
        return (((b.y-a.y)*(c.x-a.x)) - ((b.x-a.x)*(c.y-a.y)));
    }

    void drawTriangle(const ivec2& v1, const ivec2& v2, const ivec2& v3, const vec3& color)
    {
        //drawing help from: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage.html
        // get bounding coords to loop over (square)
        int minX = static_cast<int>(fmin(v1.x, fmin(v2.x, v3.x)));
        int minY = static_cast<int>(fmin(v1.y, fmin(v2.y, v3.y)));
        int maxX = static_cast<int>(fmax(v1.x, fmax(v2.x, v3.x)));
        int maxY = static_cast<int>(fmax(v1.y, fmax(v2.y, v3.y)));

        // within boundaries, check if each pixel is inside triangle or not
        for (int x = minX; x <= maxX; x++)
        {
            for (int y = minY; y <= maxY; y++)
            {
                ivec2 p = ivec2(x, y);
                int w0 = crossEdge(v2, v3, p);
                int w1 = crossEdge(v3, v1, p);
                int w2 = crossEdge(v1, v2, p);

                if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || //if clockwise order of verts
                    (w0 <= 0 && w1 <= 0 && w2 <= 0)) //if counter-clockwise order of verts
                {
                    drawPixel(p, color);
                }
            }
        }
    }
};

#endif