#ifndef __THRESHOLDS_HPP__
#define __THRESHOLDS_HPP__

#include "vec3.hpp"

// Single source of truth for the three-stop load palette used by Bar
// fills, the per-core strip, the memory breakdown segments, the
// percentage readout text, and any other metric that wants to show
// 'pressure rising' green to yellow to red. Centralizing prevents the
// palette from drifting across call sites.
//
// The hex values match Theme::accentGreen / accentYellow / accentRed
// exactly. Tokyo Night-inspired so they stay readable against the
// panelBg layer.
namespace Thresholds
{
    inline constexpr float YELLOW_RATIO = 0.60f;
    inline constexpr float RED_RATIO    = 0.80f;
    inline constexpr float YELLOW_PCT   = 60.0f;
    inline constexpr float RED_PCT      = 80.0f;

    inline vec3 colorGreen()
    {
        return vec3(0.620f, 0.808f, 0.416f); // #9ece6a
    }

    inline vec3 colorYellow()
    {
        return vec3(0.878f, 0.686f, 0.408f); // #e0af68
    }

    inline vec3 colorRed()
    {
        return vec3(0.969f, 0.463f, 0.557f); // #f7768e
    }

    inline vec3 colorForRatio(float ratio)
    {
        if (ratio > RED_RATIO)
        {
            return colorRed();
        }
        if (ratio > YELLOW_RATIO)
        {
            return colorYellow();
        }
        return colorGreen();
    }

    inline vec3 colorForPct(float pct)
    {
        if (pct >= RED_PCT)
        {
            return colorRed();
        }
        if (pct >= YELLOW_PCT)
        {
            return colorYellow();
        }
        return colorGreen();
    }
}

#endif
