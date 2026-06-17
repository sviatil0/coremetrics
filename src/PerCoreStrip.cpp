#include "PerCoreStrip.hpp"

#include <cstddef>

#include "Theme.hpp"
#include "Thresholds.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 7 of the GUI
// evolution spec. Geometry constants now live next to their only user.

namespace PerCoreStrip
{
    namespace
    {
        constexpr int kStripX0 = 84;
        constexpr int kStripX1 = 936;
        constexpr int kStripY0 = 218;
        constexpr int kStripY1 = 236;
        constexpr int kStripGap = 4;
    }

    void render(Screen &dest,
                const std::vector<float> &perCoreCpu,
                bool sparklinesEnabled)
    {
        if (perCoreCpu.empty())
        {
            return;
        }
        // Left-edge label sits in the same x=24 column as the
        // CPU/RAM/GPU labels above. Hidden when --sparklines is
        // enabled because the CPU history label at y=230 would crowd
        // it.
        if (!sparklinesEnabled)
        {
            const vec3 dim = Theme::textDim();
            Font::drawText(dest, "cores", ivec2(24, 218), dim);
        }
        const std::size_t cores = perCoreCpu.size();
        const int totalGap = kStripGap * static_cast<int>(cores - 1);
        const int slot =
            (kStripX1 - kStripX0 - totalGap) / static_cast<int>(cores);
        if (slot <= 0)
        {
            return;
        }
        const vec3 bg = Theme::panelBg();

        for (std::size_t i = 0; i < cores; ++i)
        {
            const int x0 = kStripX0 + static_cast<int>(i) * (slot + kStripGap);
            const int x1 = x0 + slot;
            dest.drawBox(ivec2(x0, kStripY0), ivec2(x1, kStripY1), bg);

            float ratio = perCoreCpu[i] / 100.0f;
            if (ratio <= 0.0f)
            {
                continue;
            }
            if (ratio > 1.0f)
            {
                ratio = 1.0f;
            }
            const int fillW =
                static_cast<int>(ratio * static_cast<float>(slot));
            if (fillW <= 0)
            {
                continue;
            }
            const vec3 color = Thresholds::colorForRatio(ratio);
            dest.drawBox(ivec2(x0, kStripY0),
                         ivec2(x0 + fillW, kStripY1),
                         color);
        }
    }
}
