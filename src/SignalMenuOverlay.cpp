#include "SignalMenuOverlay.hpp"

#include <string>

#include "SignalUtils.hpp"
#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 11 of the GUI
// evolution spec. The previous in-place block carried a hand-tuned
// COLOR_ACCENT_GREEN triplet (0.871, 1.0, 0.608) for the hotkey list;
// the call now routes through Theme::accentGreen() so the modal sits
// on the Tokyo Night palette PR #207 / PR #216 adopted everywhere
// else. The picker line stays a distinct color from the prompt text
// so the "1 TERM 2 KILL ..." hint never reads as part of the
// question above it.

namespace SignalMenuOverlay
{
    namespace
    {
        // Centered panel. 200..760 horizontally and 200..320 vertically
        // sits the modal roughly in the middle of the 960x540 window
        // and leaves room above the footer strip (y=448..464) and
        // below the tab buttons (y=8..40). The overlay is small and
        // the message is what matters; no decorative chrome beyond a
        // 1px border so it reads as an actionable foreground element.
        constexpr int kPanelX0 = 200;
        constexpr int kPanelY0 = 200;
        constexpr int kPanelX1 = 760;
        constexpr int kPanelY1 = 320;

        // Inset for the text lines. 24px from the left edge of the
        // panel, 18px below the top edge for the first line, 50px
        // below the top edge for the second line, and 30px above the
        // bottom edge for the "Esc cancels" hint.
        constexpr int kTextInsetX = 24;
        constexpr int kLine1OffsetY = 18;
        constexpr int kLine2OffsetY = 50;
        constexpr int kHintOffsetFromBottom = 30;
    }

    void render(Screen &dest, int selectedPid, int pickedIndex)
    {
        // Pillar A palette migration (PR #207 / PR #216). Panel fill,
        // border, and text route through Theme tokens so the modal
        // tracks the Tokyo Night palette instead of pre-#207 raw
        // literals.
        //   panelBg     -> Theme::panelBg()     (elevated card layer)
        //   panelBorder -> Theme::accent()      (brand blue; signals
        //                                        actionable foreground
        //                                        and avoids blending
        //                                        with the green load
        //                                        palette)
        //   textColor   -> Theme::textPrimary() (high-emphasis copy)
        //   hintColor   -> Theme::textDim()     (mid-emphasis hint)
        //   hotkeyColor -> Theme::accentGreen() (distinguishes the
        //                                        hotkey list from the
        //                                        prompt above it)
        const vec3 panelBg = Theme::panelBg();
        const vec3 panelBorder = Theme::accent();
        dest.drawBox(ivec2(kPanelX0, kPanelY0), ivec2(kPanelX1, kPanelY1), panelBg);

        // Border. 4 thin boxes is cheaper than a stroked rectangle.
        dest.drawBox(ivec2(kPanelX0, kPanelY0),
                     ivec2(kPanelX1, kPanelY0 + 1), panelBorder);
        dest.drawBox(ivec2(kPanelX0, kPanelY1 - 1),
                     ivec2(kPanelX1, kPanelY1), panelBorder);
        dest.drawBox(ivec2(kPanelX0, kPanelY0),
                     ivec2(kPanelX0 + 1, kPanelY1), panelBorder);
        dest.drawBox(ivec2(kPanelX1 - 1, kPanelY0),
                     ivec2(kPanelX1, kPanelY1), panelBorder);

        const vec3 textColor = Theme::textPrimary();
        const vec3 hintColor = Theme::textDim();
        const vec3 hotkeyColor = Theme::accentGreen();
        if (pickedIndex < 0)
        {
            Font::drawText(dest,
                           "Send signal to pid " + std::to_string(selectedPid),
                           ivec2(kPanelX0 + kTextInsetX, kPanelY0 + kLine1OffsetY),
                           textColor);
            Font::drawText(dest,
                           "1 TERM   2 KILL   3 INT   4 HUP   5 STOP   6 CONT",
                           ivec2(kPanelX0 + kTextInsetX, kPanelY0 + kLine2OffsetY),
                           hotkeyColor);
            Font::drawText(dest,
                           "Esc cancels",
                           ivec2(kPanelX0 + kTextInsetX,
                                 kPanelY1 - kHintOffsetFromBottom),
                           hintColor);
        }
        else
        {
            SignalUtils::Signal sig =
                static_cast<SignalUtils::Signal>(pickedIndex);
            std::string prompt = std::string("Send ")
                                 + SignalUtils::name(sig)
                                 + " to pid " + std::to_string(selectedPid) + "?";
            Font::drawText(dest, prompt,
                           ivec2(kPanelX0 + kTextInsetX, kPanelY0 + kLine1OffsetY),
                           textColor);
            Font::drawText(dest,
                           "Y / Enter = send     N / Esc = cancel",
                           ivec2(kPanelX0 + kTextInsetX, kPanelY0 + kLine2OffsetY),
                           hotkeyColor);
        }
    }
}
