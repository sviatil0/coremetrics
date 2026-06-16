#include "UptimeAndLoad.hpp"

#include <cstdio>
#include <string>

#include "ProcessUtils.hpp"
#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 3 of the GUI
// evolution spec. Stays pure: only state lives in the inputs.

namespace UptimeAndLoad
{
    static std::string formatLoadAverages(const std::vector<float> &loads)
    {
        if (loads.size() < 3)
        {
            return "Load --";
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Load %.2f %.2f %.2f",
                      loads[0], loads[1], loads[2]);
        return std::string(buf);
    }

    void render(Screen &dest,
                unsigned long long uptimeSeconds,
                const std::vector<float> &loadAverages)
    {
        // Dim white labels so the status row reads as a single line.
        const vec3 dimColor = Theme::textDim();
        Font::drawText(dest, formatUptimeString(uptimeSeconds),
                       ivec2(24, 44), dimColor);
        // Uptime + Load + Disk all share the y=44 baseline so the status
        // row reads as a single line instead of a staircase. The earlier
        // y=56 placement for Load predated the DISK strip and left Load
        // ~12 px below Up, which was visible as a misaligned middle field.
        Font::drawText(dest, formatLoadAverages(loadAverages),
                       ivec2(220, 44), dimColor);
    }
}
