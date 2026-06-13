#include <cmath>
#include <iostream>
#include "ProcessUtilsTest.hpp"
#include "ProcessUtils.hpp"

namespace
{
    int g_failures = 0;

    void report(const char *label, bool passed)
    {
        std::cout << "  " << label << ": " << (passed ? "PASS" : "FAIL") << '\n';
        if (!passed)
        {
            ++g_failures;
        }
    }

    bool nearly(float a, float b)
    {
        return std::fabs(a - b) < 0.001f;
    }

    ProcessInfo make(int pid, const std::string &name, float cpu, float mem)
    {
        ProcessInfo p;
        p.pid = pid;
        p.name = name;
        p.cpuPct = cpu;
        p.memPct = mem;
        return p;
    }
}

static void testFormatPctRoundsToOneDecimal()
{
    report("formatPct rounds to one decimal", formatPct(12.345f) == "12.3");
    report("formatPct of zero is 0.0", formatPct(0.0f) == "0.0");
    report("formatPct of 100 is 100.0", formatPct(100.0f) == "100.0");
}

static void testComputeCpuPercentDeltaDenomZero()
{
    report("computeCpuPercentDelta denom 0 returns 0", computeCpuPercentDelta(50, 25, 0) == 0.0f);
}

static void testComputeCpuPercentDeltaProcWentBackwards()
{
    report("computeCpuPercentDelta proc reset returns 0", computeCpuPercentDelta(10, 100, 500) == 0.0f);
}

static void testComputeCpuPercentDeltaTenPercent()
{
    float pct = computeCpuPercentDelta(1100, 1000, 1000);
    report("computeCpuPercentDelta proc used 10% of window", nearly(pct, 10.0f));
}

static void testComputeCpuPercentDeltaFullSaturation()
{
    float pct = computeCpuPercentDelta(2000, 1000, 1000);
    report("computeCpuPercentDelta saturated at 100", nearly(pct, 100.0f));
}

static void testComputeCpuPercentDeltaClampsOverflow()
{
    float pct = computeCpuPercentDelta(5000, 1000, 1000);
    report("computeCpuPercentDelta clamps overflow to 100", nearly(pct, 100.0f));
}

static void testComputeCpuPercentDeltaEqual()
{
    report("computeCpuPercentDelta proc idle returns 0",
           computeCpuPercentDelta(1000, 1000, 1000) == 0.0f);
}

static void testCompareByPidDesc()
{
    ProcessInfo a = make(100, "a", 5.0f, 5.0f);
    ProcessInfo b = make(50, "b", 5.0f, 5.0f);
    report("compareProcessByColumn PID desc puts larger pid first",
           compareProcessByColumn(a, b, SORT_PID, false));
}

static void testCompareByPidAsc()
{
    ProcessInfo a = make(100, "a", 5.0f, 5.0f);
    ProcessInfo b = make(50, "b", 5.0f, 5.0f);
    report("compareProcessByColumn PID asc puts smaller pid first",
           compareProcessByColumn(b, a, SORT_PID, true));
}

static void testCompareByName()
{
    ProcessInfo a = make(1, "alpha", 0, 0);
    ProcessInfo b = make(2, "beta", 0, 0);
    report("compareProcessByColumn NAME asc puts a-prefix first",
           compareProcessByColumn(a, b, SORT_NAME, true));
    report("compareProcessByColumn NAME desc puts b-prefix first",
           compareProcessByColumn(b, a, SORT_NAME, false));
}

static void testCompareByCpu()
{
    ProcessInfo busy = make(1, "x", 99.0f, 0);
    ProcessInfo idle = make(2, "y", 0.1f, 0);
    report("compareProcessByColumn CPU desc puts busiest first",
           compareProcessByColumn(busy, idle, SORT_CPU, false));
    report("compareProcessByColumn CPU asc puts idle first",
           compareProcessByColumn(idle, busy, SORT_CPU, true));
}

static void testCompareByMem()
{
    ProcessInfo heavy = make(1, "x", 0, 90.0f);
    ProcessInfo light = make(2, "y", 0, 0.5f);
    report("compareProcessByColumn MEM desc puts heaviest first",
           compareProcessByColumn(heavy, light, SORT_MEM, false));
    report("compareProcessByColumn MEM asc puts lightest first",
           compareProcessByColumn(light, heavy, SORT_MEM, true));
}

static void testCompareUnknownColumnFallback()
{
    // An unrecognized column value (e.g. an enum value added later in the
    // header but missing a case here) must fall through to `return false`.
    // This pins the contract so a future enum addition that forgets a
    // switch case becomes a test failure rather than a silent wrong sort.
    ProcessInfo a = make(1, "a", 5.0f, 5.0f);
    ProcessInfo b = make(2, "b", 5.0f, 5.0f);
    SortColumn unknown = static_cast<SortColumn>(999);
    report("compareProcessByColumn unknown column returns false (asc)",
           compareProcessByColumn(a, b, unknown, true) == false);
    report("compareProcessByColumn unknown column returns false (desc)",
           compareProcessByColumn(b, a, unknown, false) == false);
}

static void testComputeIoKbPerSecZeroElapsed()
{
    report("computeIoKbPerSec elapsed 0 returns 0",
           computeIoKbPerSec(0, 1024 * 1024, 0.0) == 0);
}

