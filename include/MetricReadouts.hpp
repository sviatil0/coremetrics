#ifndef __METRICREADOUTS_HPP__
#define __METRICREADOUTS_HPP__

#include "screen.hpp"

// Paints the dominant CPU / RAM / GPU percent readouts on each System
// tab card at Font::Size::Title (28 pt). The value glyphs are
// Theme::textPrimary and the trailing "%" sign is Theme::textDim so the
// number leads visually. Caller passes the three percentages by value.
//
// First visible use of the new Font::Size::Title typography size from
// PR #217 (Pillar A2 wire-up).
namespace MetricReadouts
{
    void render(Screen &dest, float cpuPct, float ramPct, float gpuPct);
}

#endif
