#ifndef __FONT_HPP__
#define __FONT_HPP__

#include <string>
#include "screen.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

namespace Font
{
    // Typography size hierarchy. Each entry maps to its own TTF face handle
    // opened at a distinct point size and its own per-size glyph surface
    // cache. Body is the legacy 20 pt face the rest of the app already
    // relies on; Caption and Title are new and meant to give the renderer
    // a way to express hierarchy (subdued metadata vs section titles).
    enum class Size
    {
        Caption, // 14 pt
        Body,    // 20 pt
        Title    // 28 pt
    };

    // Size-aware text rendering. The legacy two-argument overloads default
    // to Size::Body and forward into the size-aware path, so existing call
    // sites are bit-compatible with the previous single-size implementation.
    void drawText(Screen &screen, const std::string &text, ivec2 pos, vec3 color);
    void drawText(Screen &screen, const std::string &text, ivec2 pos, vec3 color, Size size);
    // Same as drawText but blits the glyph surface twice with a 1px x-offset
    // to fake a bold weight without bundling a second TTF. Used for the high
    // value CPU/RAM/GPU percent readouts so the numbers visually outweigh
    // their static "CPU"/"RAM"/"GPU" labels.
    void drawTextBold(Screen &screen, const std::string &text, ivec2 pos, vec3 color);
    void drawTextBold(Screen &screen, const std::string &text, ivec2 pos, vec3 color, Size size);
    void shutdown();
}

#endif
