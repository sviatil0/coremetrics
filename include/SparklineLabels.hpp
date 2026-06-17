#ifndef __SPARKLINELABELS_HPP__
#define __SPARKLINELABELS_HPP__

#include "screen.hpp"

// Paints the four tiny "CPU history" / "RAM history" / "GPU history" /
// "NET history" accent labels at the left edge of each sparkline strip
// on the System tab. Self-contained: no globals, fixed positions.
//
// Phase 1.2 slice 2 from the GUI evolution spec (PR #163): peel render
// helpers out of coremetrics.cpp into their own TUs.
namespace SparklineLabels
{
    void render(Screen &dest);
}

#endif
