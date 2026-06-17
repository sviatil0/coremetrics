#include "StatusFlash.hpp"

#include <string>

#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as the final step of the
// COLOR_ACCENT_GREEN -> Theme token migration (PRs #207, #216, #218).
// The flash anchors at the right end of the footer status strip,
// before the EXIT button, and reuses the load-success accent green
// so it reads as part of the same status row rather than a foreign
// popup. Geometry constants now live next to their only user.

namespace StatusFlash
{
    namespace
    {
        constexpr int kTextX = 540;
        constexpr int kTextY = 492;
    }

    void render(Screen &dest, const std::string &text)
    {
        Font::drawText(dest, text, ivec2(kTextX, kTextY), Theme::accentGreen());
    }
}
