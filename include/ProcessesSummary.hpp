#ifndef __PROCESSESSUMMARY_HPP__
#define __PROCESSESSUMMARY_HPP__

#include <cstddef>

#include "screen.hpp"

// Paints the Processes-tab aggregate strip directly below the visible
// data rows and above the footer chrome. The strip shows the total
// process count, summed CPU%, summed MEM%, and a scroll-position
// indicator when the table is longer than the visible window. Inputs
// are passed explicitly so the helper has no static state of its own;
// the caller threads the cached g_lastProcCount / g_sumCpuPct /
// g_sumMemPct / g_processScrollOffset / g_processVisibleCount that
// pollMetrics() refreshes each tick.
//
// Phase 1.2 slice 9 of the GUI evolution spec. Extracted from
// coremetrics.cpp; also swaps the hardcoded vec3(0.65f, 0.65f, 0.65f)
// gray for Theme::textDim() and routes the count/scroll-indicator
// accent through Theme::accentGreen() so the strip reads as part of
// the Tokyo Night palette adopted in Pillar A1 (PR #207).
namespace ProcessesSummary
{
    void render(Screen &dest,
                std::size_t procCount,
                float sumCpuPct,
                float sumMemPct,
                std::size_t scrollOffset,
                std::size_t visibleCount,
                std::size_t windowSize);
}

#endif
