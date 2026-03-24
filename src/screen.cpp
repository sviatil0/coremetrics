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
    if (!SDL_FillSurfaceRect(surface, nullptr, 0))
    {
        std::cerr << "Failed to color the surface: " << SDL_GetError() << '\n';
    }

    if (!SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE))
    {
        std::cerr << "Failed to set the blending mode for the surface: " << SDL_GetError() << '\n';
    }
}

Screen::~Screen()
{
    if (surface)
    {
        SDL_DestroySurface(surface);
        surface = nullptr;
    }
}

void Screen::clear()
{
    if (!surface)
    {
        return;
    }
    SDL_FillSurfaceRect(surface, nullptr, 0);
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

void Screen::drawPixel(const Tvec2<int> &pos, const Tvec3<float> &color)
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

void Screen::drawLine(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<float> &color)
{
    if (std::abs(b.y - a.y) < std::abs(b.x - a.x))
    {
        if (a.x > b.x)
        {
            plotLineLow(b, a, color);
        }
        else
        {
            plotLineLow(a, b, color);
        }
    }
    else
    {
        if (a.y > b.y)
        {
            plotLineHigh(b, a, color);
        }
        else
        {
            plotLineHigh(a, b, color);
        }
    }
}

void Screen::drawLine(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<int> &color)
{
    vec3 floatColor(
        static_cast<float>(color.x) / static_cast<float>(MAX_CHANNEL_VALUE),
        static_cast<float>(color.y) / static_cast<float>(MAX_CHANNEL_VALUE),
        static_cast<float>(color.z) / static_cast<float>(MAX_CHANNEL_VALUE)
    );
    drawLine(a, b, floatColor);
}

void Screen::drawBox(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<float> &color)
{
    int xMin = std::min(a.x, b.x);
    int xMax = std::max(a.x, b.x);
    int yMin = std::min(a.y, b.y);
    int yMax = std::max(a.y, b.y);
    for (int row = yMin; row <= yMax; row++)
    {
        for (int col = xMin; col <= xMax; col++)
        {
            drawPixel(ivec2(col, row), color);
        }
    }
}
    
int Screen::crossEdge(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec2<int> &c)
{
    return (((b.y-a.y)*(c.x-a.x)) - ((b.x-a.x)*(c.y-a.y)));
}

void Screen::drawTriangle(const Tvec2<int> &v1, const Tvec2<int> &v2, const Tvec2<int> &v3, const Tvec3<float> &color)
{
    //drawing help from: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage.html
    // get bounding coords to loop over (box)
    int minX = std::min(v1.x, std::min(v2.x, v3.x));
    int minY = std::min(v1.y, std::min(v2.y, v3.y));
    int maxX = std::max(v1.x, std::max(v2.x, v3.x));
    int maxY = std::max(v1.y, std::max(v2.y, v3.y));

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
