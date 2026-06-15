#ifndef __FONT_HPP__
#define __FONT_HPP__

#include <string>
#include "screen.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

namespace Font
{
    void drawText(Screen &screen, const std::string &text, ivec2 pos, vec3 color);
    // Same as drawText but blits the glyph surface twice with a 1px x-offset
    // to fake a bold weight without bundling a second TTF. Used for the high
    // value CPU/RAM/GPU percent readouts so the numbers visually outweigh
    // their static "CPU"/"RAM"/"GPU" labels.
    void drawTextBold(Screen &screen, const std::string &text, ivec2 pos, vec3 color);
    void shutdown();
}

#endif
