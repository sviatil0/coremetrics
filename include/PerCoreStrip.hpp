#ifndef __PERCORESTRIP_HPP__
#define __PERCORESTRIP_HPP__

#include <vector>

#include "screen.hpp"

// Paints the horizontal per-core CPU strip directly under the CPU/RAM
// /GPU bars on the System tab. Each core gets a small slot; the slot
// fills left-to-right with the load color picked by Thresholds. Inputs
// are passed explicitly so the helper has no static state.
//
// Phase 1.2 slice 7 of the GUI evolution spec (PR #163).
namespace PerCoreStrip
{
    void render(Screen &dest,
                const std::vector<float> &perCoreCpu,
                bool sparklinesEnabled);
}

#endif
