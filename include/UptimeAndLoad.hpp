#ifndef __UPTIMEANDLOAD_HPP__
#define __UPTIMEANDLOAD_HPP__

#include <vector>

#include "screen.hpp"

// Paints the "Up Xd Yh Zm" + "Load 1.23 4.56 7.89" row at the top of
// the System tab (y=44). Inputs are passed explicitly so the helper
// stays free of coremetrics.cpp globals.
//
// Phase 1.2 slice 3 of the GUI evolution spec (PR #163).
namespace UptimeAndLoad
{
    void render(Screen &dest,
                unsigned long long uptimeSeconds,
                const std::vector<float> &loadAverages);
}

#endif
