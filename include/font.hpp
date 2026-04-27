#ifndef __FONT_HPP__
#define __FONT_HPP__

#include <string>
#include "screen.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

namespace Font
{
    void drawText(Screen &screen, const std::string &text, ivec2 pos, vec3 color);
    void shutdown();
}

#endif
