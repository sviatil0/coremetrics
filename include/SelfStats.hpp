#ifndef __SELFSTATS_HPP__
#define __SELFSTATS_HPP__

#include "screen.hpp"

// Self-monitoring: expose the running process's own RSS / CPU% /
// uptime / on-disk binary size so the live UI can paint "look how
// little I cost" without an external profiler. Pillar E of the v0.4
// roadmap. The footprint marketing in README ("1.6 MB binary, ~60 MB
// resident") is checkable at runtime by anyone looking at the footer
// badge or the About tab.
//
// Backend: where possible we filter SystemMetrics::topProcesses(...)
// by getpid() so the values match the per-process table the rest of
// the app already consults. If the current process is not in the top-N
// window (small machine + heavy background load), we fall back to a
// platform-specific single-process read so the badge never goes blank.
//
// Peak RSS is tracked across the process lifetime as a file-static
// inside SelfStats.cpp and bumped on every read() call. Process
// uptime is similarly derived from a file-static start time captured
// on the first read(). Binary size is the file size of argv[0] (we
// resolve the running executable path internally and cache the
// answer after the first successful read so the per-frame cost is a
// single struct copy).
namespace SelfStats
{
    struct Snapshot
    {
        unsigned long long rssBytes = 0;
        unsigned long long peakRssBytes = 0;
        float cpuPct = 0.0f;
        unsigned long long binaryBytes = 0;
        unsigned long long processUptimeSeconds = 0;
    };

    // Read the current process's stats. Safe to call from any thread;
    // platform calls are cheap. Backend uses getpid() + the existing
    // SystemMetrics::topProcesses() helper, falling back to a
    // platform-specific call (/proc/self/status on linux, mach
    // task_info on macOS, GetProcessMemoryInfo on windows).
    Snapshot read();

    // Paint a "self RSS 60M CPU 0.5%" badge at the right edge of the
    // footer chrome so every screenshot doubles as evidence of the
    // app's own footprint. Numeric values render in accentGreen,
    // labels in textDim, matching the rest of the footer palette.
    void renderFooterBadge(Screen &dest);
}

#endif
