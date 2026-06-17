#include "MemBreakdownStrip.hpp"

#include "Theme.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 6 of the GUI
// evolution spec. The segPx lambda that lived inside the function
// becomes a free anonymous-namespace helper, matching the project
// rule of no lambdas outside ThreadPool::submit.

namespace MemBreakdownStrip
{
    namespace
    {
        constexpr int kStripX0 = 84;
        constexpr int kStripX1 = 864;
        constexpr int kStripY0 = 162;
        constexpr int kStripY1 = 166;

        int segmentPx(int totalWidth,
                      unsigned long long segKb,
                      unsigned long long totalKb)
        {
            // Compute as 64-bit so the multiplication does not overflow
            // on large-memory hosts (256GB+).
            const unsigned long long n =
                static_cast<unsigned long long>(totalWidth) * segKb;
            return static_cast<int>(n / totalKb);
        }
    }

    void render(Screen &dest, const MemBreakdown &mem)
    {
        if (mem.totalKb == 0)
        {
            return;
        }
        const int width = kStripX1 - kStripX0;
        if (width <= 0)
        {
            return;
        }

        const vec3 colActive = Theme::accentRed();    // in-use by processes
        const vec3 colWired(0.95f, 0.66f, 0.30f);     // kernel + pinned
        const vec3 colCached(0.45f, 0.78f, 0.95f);    // reclaimable cache
        const vec3 colFree(0.18f, 0.18f, 0.18f);      // free

        int x = kStripX0;
        const int aw = segmentPx(width, mem.activeKb, mem.totalKb);
        if (aw > 0)
        {
            dest.drawBox(ivec2(x, kStripY0), ivec2(x + aw, kStripY1), colActive);
            x += aw;
        }
        const int ww = segmentPx(width, mem.wiredKb, mem.totalKb);
        if (ww > 0)
        {
            dest.drawBox(ivec2(x, kStripY0), ivec2(x + ww, kStripY1), colWired);
            x += ww;
        }
        const int cw = segmentPx(width, mem.cachedKb, mem.totalKb);
        if (cw > 0)
        {
            dest.drawBox(ivec2(x, kStripY0), ivec2(x + cw, kStripY1), colCached);
            x += cw;
        }
        // Free fills whatever is left so rounding remainder gets absorbed
        // visually rather than leaving a sliver.
        if (x < kStripX1)
        {
            dest.drawBox(ivec2(x, kStripY0), ivec2(kStripX1, kStripY1), colFree);
        }
    }
}
