#include "EmptyStates.hpp"

#include <string>

#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

namespace EmptyStates
{
    // Roughly centered on the 960 px window. The 29-char hint rendered
    // through Font::drawText at the default 20 pt Roboto weight spans
    // ~300 px, so HINT_X=330 leaves ~330 px on the left edge and ~330
    // px on the right. y=240 puts the hint at the visual middle of the
    // 16-row table (rows span ~y=100..420) so the eye catches it
    // without crowding the column headers.
    constexpr int HINT_X = 330;
    constexpr int HINT_Y = 240;

    void renderProcessesEmpty(Screen &dest)
    {
        const std::string hint = "No processes yet. Sampling...";
        const vec3 dimColor = Theme::textDim();
        Font::drawText(dest, hint, ivec2(HINT_X, HINT_Y), dimColor);
    }
}
