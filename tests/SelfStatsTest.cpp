#include <iostream>

#include "SelfStatsTest.hpp"
#include "SelfStats.hpp"

// Pillar E of the v0.4 roadmap: live self-monitoring. The unit test
// here is intentionally narrow: it only asserts the read() call lands
// non-zero RSS + a non-zero binary size + a coherent peak. Anything
// fancier (mocking topProcesses, simulating a pid not in the top-N
// window) drags the binary into platform-specific test plumbing that
// the rest of the suite does not pay for.

static int g_failures = 0;

static void report(const char *label, bool passed)
{
    std::cout << "  " << label << ": " << (passed ? "PASS" : "FAIL") << '\n';
    if (!passed)
    {
        ++g_failures;
    }
}

static void testReadReturnsLiveSnapshot()
{
    SelfStats::Snapshot snap = SelfStats::read();
    report("SelfStats::read rssBytes > 0",
           snap.rssBytes > 0);
    report("SelfStats::read binaryBytes > 0",
           snap.binaryBytes > 0);
    report("SelfStats::read peak >= rss",
           snap.peakRssBytes >= snap.rssBytes);
    report("SelfStats::read cpuPct in [0, 100]",
           snap.cpuPct >= 0.0f && snap.cpuPct <= 100.0f);
    // Uptime is monotonic from the first call but the resolution is
    // 1 second; on a fast test runner we may still land 0 here. The
    // sanity check is "not nonsense" -- bounded above by a generous
    // ceiling so a wrap-around bug surfaces.
    report("SelfStats::read processUptimeSeconds bounded",
           snap.processUptimeSeconds < 60ULL * 60ULL * 24ULL * 365ULL);
}

void selfStatsTestSuite()
{
    std::cout << "SelfStats:\n";
    testReadReturnsLiveSnapshot();
    if (g_failures != 0)
    {
        std::cout << "SelfStats: " << g_failures << " FAILURES\n";
    }
}
