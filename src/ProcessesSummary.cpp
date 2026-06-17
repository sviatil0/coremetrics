#include "ProcessesSummary.hpp"

#include <cstddef>
#include <string>

#include "ProcessUtils.hpp"
#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 9 of the GUI
// evolution spec. The previous in-place helper used a hardcoded
// vec3(0.65f, 0.65f, 0.65f) gray and a coremetrics-static
// COLOR_ACCENT_GREEN triplet that both predated Pillar A1 (PR #207);
// the calls now route through Theme::textDim() and
// Theme::accentGreen() so the strip reads as part of the Tokyo Night
// palette like the rest of the chrome.

namespace ProcessesSummary
{
    namespace
    {
        // Strip lives at y=448..464, BELOW the 15 visible data rows
        // (y=100..400) and ABOVE the footer chrome (y=476..520).
        // Earlier versions sat the strip at y=40..60 which physically
        // collided with the y=8..40 tab buttons defined in base.xml;
        // glyph descenders bled into the button outlines.
        constexpr int kStripX0 = 24;
        constexpr int kStripY0 = 448;
        constexpr int kStripX1 = 936;
        constexpr int kStripY1 = 466;

        constexpr int kTextY = 452;
        constexpr int kTotalX = 32;
        constexpr int kCpuX = 280;
        constexpr int kMemX = 520;
        constexpr int kScrollX = 760;

        constexpr std::size_t kMaxProcCount = 9999;

        // Background fill behind the strip text. Kept as a local
        // constant rather than a Theme entry because the strip is the
        // only consumer of this exact shade today; promoting it to the
        // palette is a separate change.
        const vec3 kStripBg(0.08f, 0.08f, 0.08f);
    }

    void render(Screen &dest,
                std::size_t procCount,
                float sumCpuPct,
                float sumMemPct,
                std::size_t scrollOffset,
                std::size_t visibleCount,
                std::size_t windowSize)
    {
        dest.drawBox(ivec2(kStripX0, kStripY0),
                     ivec2(kStripX1, kStripY1),
                     kStripBg);

        const vec3 dim = Theme::textDim();
        const vec3 accent = Theme::accentGreen();

        std::size_t n = procCount;
        if (n > kMaxProcCount)
        {
            n = kMaxProcCount;
        }
        const std::string totalText = "Total: " + std::to_string(n) + " procs";
        const std::string cpuText = "Sum CPU: " + formatPct(sumCpuPct) + "%";
        const std::string memText = "Sum MEM: " + formatPct(sumMemPct) + "%";

        Font::drawText(dest, totalText, ivec2(kTotalX, kTextY), accent);
        Font::drawText(dest, cpuText, ivec2(kCpuX, kTextY), dim);
        Font::drawText(dest, memText, ivec2(kMemX, kTextY), dim);

        // Scroll position indicator. Only shown when the table is
        // longer than the visible window so a user with 22 processes
        // does not see a meaningless "1-15 / 22" on a static page; the
        // moment it appears it signals "PgDown/Down arrow to scroll".
        if (visibleCount > windowSize)
        {
            std::size_t firstRow = scrollOffset + 1;
            std::size_t lastRow = scrollOffset + windowSize;
            if (lastRow > visibleCount)
            {
                lastRow = visibleCount;
            }
            const std::string scrollText = std::to_string(firstRow) + ".."
                                           + std::to_string(lastRow) + " / "
                                           + std::to_string(visibleCount);
            Font::drawText(dest, scrollText, ivec2(kScrollX, kTextY), accent);
        }
    }
}
