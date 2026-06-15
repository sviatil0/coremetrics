#ifndef __THEME_HPP__
#define __THEME_HPP__

#include "vec3.hpp"

// Single source of truth for the CoreMetrics dark color palette. Every
// background, panel, accent, and text shade used by the renderer should
// flow through these getters so swapping to a light theme or a
// high-contrast theme later is a one-file change. The accent triplet
// here intentionally matches Thresholds::colorGreen/Yellow/Red so the
// load palette and the general accent palette stay visually consistent.
namespace Theme
{
    inline vec3 bgDark()
    {
        return vec3(0.05f, 0.05f, 0.05f);
    }

    inline vec3 accentGreen()
    {
        return vec3(0.871f, 1.0f, 0.608f);
    }

    inline vec3 accentYellow()
    {
        return vec3(0.95f, 0.82f, 0.40f);
    }

    inline vec3 accentRed()
    {
        return vec3(0.95f, 0.35f, 0.35f);
    }

    inline vec3 textPrimary()
    {
        return vec3(0.85f, 0.85f, 0.85f);
    }

    inline vec3 textDim()
    {
        return vec3(0.55f, 0.55f, 0.55f);
    }

    inline vec3 textVeryDim()
    {
        return vec3(0.30f, 0.30f, 0.30f);
    }

    inline vec3 panelBg()
    {
        return vec3(0.12f, 0.12f, 0.12f);
    }
}

#endif
