#include "label.hpp"
#include "screen.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>

constexpr const char *DEFAULT_FONT_PATH = "assets/font.ttf";
constexpr float DEFAULT_FONT_SIZE = 14.0f;

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

Label::Label(std::string text, ivec2 pos, vec3 col) : m_text(std::move(text)), m_position(pos), m_color(col)
{
}

std::string Label::getText() const
{
    return m_text;
}

void Label::setText(std::string text)
{
    m_text = std::move(text);
}

void Label::draw(Screen &screen)
{
    if (m_text.empty())
    {
        return;
    }
    TTF_Font *font = ensureFont();
    if (font == nullptr)
    {
        return;
    }

    SDL_Color sdlColor;
    sdlColor.r = static_cast<Uint8>(m_color.x * 255.0f);
    sdlColor.g = static_cast<Uint8>(m_color.y * 255.0f);
    sdlColor.b = static_cast<Uint8>(m_color.z * 255.0f);
    sdlColor.a = 255;

    SDL_Surface *rendered = TTF_RenderText_Blended(font, m_text.c_str(), m_text.size(), sdlColor);
    if (rendered == nullptr)
    {
        return;
    }

    screen.blitSurface(rendered, m_position);
    SDL_DestroySurface(rendered);
}
