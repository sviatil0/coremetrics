#include "image.hpp"
#include <iostream>

Image::Image(std::string filePath, ivec2 pos) : m_filePath(filePath), m_position(pos) 
{
}
std::string Image::getFilePath() const 
{
    return m_filePath;
}

void Image::draw(Screen& screen) 
{
    SDL_Surface* tempSurface = SDL_LoadBMP(m_filePath.c_str()); // using SDL to read the bytes
    if (!tempSurface)
    {
        return;
    }

    SDL_Surface* formattedSurface = SDL_ConvertSurface(tempSurface, SDL_PIXELFORMAT_RGBA8888);
    SDL_DestroySurface(tempSurface);

    if (!formattedSurface)
    {
        return;
    }

    Uint32* pixels = static_cast<Uint32*>(formattedSurface->pixels);

    for (int y = 0; y < formattedSurface->h; ++y) 
    {
        for (int x = 0; x < formattedSurface->w; ++x) 
        {
            Uint32 pixelColor = pixels[y * formattedSurface->w + x];

            Uint8 r, g, b, a;
            const SDL_PixelFormatDetails* formatDetails = SDL_GetPixelFormatDetails(formattedSurface->format);
            SDL_GetRGBA(pixelColor, formatDetails, nullptr, &r, &g, &b, &a);

            if (a > 0)
            {
            vec3 color = { r / 255.0f, g / 255.0f, b / 255.0f };
            ivec2 screenPos = { m_position.x + x, m_position.y + y };
            screen.drawPixel(screenPos, color);
            }
        }
    }
    SDL_DestroySurface(formattedSurface);
}