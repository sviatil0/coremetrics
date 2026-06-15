#ifndef __THRESHOLDS_HPP__
#define __THRESHOLDS_HPP__

#include "vec3.hpp"

// Single source of truth for the three-stop load palette used by Bar
// fills, the per-core strip, the memory breakdown segments, the
// percentage readout text, and any other metric that wants to show
// 'pressure rising' green to yellow to red. Centralizing prevents the
// palette from drifting across call sites.
namespace Thresholds
{
    inline constexpr float YELLOW_RATIO = 0.60f;
    inline constexpr float RED_RATIO    = 0.80f;
    inline constexpr float YELLOW_PCT   = 60.0f;
    inline constexpr float RED_PCT      = 80.0f;

    inline vec3 colorGreen()
    {
        return vec3(0.871f, 1.0f, 0.608f);
    }

    inline vec3 colorYellow()
    {
        return vec3(0.95f, 0.82f, 0.40f);
    }

    inline vec3 colorRed()
    {
        return vec3(0.95f, 0.35f, 0.35f);
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
