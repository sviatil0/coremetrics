#ifndef __DISKUSAGEROW_HPP__
#define __DISKUSAGEROW_HPP__

#include "SystemMetrics.hpp"
#include "screen.hpp"

// Paints the right-hand "DISK used / total GB (pct%)" label on the
// y=44 status row of the System tab. Color follows Thresholds (red
// over 90%, yellow over 75%). Inputs are passed explicitly so the
// helper has no static state.
//
// Phase 1.2 slice 5 of the GUI evolution spec (PR #163).
namespace DiskUsageRow
{
    void render(Screen &dest, const DiskUsage &disk);
}

#endif
