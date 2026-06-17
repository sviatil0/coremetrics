#ifndef __MEMBREAKDOWNSTRIP_HPP__
#define __MEMBREAKDOWNSTRIP_HPP__

#include "SystemMetrics.hpp"
#include "screen.hpp"

// Paints the 4-color memory composition strip (active, wired, cached,
// free) under the RAM bar on the System tab. Geometry is fixed
// (x=84..864, y=162..166). Inputs are passed explicitly so the helper
// has no static state.
//
// Phase 1.2 slice 6 of the GUI evolution spec (PR #163).
namespace MemBreakdownStrip
{
    void render(Screen &dest, const MemBreakdown &mem);
}

#endif
