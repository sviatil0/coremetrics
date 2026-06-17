#include "DiskUsageRow.hpp"

#include <string>

#include "ProcessUtils.hpp"
#include "Theme.hpp"
#include "Thresholds.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 5 of the GUI
// evolution spec. Pure paint - no state.

namespace DiskUsageRow
{
    void render(Screen &dest, const DiskUsage &disk)
    {
        if (disk.totalKb == 0)
        {
            return;
        }
        const unsigned long long usedKb =
            (disk.totalKb > disk.freeKb) ? disk.totalKb - disk.freeKb : 0;
        const float pct = computeDiskUsedPct(disk.totalKb, disk.freeKb);

        vec3 color = Theme::textDim();
        if (pct >= Thresholds::RED_PCT)
        {
            color = Thresholds::colorRed();
        }
        else if (pct >= Thresholds::YELLOW_PCT)
        {
            color = Thresholds::colorYellow();
        }

        const std::string text = "DISK " + formatGbString(usedKb) + " / "
                                 + formatGbString(disk.totalKb) + " GB ("
                                 + formatPct(pct) + "%)";
        // Right side of the status row at y=44, mirroring the Up + Load
        // string on the left. x=560 leaves room for a long string ("DISK
        // 1457 / 2048 GB (71.1%)") without clipping the SOUND ON button
        // at x=812.
        Font::drawText(dest, text, ivec2(560, 44), color);
    }
}
