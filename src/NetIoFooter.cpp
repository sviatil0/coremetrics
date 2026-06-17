#include "NetIoFooter.hpp"

#include <sstream>
#include <string>

#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 4 of the GUI
// evolution spec. The lambda that used to live inside renderNetIo is
// now a free anonymous-namespace helper, matching the project rule of
// no lambdas in app code outside ThreadPool::submit.

namespace NetIoFooter
{
    namespace
    {
        // Compact units (K/M/G) keep the worst-case string bounded:
        // even a 10 GB/s host renders as '10.0G' (5 chars) rather than
        // '10240.0M' (8 chars). Caps the right edge of the footer well
        // before x=810 (the EXIT button's left edge).
        std::string formatCompactRate(unsigned long long kbPerSec)
        {
            if (kbPerSec >= 1024ULL * 1024ULL)
            {
                std::ostringstream oss;
                oss.precision(1);
                oss << std::fixed
                    << (static_cast<double>(kbPerSec) / (1024.0 * 1024.0))
                    << "G";
                return oss.str();
            }
            if (kbPerSec >= 1024)
            {
                std::ostringstream oss;
                oss.precision(1);
                oss << std::fixed
                    << (static_cast<double>(kbPerSec) / 1024.0) << "M";
                return oss.str();
            }
            return std::to_string(kbPerSec) + "K";
        }
    }

    void render(Screen &dest,
                unsigned long long diskReadKbPerSec,
                unsigned long long diskWriteKbPerSec,
                const NetIo &netIo)
    {
        const bool anyDisk = (diskReadKbPerSec != 0 || diskWriteKbPerSec != 0);
        const bool anyNet = (netIo.rxKbPerSec != 0 || netIo.txKbPerSec != 0);
        if (!anyDisk && !anyNet)
        {
            return;
        }
        const vec3 dim = Theme::textDim();
        std::string text;
        if (anyDisk)
        {
            text = "DISK " + formatCompactRate(diskReadKbPerSec)
                   + "/" + formatCompactRate(diskWriteKbPerSec);
        }
        if (anyNet)
        {
            if (!text.empty())
            {
                text += "  ";
            }
            text += "NET " + formatCompactRate(netIo.rxKbPerSec)
                    + "/" + formatCompactRate(netIo.txKbPerSec);
        }
        // Worst-case string at G-scale is roughly 34 chars; at 10px per
        // glyph that lands the right edge at ~x=800, just left of the
        // EXIT button at x=820. Hard-clip the string at 36 chars so a
        // future unit-format change cannot blow through the budget.
        if (text.size() > 36)
        {
            text = text.substr(0, 36);
        }
        Font::drawText(dest, text, ivec2(460, 492), dim);
    }
}
