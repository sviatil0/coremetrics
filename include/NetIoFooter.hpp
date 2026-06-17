#ifndef __NETIOFOOTER_HPP__
#define __NETIOFOOTER_HPP__

#include "SystemMetrics.hpp"
#include "screen.hpp"

// Paints the bottom-right "DISK r/w  NET rx/tx" footer string in
// compact units (K/M/G). Worst-case ~34 glyphs keeps the right edge
// to the left of the EXIT button. Inputs are passed explicitly so the
// helper has no static state of its own.
//
// Phase 1.2 slice 4 of the GUI evolution spec (PR #163).
namespace NetIoFooter
{
    void render(Screen &dest,
                unsigned long long diskReadKbPerSec,
                unsigned long long diskWriteKbPerSec,
                const NetIo &netIo);
}

#endif
