#include "font.hpp"
#include "AssetPath.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <unordered_map>
#include <string>

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

// Cache TTF render output keyed on (text, color). Same-string + same-color
// requests are the common case for static chrome (CPU history, RAM history,
// poll Xms, etc); each TTF_RenderText_Blended call is 50-200 microseconds,
// so caching them turns the rendering of every static label into a hash
// lookup plus a surface blit. Cleared by Font::shutdown().
static std::unordered_map<std::string, SDL_Surface *> g_textSurfaceCache;

// Resolve a cached TTF_RenderText_Blended surface for (text, color), rendering
// and caching on miss. Returns nullptr if the font isn't ready or the render
// fails. Shared by drawText and drawTextBold so the bold path reuses the
// exact same glyph surface and just blits it twice.
static SDL_Surface *getOrRenderSurface(const std::string &text, vec3 color)
{
    TTF_Font *font = ensureFont();
    if (font == nullptr)
    {
        return nullptr;
    }

    Uint8 r = static_cast<Uint8>(color.x * 255.0f);
    Uint8 g = static_cast<Uint8>(color.y * 255.0f);
    Uint8 b = static_cast<Uint8>(color.z * 255.0f);
    // Cache key folds the text and 24-bit RGB into a single string so the
    // hash lookup is cheap; alpha is always 255 for our usage.
    std::string key;
    key.reserve(text.size() + 4);
    key.append(text);
    key.push_back(static_cast<char>(r));
    key.push_back(static_cast<char>(g));
    key.push_back(static_cast<char>(b));

    auto it = g_textSurfaceCache.find(key);
    if (it != g_textSurfaceCache.end())
    {
        return it->second;
    }

    SDL_Color sdlColor;
    sdlColor.r = r;
    sdlColor.g = g;
    sdlColor.b = b;
    sdlColor.a = 255;
    SDL_Surface *rendered = TTF_RenderText_Blended(font, text.c_str(), text.size(), sdlColor);
    if (rendered == nullptr)
    {
        return nullptr;
    }
    g_textSurfaceCache.emplace(std::move(key), rendered);
    return rendered;
}

void Font::drawText(Screen &screen, const std::string &text, ivec2 pos, vec3 color)
{
    if (text.empty())
    {
        return;
    }
    SDL_Surface *rendered = getOrRenderSurface(text, color);
    if (rendered == nullptr)
    {
        return;
    }
    screen.blitSurface(rendered, pos);
}

void Font::drawTextBold(Screen &screen, const std::string &text, ivec2 pos, vec3 color)
{
    if (text.empty())
    {
        return;
    }
    // Fake bold by blitting the cached glyph surface twice, once at the
    // requested position and once shifted by 1px on x. The extra column of
    // opaque pixels thickens every vertical stem without needing a bold TTF.
    // We avoid TTF_SetFontStyle(TTF_STYLE_BOLD) here because flipping the
    // style mutates the shared TTF_Font handle and would also poison every
    // cached regular-weight surface drawn after the call.
    SDL_Surface *rendered = getOrRenderSurface(text, color);
    if (rendered == nullptr)
    {
        return;
    }
    screen.blitSurface(rendered, pos);
    screen.blitSurface(rendered, ivec2(pos.x + 1, pos.y));
}

void Font::shutdown()
{
    for (auto &kv : g_textSurfaceCache)
    {
        if (kv.second != nullptr)
        {
            SDL_DestroySurface(kv.second);
        }
    }
    g_textSurfaceCache.clear();
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