static void testComputeIoKbPerSecNegativeElapsed()
{
    report("computeIoKbPerSec elapsed < 0 returns 0",
           computeIoKbPerSec(0, 1024 * 1024, -1.0) == 0);
}

static void testComputeIoKbPerSecCounterReset()
{
    // PID reuse or some Windows resets can roll the cumulative counter
    // backwards; we want 0 KB/s, not a billion KB/s from underflow.
    report("computeIoKbPerSec counter reset returns 0",
           computeIoKbPerSec(2048, 1024, 1.0) == 0);
}

static void testComputeIoKbPerSecNormalDelta()
{
    // 1 MB delta over 1 second is 1024 KB/sec.
    report("computeIoKbPerSec 1MB/s",
           computeIoKbPerSec(0, 1024 * 1024, 1.0) == 1024);
    // 512 KB delta over 0.5 seconds is also 1024 KB/sec.
    report("computeIoKbPerSec scales by elapsed",
           computeIoKbPerSec(0, 512 * 1024, 0.5) == 1024);
}

static void testComputeIoKbPerSecEqualCounters()
{
    report("computeIoKbPerSec curr == prev returns 0",
           computeIoKbPerSec(1024, 1024, 1.0) == 0);
}

static void testProcessNameMatchesFilterEmptyNeedle()
{
    report("filter empty needle matches anything",
           processNameMatchesFilter("bash", "") == true);
    report("filter empty needle matches empty name",
           processNameMatchesFilter("", "") == true);
}

static void testProcessNameMatchesFilterEmptyName()
{
    report("filter empty name does not match non-empty needle",
           processNameMatchesFilter("", "bash") == false);
}

static void testProcessNameMatchesFilterCaseInsensitive()
{
    report("filter is case-insensitive (BASH matches bash)",
           processNameMatchesFilter("BASH", "bash") == true);
    report("filter is case-insensitive (bash matches BASH)",
           processNameMatchesFilter("bash", "BASH") == true);
}

static void testProcessNameMatchesFilterSubstring()
{
    report("filter matches substring at start",
           processNameMatchesFilter("postgres-server", "post") == true);
    report("filter matches substring in middle",
           processNameMatchesFilter("com.docker.backend", "docker") == true);
    report("filter rejects non-match",
           processNameMatchesFilter("kernel_task", "chrome") == false);
}

static void testFormatUptimeStringZero()
{
    report("formatUptimeString 0 yields 'Up --'",
           formatUptimeString(0) == "Up --");
}

static void testFormatUptimeStringMinutesOnly()
{
    report("formatUptimeString 0d 0h 5m -> 'Up 5m'",
           formatUptimeString(5 * 60) == "Up 5m");
}

static void testFormatUptimeStringHoursAndMinutes()
{
    // 2h 30m
    report("formatUptimeString 2h 30m -> 'Up 2h 30m'",
           formatUptimeString(2 * 3600 + 30 * 60) == "Up 2h 30m");
}

static void testFormatUptimeStringDaysHoursMinutes()
{
    // 9d 22h 35m
    unsigned long long secs = 9 * 86400 + 22 * 3600 + 35 * 60;
    report("formatUptimeString 9d 22h 35m -> 'Up 9d 22h 35m'",
           formatUptimeString(secs) == "Up 9d 22h 35m");
}

static void testFormatUptimeStringExactlyOneHour()
{
    report("formatUptimeString exactly 1h 0m -> 'Up 1h 0m'",
           formatUptimeString(3600) == "Up 1h 0m");
}

static void testFormatUptimeStringExactlyOneMinute()
{
    report("formatUptimeString exactly 1m 'Up 1m'",
           formatUptimeString(60) == "Up 1m");
}

void processUtilsTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  ProcessUtils : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    g_failures = 0;

    testFormatPctRoundsToOneDecimal();

    testComputeCpuPercentDeltaDenomZero();
    testComputeCpuPercentDeltaProcWentBackwards();
    testComputeCpuPercentDeltaTenPercent();
    testComputeCpuPercentDeltaFullSaturation();
    testComputeCpuPercentDeltaClampsOverflow();
    testComputeCpuPercentDeltaEqual();

    testCompareByPidDesc();
    testCompareByPidAsc();
    testCompareByName();
    testCompareByCpu();
    testCompareByMem();
    testCompareUnknownColumnFallback();

    testComputeIoKbPerSecZeroElapsed();
    testComputeIoKbPerSecNegativeElapsed();
    testComputeIoKbPerSecCounterReset();
    testComputeIoKbPerSecNormalDelta();
    testComputeIoKbPerSecEqualCounters();

    testProcessNameMatchesFilterEmptyNeedle();
    testProcessNameMatchesFilterEmptyName();
    testProcessNameMatchesFilterCaseInsensitive();
    testProcessNameMatchesFilterSubstring();

    testFormatUptimeStringZero();
    testFormatUptimeStringMinutesOnly();
    testFormatUptimeStringHoursAndMinutes();
    testFormatUptimeStringDaysHoursMinutes();
    testFormatUptimeStringExactlyOneHour();
    testFormatUptimeStringExactlyOneMinute();

    std::cout << '\n';
    std::cout << "  Failures: " << g_failures << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  ProcessUtils tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
