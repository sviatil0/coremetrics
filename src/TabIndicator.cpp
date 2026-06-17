#include "TabIndicator.hpp"

#include "Theme.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Geometry mirrors base.xml's tabbar button definitions. Hardcoded
// here so the indicator does not have to walk the layout tree every
// frame. base.xml owns the buttons, this file owns the matching
// underline, and the two are kept in lockstep by reviewers.
namespace TabIndicator
{
    constexpr int STRIPE_Y0 = 42;
    constexpr int STRIPE_Y1 = 44;

    constexpr int SYSTEM_X0 = 8;
    constexpr int SYSTEM_X1 = 272;

    constexpr int PROCESSES_X0 = 280;
    constexpr int PROCESSES_X1 = 544;

    constexpr int ABOUT_X0 = 552;
    constexpr int ABOUT_X1 = 804;

    void render(Screen &dest, int tabIndex)
    {
        int x0 = 0;
        int x1 = 0;
        if (tabIndex == 0)
        {
            x0 = SYSTEM_X0;
            x1 = SYSTEM_X1;
        }
        else if (tabIndex == 1)
        {
            x0 = PROCESSES_X0;
            x1 = PROCESSES_X1;
        }
        else if (tabIndex == 2)
        {
            x0 = ABOUT_X0;
            x1 = ABOUT_X1;
        }
        else
        {
            // Unknown tab index. Stay inert rather than guessing a
            // position so a new tab added to base.xml does not silently
            // ship a misaligned stripe.
            return;
        }

        const vec3 stripeColor = Theme::accent();
        dest.drawBox(ivec2(x0, STRIPE_Y0), ivec2(x1, STRIPE_Y1), stripeColor);
    }
}
