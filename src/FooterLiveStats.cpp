#include "FooterLiveStats.hpp"

#include <cstddef>
#include <string>

#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 8 of the GUI
// evolution spec. The previous in-place helper used a hardcoded
// vec3(0.5f, 0.5f, 0.5f) gray that predated Pillar A1 (PR #207);
// the call now routes through Theme::textDim() so the footer reads
// as part of the Tokyo Night palette like the rest of the chrome.

namespace FooterLiveStats
{
    namespace
    {
        constexpr int kTextX = 370;
        constexpr int kTextY = 492;
        constexpr std::size_t kMaxProcCount = 9999;
    }

    void render(Screen &dest, std::size_t procCount)
    {
        // procs N at x=370. The DISK+NET text begins at x=460. To
        // survive a 4-digit proc count (10 chars * ~10px = 100px
        // ending at x=470), truncate the count to a max width budget
        // rather than printing arbitrary integers. Real-world process
        // counts above ~9999 are already pathological on a personal
        // machine.
        std::size_t n = procCount;
        if (n > kMaxProcCount)
        {
            n = kMaxProcCount;
        }
        const vec3 dim = Theme::textDim();
        const std::string text = std::to_string(n) + " procs";
        Font::drawText(dest, text, ivec2(kTextX, kTextY), dim);
    }
}
