#include "image.hpp"
#include <SDL3_image/SDL_image.h>
#include <iostream>

Image::Image(std::string filePath, ivec2 pos) : m_filePath(std::move(filePath)), m_position(pos)
{
}

std::string Image::getFilePath() const
{
    return m_filePath;
}

void Image::draw(Screen &screen)
{
    SDL_Surface *loaded = IMG_Load(m_filePath.c_str());
    if (loaded == nullptr)
    {
        std::cerr << "IMG_Load failed for " << m_filePath << ": " << SDL_GetError() << '\n';
        return;
    }

    SDL_Surface *rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (rgba == nullptr)
    {
        return;
    }

    Uint8 *base = static_cast<Uint8 *>(rgba->pixels);
    int pitch = rgba->pitch;

    for (int y = 0; y < rgba->h; ++y)
    {
        Uint8 *row = base + y * pitch;
        for (int x = 0; x < rgba->w; ++x)
        {
            Uint8 r = row[x * 4 + 0];
            Uint8 g = row[x * 4 + 1];
            Uint8 b = row[x * 4 + 2];
            Uint8 a = row[x * 4 + 3];
            if (a < 128)
            {
                continue;
            }
            vec3 color = {r / 255.0f, g / 255.0f, b / 255.0f};
            ivec2 screenPos = {m_position.x + x, m_position.y + y};
            screen.drawPixel(screenPos, color);
        }
    }
    SDL_DestroySurface(rgba);
}
