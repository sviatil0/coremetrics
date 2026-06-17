#include "HelpOverlay.hpp"
#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as the first slice of Phase 1.2 from
// the GUI evolution spec. The overlay is self-contained: no state
// reads, no globals, no side effects beyond paints onto the Screen
// passed in. The caller decides when to call render() based on
// whether the visibility flag is set.

namespace HelpOverlay
{
    void render(Screen &dest)
    {
        const int panelX0 = 120;
        const int panelY0 = 80;
        const int panelX1 = 840;
        const int panelY1 = 460;
        const vec3 panelBg = Theme::bgDark();
        const vec3 panelBorder = Theme::accentGreen();
        dest.drawBox(ivec2(panelX0, panelY0), ivec2(panelX1, panelY1), panelBg);
        // Four thin boxes draw the 1px border; cheaper than a stroked
        // rect and matches the signal menu treatment.
        dest.drawBox(ivec2(panelX0, panelY0), ivec2(panelX1, panelY0 + 1), panelBorder);
        dest.drawBox(ivec2(panelX0, panelY1 - 1), ivec2(panelX1, panelY1), panelBorder);
        dest.drawBox(ivec2(panelX0, panelY0), ivec2(panelX0 + 1, panelY1), panelBorder);
        dest.drawBox(ivec2(panelX1 - 1, panelY0), ivec2(panelX1, panelY1), panelBorder);

        const vec3 titleColor(1.0f, 1.0f, 1.0f);
        const vec3 keyColor = Theme::accentGreen();
        const vec3 descColor(0.65f, 0.65f, 0.65f);
        const vec3 hintColor(0.5f, 0.5f, 0.5f);

        Font::drawText(dest, "Keyboard shortcuts",
                       ivec2(panelX0 + 24, panelY0 + 18), titleColor);

        // Two-column layout: key glyph at x+24, description at x+160.
        // Row pitch 28px gives ~10 lines comfortably inside the 380px
        // panel.
        const int keyX = panelX0 + 24;
        const int descX = panelX0 + 160;
        int row = panelY0 + 60;
        constexpr int rowStep = 28;

        Font::drawText(dest, "?", ivec2(keyX, row), keyColor);
        Font::drawText(dest, "toggle this help", ivec2(descX, row), descColor);
        row += rowStep;

        Font::drawText(dest, "1..N", ivec2(keyX, row), keyColor);
        Font::drawText(dest, "switch tab", ivec2(descX, row), descColor);
        row += rowStep;

        Font::drawText(dest, "/", ivec2(keyX, row), keyColor);
        Font::drawText(dest, "filter processes by name", ivec2(descX, row), descColor);
        row += rowStep;

        Font::drawText(dest, "t", ivec2(keyX, row), keyColor);
        Font::drawText(dest, "toggle tree mode", ivec2(descX, row), descColor);
        row += rowStep;

        Font::drawText(dest, "up / down", ivec2(keyX, row), keyColor);
        Font::drawText(dest, "row select (auto-scroll at edge)", ivec2(descX, row), descColor);
        row += rowStep;

        Font::drawText(dest, "pgup / pgdn", ivec2(keyX, row), keyColor);
        Font::drawText(dest, "scroll process list by a page", ivec2(descX, row), descColor);
        row += rowStep;

        Font::drawText(dest, "home / end", ivec2(keyX, row), keyColor);
        Font::drawText(dest, "jump to top / bottom of list", ivec2(descX, row), descColor);
        row += rowStep;

        Font::drawText(dest, "wheel", ivec2(keyX, row), keyColor);
        Font::drawText(dest, "scroll process list (3 rows/tick)", ivec2(descX, row), descColor);
        row += rowStep;

        Font::drawText(dest, "k", ivec2(keyX, row), keyColor);
        Font::drawText(dest, "open signal / kill menu", ivec2(descX, row), descColor);
        row += rowStep;

        Font::drawText(dest, "y / enter", ivec2(keyX, row), keyColor);
        Font::drawText(dest, "confirm action", ivec2(descX, row), descColor);
        row += rowStep;

        Font::drawText(dest, "esc", ivec2(keyX, row), keyColor);
        Font::drawText(dest, "cancel / clear filter / dismiss", ivec2(descX, row), descColor);

        Font::drawText(dest, "press ? or esc to close",
                       ivec2(panelX0 + 24, panelY1 - 28), hintColor);
    }
}
