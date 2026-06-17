#include "FilterStrip.hpp"

#include <string>

#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 12 of the
// modernization roadmap. Geometry constants and the cursor-blink
// cadence now live next to their only user. Colors flow through
// Theme tokens (accentGreen / textPrimary / textDim) so the strip
// stays aligned with the Pillar A palette migration (PR #207, #216).

namespace FilterStrip
{
    namespace
    {
        constexpr int kLabelX = 24;
        constexpr int kBodyX = 120;
        constexpr int kHintX = 800;
        constexpr int kStripY = 44;
        constexpr unsigned long long kBlinkPeriodMs = 400;
    }

    void render(Screen &dest,
                bool inputActive,
                const std::string &filterText,
                unsigned long long tickMs)
    {
        const vec3 labelColor = Theme::accentGreen();
        const vec3 textColor = Theme::textPrimary();
        const vec3 hintColor = Theme::textDim();

        const std::string prefix = inputActive ? "filter> " : "filter: ";
        std::string body = filterText;
        if (inputActive && ((tickMs / kBlinkPeriodMs) % 2) == 0)
        {
            body += "_";
        }

        Font::drawText(dest, prefix, ivec2(kLabelX, kStripY), labelColor);
        Font::drawText(dest, body, ivec2(kBodyX, kStripY), textColor);
        if (!inputActive && !filterText.empty())
        {
            Font::drawText(dest, "Esc clears", ivec2(kHintX, kStripY), hintColor);
        }
    }
}
