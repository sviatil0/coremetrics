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
// read at screenshot resolution. Body remains 20pt; Caption / Title are
// derived sizes for the new typography hierarchy (Pillar A2).
constexpr float CAPTION_FONT_SIZE = 14.0f;
constexpr float BODY_FONT_SIZE = 20.0f;
constexpr float TITLE_FONT_SIZE = 28.0f;

// Each Size gets its own TTF_Font handle so a per-size point size is
// baked into the face. Sharing a handle and using TTF_SetFontSize would
// mutate state every call and poison the per-size glyph caches.
static TTF_Font *g_fontCaption = nullptr;
static TTF_Font *g_fontBody = nullptr;
static TTF_Font *g_fontTitle = nullptr;

// TTF_Init is process-global, so we track it once for all three faces
// rather than per-size. Asset resolution is also a one-time concern.
static bool g_ttfInitTried = false;
static bool g_ttfInitOk = false;

static float fontSizeFor(Font::Size size)
{
    switch (size)
    {
        case Font::Size::Caption:
            return CAPTION_FONT_SIZE;
        case Font::Size::Title:
            return TITLE_FONT_SIZE;
        case Font::Size::Body:
        default:
            return BODY_FONT_SIZE;
    }
}

static TTF_Font *&fontHandleFor(Font::Size size)
{
    switch (size)
    {
        case Font::Size::Caption:
            return g_fontCaption;
        case Font::Size::Title:
            return g_fontTitle;
        case Font::Size::Body:
        default:
            return g_fontBody;
    }
}

static TTF_Font *ensureFont(Font::Size size)
{
    if (!g_ttfInitTried)
    {
        g_ttfInitTried = true;
        if (!TTF_Init())
        {
            std::cerr << "TTF_Init failed: " << SDL_GetError() << '\n';
            g_ttfInitOk = false;
            return nullptr;
        }
        g_ttfInitOk = true;
    }
    if (!g_ttfInitOk)
    {
        return nullptr;
    }

    TTF_Font *&slot = fontHandleFor(size);
    if (slot != nullptr)
    {
        return slot;
    }

    std::string resolved = AssetPath::resolve(DEFAULT_FONT_PATH);
    slot = TTF_OpenFont(resolved.c_str(), fontSizeFor(size));
    if (slot == nullptr)
    {
        std::cerr << "TTF_OpenFont failed for " << resolved << ": " << SDL_GetError() << '\n';
        return nullptr;
    }
    // Hinting LIGHT keeps Roboto's stems straight on a dark background without
    // forcing sub-pixel positioning that the destination surface (BLENDMODE_NONE)
    // can't blend nicely. NONE would be even crisper but also a touch chunkier.
    // Same hint for every size so Caption / Body / Title share weight character.
    TTF_SetFontHinting(slot, TTF_HINTING_LIGHT);
    return slot;
}

// Cache TTF render output keyed on (text, color), one cache per Size so the
// existing Body cache is never invalidated when Caption or Title rasterizes
// the same string. Same-string + same-color requests are the common case for
// static chrome (CPU history, RAM history, poll Xms, etc); each
// TTF_RenderText_Blended call is 50-200 microseconds, so caching them turns
// every static label into a hash lookup plus a surface blit. Cleared by
// Font::shutdown().
static std::unordered_map<std::string, SDL_Surface *> g_textSurfaceCacheCaption;
static std::unordered_map<std::string, SDL_Surface *> g_textSurfaceCacheBody;
static std::unordered_map<std::string, SDL_Surface *> g_textSurfaceCacheTitle;

static std::unordered_map<std::string, SDL_Surface *> &cacheFor(Font::Size size)
{
    switch (size)
    {
        case Font::Size::Caption:
            return g_textSurfaceCacheCaption;
        case Font::Size::Title:
            return g_textSurfaceCacheTitle;
        case Font::Size::Body:
        default:
            return g_textSurfaceCacheBody;
    }
}

