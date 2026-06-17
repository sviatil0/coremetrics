#include "TabIndicator.hpp"

#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

#include <string>

// Geometry mirrors base.xml's tabbar button definitions. Hardcoded
// here so the indicator does not have to walk the layout tree every
// frame. base.xml owns the buttons, this file owns the matching
// underline, and the two are kept in lockstep by reviewers.
namespace TabIndicator
{
    constexpr int STRIPE_Y0 = 42;
    constexpr int STRIPE_Y1 = 44;

    // Hover-tint rect sits behind the active tab button. y range is
    // inset by 1 px from the button's 8..40 frame so the button's own
    // border still reads. The tint is the same panelHover swatch the
    // rest of the UI uses for active / hover states, so the active tab
    // visually "lifts" off the tabbar strip.
    constexpr int HOVER_Y0 = 9;
    constexpr int HOVER_Y1 = 40;

    constexpr int SYSTEM_X0 = 8;
    constexpr int SYSTEM_X1 = 272;
    constexpr int SYSTEM_LABEL_X = 20;

    constexpr int PROCESSES_X0 = 280;
    constexpr int PROCESSES_X1 = 544;
    constexpr int PROCESSES_LABEL_X = 292;

    constexpr int ABOUT_X0 = 552;
    constexpr int ABOUT_X1 = 804;
    constexpr int ABOUT_LABEL_X = 564;

    // All three tab labels live on the same baseline (y=14 in base.xml)
    // so a single constant keeps every redraw site in lockstep with the
    // XML.
    constexpr int LABEL_Y = 14;

    void render(Screen &dest, int tabIndex)
    {
        int x0 = 0;
        int x1 = 0;
        int labelX = 0;
        std::string labelText;
        if (tabIndex == 0)
        {
            x0 = SYSTEM_X0;
            x1 = SYSTEM_X1;
            labelX = SYSTEM_LABEL_X;
            labelText = "System";
        }
        else if (tabIndex == 1)
        {
            x0 = PROCESSES_X0;
            x1 = PROCESSES_X1;
            labelX = PROCESSES_LABEL_X;
            labelText = "Processes";
        }
        else if (tabIndex == 2)
        {
            x0 = ABOUT_X0;
            x1 = ABOUT_X1;
            labelX = ABOUT_LABEL_X;
            labelText = "About";
        }
        else
        {
            // Unknown tab index. Stay inert rather than guessing a
            // position so a new tab added to base.xml does not silently
            // ship a misaligned stripe.
            return;
        }

        // Paint the panelHover fill FIRST so the accent stripe paints
        // on top of the tint, not under it. The tint is inset 1 px from
        // the button's 8..40 frame so the button's own border still
        // reads through.
        const vec3 hoverColor = Theme::panelHover();
        dest.drawBox(ivec2(x0, HOVER_Y0), ivec2(x1, HOVER_Y1), hoverColor);

        // base.xml paints the tab label BEFORE TabIndicator runs, so the
        // hover fill above clobbers it. Redraw the same string in the
        // same textPrimary swatch on top of the tint so the active tab
        // still reads "Processes" / "System".
        Font::drawText(dest, labelText, ivec2(labelX, LABEL_Y), Theme::textPrimary());

        const vec3 stripeColor = Theme::accent();
        dest.drawBox(ivec2(x0, STRIPE_Y0), ivec2(x1, STRIPE_Y1), stripeColor);
    }
}
