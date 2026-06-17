#ifndef __THEME_HPP__
#define __THEME_HPP__

#include "vec3.hpp"

// Single source of truth for the CoreMetrics dark color palette. Every
// background, panel, accent, and text shade used by the renderer flows
// through these getters so swapping to a light theme or a high-contrast
// theme later is a one-file change. The accent triplet here
// intentionally matches Thresholds::colorGreen/Yellow/Red so the load
// palette and the general accent palette stay visually consistent.
//
// Palette inspired by Tokyo Night. The redesign separated bgBase from
// panelBg by 4 percentage points of luminance so cards visibly float
// off the background, and bumped textPrimary from ~85% gray to a soft
// off-white (#c0caf5) to recover the contrast budget that the previous
// flat ~5% gray-on-5% gray scheme was burning.
namespace Theme
{
    // Deepest layer. The window background that everything else stacks
    // on.
    inline vec3 bgBase()
    {
        return vec3(0.102f, 0.106f, 0.149f); // #1a1b26
    }

    // Legacy alias kept so existing call sites compile during the
    // visual lift. New code should prefer bgBase().
    inline vec3 bgDark()
    {
        return bgBase();
    }

    // Elevated layer. Cards, table rows, header strip, tab strip
    // background.
    inline vec3 panelBg()
    {
        return vec3(0.141f, 0.157f, 0.231f); // #24283b
    }

    // Slightly raised layer for hover / active states on top of panels.
    inline vec3 panelHover()
    {
        return vec3(0.184f, 0.200f, 0.302f); // #2f334d
    }

    // 1 px stroke used to delineate cards from the background.
    inline vec3 panelBorder()
    {
        return vec3(0.255f, 0.282f, 0.408f); // #414868
    }

    // Brand / "go look here" accent. Active-tab underline, primary
    // buttons, focused widgets.
    inline vec3 accent()
    {
        return vec3(0.478f, 0.635f, 0.969f); // #7aa2f7
    }

    // The three-stop load palette. Kept synced with Thresholds so a
    // single color edit propagates everywhere a pressure metric is
    // shown.
    inline vec3 accentGreen()
    {
        return vec3(0.620f, 0.808f, 0.416f); // #9ece6a
    }

    inline vec3 accentYellow()
    {
        return vec3(0.878f, 0.686f, 0.408f); // #e0af68
    }

    inline vec3 accentRed()
    {
        return vec3(0.969f, 0.463f, 0.557f); // #f7768e
    }

    // High-emphasis text. Section headers, key numbers, hovered rows.
    inline vec3 textPrimary()
    {
        return vec3(0.753f, 0.792f, 0.961f); // #c0caf5
    }

    // Mid-emphasis text. Default body text, regular table rows, labels.
    inline vec3 textDim()
    {
        return vec3(0.604f, 0.647f, 0.808f); // #9aa5ce
    }

    // Low-emphasis text. Captions, disabled fields, dividers.
    inline vec3 textVeryDim()
    {
        return vec3(0.337f, 0.373f, 0.537f); // #565f89
    }
}

#endif