// Resolve a cached TTF_RenderText_Blended surface for (text, color, size),
// rendering and caching on miss. Returns nullptr if the font isn't ready or
// the render fails. Shared by drawText and drawTextBold so the bold path
// reuses the exact same glyph surface and just blits it twice. Single
// surface-cache resolver across both API entry points and all three sizes.
static SDL_Surface *getOrRenderSurface(const std::string &text, vec3 color, Font::Size size)
{
    TTF_Font *font = ensureFont(size);
    if (font == nullptr)
    {
        return nullptr;
    }

    Uint8 r = static_cast<Uint8>(color.x * 255.0f);
    Uint8 g = static_cast<Uint8>(color.y * 255.0f);
    Uint8 b = static_cast<Uint8>(color.z * 255.0f);
    // Cache key folds the text and 24-bit RGB into a single string so the
    // hash lookup is cheap; alpha is always 255 for our usage. Size is not
    // part of the key because each Size has its own dedicated cache map.
    std::string key;
    key.reserve(text.size() + 4);
    key.append(text);
    key.push_back(static_cast<char>(r));
    key.push_back(static_cast<char>(g));
    key.push_back(static_cast<char>(b));

    std::unordered_map<std::string, SDL_Surface *> &cache = cacheFor(size);
    auto it = cache.find(key);
    if (it != cache.end())
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
    cache.emplace(std::move(key), rendered);
    return rendered;
}

void Font::drawText(Screen &screen, const std::string &text, ivec2 pos, vec3 color, Size size)
{
    if (text.empty())
    {
        return;
    }
    SDL_Surface *rendered = getOrRenderSurface(text, color, size);
    if (rendered == nullptr)
    {
        return;
    }
    screen.blitSurface(rendered, pos);
}

void Font::drawText(Screen &screen, const std::string &text, ivec2 pos, vec3 color)
{
    drawText(screen, text, pos, color, Size::Body);
}

void Font::drawTextBold(Screen &screen, const std::string &text, ivec2 pos, vec3 color, Size size)
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
    SDL_Surface *rendered = getOrRenderSurface(text, color, size);
    if (rendered == nullptr)
    {
        return;
    }
    screen.blitSurface(rendered, pos);
    screen.blitSurface(rendered, ivec2(pos.x + 1, pos.y));
}

void Font::drawTextBold(Screen &screen, const std::string &text, ivec2 pos, vec3 color)
{
    drawTextBold(screen, text, pos, color, Size::Body);
}

int Font::measureTextWidth(const std::string &text, Size size)
{
    if (text.empty())
    {
        return 0;
    }
    TTF_Font *font = ensureFont(size);
    if (font == nullptr)
    {
        return 0;
    }
    // TTF_GetStringSize returns the laid-out width without rasterizing the
    // glyphs, so the call is cheap enough to do on every toggle event. The
    // height result is ignored: the y baseline is fixed by the button rect.
    int width = 0;
    int height = 0;
    if (!TTF_GetStringSize(font, text.c_str(), text.size(), &width, &height))
    {
        return 0;
    }
    return width;
}

int Font::measureTextWidth(const std::string &text)
{
    return measureTextWidth(text, Size::Body);
}

void Font::shutdown()
{
    for (auto &kv : g_textSurfaceCacheCaption)
    {
        if (kv.second != nullptr)
        {
            SDL_DestroySurface(kv.second);
        }
    }
    g_textSurfaceCacheCaption.clear();
    for (auto &kv : g_textSurfaceCacheBody)
    {
        if (kv.second != nullptr)
        {
            SDL_DestroySurface(kv.second);
        }
    }
    g_textSurfaceCacheBody.clear();
    for (auto &kv : g_textSurfaceCacheTitle)
    {
        if (kv.second != nullptr)
        {
            SDL_DestroySurface(kv.second);
        }
    }
    g_textSurfaceCacheTitle.clear();

    if (g_fontCaption != nullptr)
    {
        TTF_CloseFont(g_fontCaption);
        g_fontCaption = nullptr;
    }
    if (g_fontBody != nullptr)
    {
        TTF_CloseFont(g_fontBody);
        g_fontBody = nullptr;
    }
    if (g_fontTitle != nullptr)
    {
        TTF_CloseFont(g_fontTitle);
        g_fontTitle = nullptr;
    }
    if (g_ttfInitTried)
    {
        if (g_ttfInitOk)
        {
            TTF_Quit();
        }
        g_ttfInitTried = false;
        g_ttfInitOk = false;
    }
}
