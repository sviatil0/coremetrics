#ifndef __FOOTERLIVESTATS_HPP__
#define __FOOTERLIVESTATS_HPP__

#include <cstddef>

#include "screen.hpp"

// Paints the "N procs" live count at x=370, y=492 along the bottom
// chrome row. Sits to the left of the "DISK r/w  NET rx/tx" footer
// painted by NetIoFooter so the two strings read as one footer line.
// The proc count is clamped to 9999 to keep the worst-case glyph run
// short enough that it never collides with the disk/net text that
// begins at x=460. Inputs are passed explicitly so the helper has no
// static state of its own.
//
// Phase 1.2 slice 8 of the GUI evolution spec. Extracted from
// coremetrics.cpp; also swaps the hardcoded vec3(0.5f, 0.5f, 0.5f)
// gray for Theme::textDim() so the footer matches the Tokyo Night
// palette adopted in Pillar A1 (PR #207).
namespace FooterLiveStats
{
    void render(Screen &dest, std::size_t procCount);
}

#endif
