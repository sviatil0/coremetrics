#include "font.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>

constexpr const char *DEFAULT_FONT_PATH = "assets/font.ttf";
constexpr float DEFAULT_FONT_SIZE = 18.0f;

static TTF_Font *g_font = nullptr;
static bool g_fontInitTried = false;

static TTF_Font *ensureFont()
{
    if (g_fontInitTried)
    {
        return g_font;
    }
    g_fontInitTried = true;

    if (!TTF_Init())
    {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << '\n';
        return nullptr;
    }
    g_font = TTF_OpenFont(DEFAULT_FONT_PATH, DEFAULT_FONT_SIZE);
    if (g_font == nullptr)
    {
        std::cerr << "TTF_OpenFont failed for " << DEFAULT_FONT_PATH << ": " << SDL_GetError() << '\n';
    }
    return g_font;
}

void Font::drawText(Screen &screen, const std::string &text, ivec2 pos, vec3 color)
{
    if (text.empty())
    {
        return;
    }
    TTF_Font *font = ensureFont();
    if (font == nullptr)
    {
        return;
    }

    SDL_Color sdlColor;
    sdlColor.r = static_cast<Uint8>(color.x * 255.0f);
    sdlColor.g = static_cast<Uint8>(color.y * 255.0f);
    sdlColor.b = static_cast<Uint8>(color.z * 255.0f);
    sdlColor.a = 255;

    SDL_Surface *rendered = TTF_RenderText_Blended(font, text.c_str(), text.size(), sdlColor);
    if (rendered == nullptr)
    {
        return;
    }
    screen.blitSurface(rendered, pos);
    SDL_DestroySurface(rendered);
}

void Font::shutdown()
{
    if (g_font != nullptr)
    {
        TTF_CloseFont(g_font);
        g_font = nullptr;
    }
    if (g_fontInitTried)
    {
        TTF_Quit();
        g_fontInitTried = false;
    }
}
