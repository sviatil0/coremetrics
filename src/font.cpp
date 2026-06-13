#include "font.hpp"
#include "AssetPath.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>

constexpr const char *DEFAULT_FONT_PATH = "assets/font.ttf";
// Roboto Regular at 18pt was perceptibly dim against the near-black UI
// background once anti-aliasing thinned each stroke. Going up to 20pt and
// switching the runtime style to BOLD keeps glyphs inside the 20px row
// height while giving each character enough opaque pixels to actually
// read at screenshot resolution.
constexpr float DEFAULT_FONT_SIZE = 20.0f;

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
    std::string resolved = AssetPath::resolve(DEFAULT_FONT_PATH);
    g_font = TTF_OpenFont(resolved.c_str(), DEFAULT_FONT_SIZE);
    if (g_font == nullptr)
    {
        std::cerr << "TTF_OpenFont failed for " << resolved << ": " << SDL_GetError() << '\n';
        return nullptr;
    }
    // Hinting LIGHT keeps Roboto's stems straight on a dark background without
    // forcing sub-pixel positioning that the destination surface (BLENDMODE_NONE)
    // can't blend nicely. NONE would be even crisper but also a touch chunkier.
    TTF_SetFontHinting(g_font, TTF_HINTING_LIGHT);
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
