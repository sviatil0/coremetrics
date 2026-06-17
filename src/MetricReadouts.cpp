#include "MetricReadouts.hpp"

#include "ProcessUtils.hpp"
#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Mirrors the shape of src/UptimeAndLoad.cpp: pure paint, no state, the
// caller passes the three percentages by value. First visible use of the
// new Font::Size::Title typography size (PR #217). Paints each card's
// readout in two passes: the numeric value at Title (28 pt) in
// Theme::textPrimary, then the trailing "%" sign at Body (20 pt) in
// Theme::textDim so the number is the visual focal point.

namespace
{
    // Card right edge sits at x=952 in base.xml. The old XML labels were
    // anchored at x=876 next to the body-size readout; Title glyphs are
    // wider, so we pull the value anchor in to x=836 to keep the "%"
    // sign clear of the card's right border.
    constexpr int VALUE_X = 836;
    // CPU card box: y=80..120; RAM card: y=128..168; GPU card: y=176..216.
    // Title glyphs are ~28 px tall, so a baseline 16 px above the card
    // center seats the cap height on the centerline and leaves the
    // descenders inside the card.
    constexpr int CPU_VALUE_Y = 84;
    constexpr int RAM_VALUE_Y = 132;
    constexpr int GPU_VALUE_Y = 180;
    // "%" sits at body size next to a 4-character value like "65.2"; the
    // 88 px offset places it immediately after the Title glyph block
    // without bumping into the card's right edge at x=952.
    constexpr int PERCENT_X_OFFSET = 88;
    // Vertical nudge so the smaller body-size "%" baseline aligns with
    // the visual centerline of the Title number rather than its top.
    constexpr int PERCENT_Y_NUDGE = 8;
}

namespace MetricReadouts
{
    static void paintOne(Screen &dest, float pct, int y)
    {
        const std::string valueText = formatPct(pct);
        Font::drawText(dest, valueText,
                       ivec2(VALUE_X, y),
                       Theme::textPrimary(),
                       Font::Size::Title);
        Font::drawText(dest, "%",
                       ivec2(VALUE_X + PERCENT_X_OFFSET, y + PERCENT_Y_NUDGE),
                       Theme::textDim(),
                       Font::Size::Body);
    }

    void render(Screen &dest, float cpuPct, float ramPct, float gpuPct)
    {
        paintOne(dest, cpuPct, CPU_VALUE_Y);
        paintOne(dest, ramPct, RAM_VALUE_Y);
        paintOne(dest, gpuPct, GPU_VALUE_Y);
    }
}
