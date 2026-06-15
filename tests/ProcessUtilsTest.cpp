#include <cmath>
#include <iostream>
#include <limits>
#include <string>
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

    ProcessInfo makeIo(int pid, unsigned long long readKb, unsigned long long writeKb)
    {
        ProcessInfo p;
        p.pid = pid;
        p.name = "x";
        p.cpuPct = 0.0f;
        p.memPct = 0.0f;
        p.diskReadKbPerSec = readKb;
        p.diskWriteKbPerSec = writeKb;
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

static void testCompareByDisk()
{
    ProcessInfo busyDisk = makeIo(1, 8192, 4096);
    ProcessInfo quietDisk = makeIo(2, 0, 0);
    report("compareProcessByColumn DISK desc puts busiest disk first",
           compareProcessByColumn(busyDisk, quietDisk, SORT_DISK, false));
    report("compareProcessByColumn DISK asc puts quietest disk first",
           compareProcessByColumn(quietDisk, busyDisk, SORT_DISK, true));
}

static void testFormatDiskIoIdle()
{
    report("formatDiskIo 0/0 yields empty string",
           formatDiskIo(0, 0).empty());
}

static void testFormatDiskIoKilobytes()
{
    report("formatDiskIo 100KB read + 50KB write yields '150 KB/s'",
           formatDiskIo(100, 50) == "150 KB/s");
}

static void testFormatDiskIoBoundary1023KB()
{
    // 1023 KB/s must stay in the KB branch; the >= 1024 guard sits
    // immediately above and an off-by-one would silently mislabel.
    report("formatDiskIo 1023 KB/s stays in KB branch",
           formatDiskIo(1023, 0) == "1023 KB/s");
    report("formatDiskIo 512+511 KB/s stays in KB branch",
           formatDiskIo(512, 511) == "1023 KB/s");
}

static void testComputeIoKbPerSecSubKBFloors()
{
    // Sub-KB delta truncates to 0 KB/sec (documented floor).
    report("computeIoKbPerSec 1023 B/s floors to 0",
           computeIoKbPerSec(0, 1023, 1.0) == 0);
    report("computeIoKbPerSec 1500 B/s yields 1 KB/s",
           computeIoKbPerSec(0, 1500, 1.0) == 1);
}

static void testComputeIoKbPerSecLargeDelta()
{
    // 2 GiB delta over 2 seconds -> 1 GiB/s = 1048576 KB/s. Must not
    // overflow the uint64 intermediate.
    std::uint64_t twoGib = 2ULL * 1024ULL * 1024ULL * 1024ULL;
    report("computeIoKbPerSec 1 GiB/s does not overflow",
           computeIoKbPerSec(0, twoGib, 2.0) == 1024ULL * 1024ULL);
}

static void testClampPollIntervalMsValid()
{
    report("clampPollIntervalMs 500 passes through",
           clampPollIntervalMs("500", 500) == 500);
    report("clampPollIntervalMs 1500 passes through",
           clampPollIntervalMs("1500", 500) == 1500);
}

static void testClampPollIntervalMsClampLow()
{
    report("clampPollIntervalMs 50 clamps to 100",
           clampPollIntervalMs("50", 500) == 100);
}

static void testClampPollIntervalMsClampHigh()
{
    report("clampPollIntervalMs 99999999 clamps to 10000",
           clampPollIntervalMs("99999999", 500) == 10000);
}

static void testClampPollIntervalMsInvalidInputs()
{
    // Negative input must NOT wrap to a huge unsigned and clamp to 10000;
    // the agent flagged this as the main parse bug.
    report("clampPollIntervalMs -1 falls back",
           clampPollIntervalMs("-1", 500) == 500);
    report("clampPollIntervalMs empty falls back",
           clampPollIntervalMs("", 500) == 500);
    report("clampPollIntervalMs null falls back",
           clampPollIntervalMs(nullptr, 500) == 500);
    report("clampPollIntervalMs garbage falls back",
           clampPollIntervalMs("abc", 500) == 500);
    report("clampPollIntervalMs zero falls back",
           clampPollIntervalMs("0", 500) == 500);
}

static void testPointInRect()
{
    IVec2 minXY{820, 478};
    IVec2 maxXY{944, 520};
    report("pointInRect interior point is in",
           pointInRect(IVec2{900, 500}, minXY, maxXY));
    report("pointInRect top-left corner is in (inclusive)",
           pointInRect(IVec2{820, 478}, minXY, maxXY));
    report("pointInRect bottom-right corner is in (inclusive)",
           pointInRect(IVec2{944, 520}, minXY, maxXY));
    report("pointInRect one past right is out",
           pointInRect(IVec2{945, 500}, minXY, maxXY) == false);
    report("pointInRect one past bottom is out",
           pointInRect(IVec2{900, 521}, minXY, maxXY) == false);
    report("pointInRect one before left is out",
           pointInRect(IVec2{819, 500}, minXY, maxXY) == false);
}

static void testFormatUptimeStringSubMinute()
{
    // Pin the current behavior: any 1..59 seconds renders as 'Up 0m'.
    report("formatUptimeString 45s yields 'Up 0m'",
           formatUptimeString(45) == "Up 0m");
    report("formatUptimeString 59s yields 'Up 0m'",
           formatUptimeString(59) == "Up 0m");
}

static void testFormatGbString()
{
    report("formatGbString 0 KB -> '0'",
           formatGbString(0) == "0");
    report("formatGbString sub-GB rounds down to 0",
           formatGbString(512) == "0");
    report("formatGbString exactly 1 GB -> '1'",
           formatGbString(1024ULL * 1024ULL) == "1");
    report("formatGbString 234 GB -> '234'",
           formatGbString(234ULL * 1024ULL * 1024ULL) == "234");
}

static void testComputeDiskUsedPct()
{
    report("computeDiskUsedPct total 0 yields 0",
           computeDiskUsedPct(0, 0) == 0.0f);
    report("computeDiskUsedPct free > total yields 0 (bad data)",
           computeDiskUsedPct(100, 200) == 0.0f);
    report("computeDiskUsedPct 50% used",
           computeDiskUsedPct(1000, 500) == 50.0f);
    report("computeDiskUsedPct 85% used (red threshold)",
           computeDiskUsedPct(100, 15) == 85.0f);
}

static void testFormatDiskIoMegabytesThreshold()
{
    // 1024 KB/s is the MB/s threshold.
    report("formatDiskIo at 1024KB/s yields '1.0 MB/s'",
           formatDiskIo(1024, 0) == "1.0 MB/s");
    // 13.3 MB/s composed of read 10MB + write 3.3MB.
    report("formatDiskIo at 13312 + 3072 KB/s yields '16.0 MB/s'",
           formatDiskIo(13312, 3072) == "16.0 MB/s");
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

static void testFormatPctNegative()
{
    // Negative percentages can show up briefly when a delta is computed
    // against a stale baseline; the formatter must not crash and must
    // preserve the minus sign + one decimal.
    report("formatPct -5.25 yields '-5.2' or '-5.3'",
           formatPct(-5.25f) == "-5.2" || formatPct(-5.25f) == "-5.3");
    report("formatPct -0.0 yields '-0.0' or '0.0'",
           formatPct(-0.0f) == "-0.0" || formatPct(-0.0f) == "0.0");
    report("formatPct -100 yields '-100.0'",
           formatPct(-100.0f) == "-100.0");
}

static void testFormatPctNonFinite()
{
    // libc renders NaN as "nan" and Inf as "inf" (sign preserved). We do
    // not pin the exact spelling because Windows MSVC emits "nan(ind)"
    // and old glibc emits "-nan"; the contract is: do not crash and the
    // result contains either 'n'/'N' for NaN or 'i'/'I' for Inf.
    std::string nanStr = formatPct(std::nanf(""));
    bool nanOk = !nanStr.empty()
              && (nanStr.find('n') != std::string::npos
                  || nanStr.find('N') != std::string::npos);
    report("formatPct(NaN) produces a NaN-like token", nanOk);

    std::string infStr = formatPct(std::numeric_limits<float>::infinity());
    bool infOk = !infStr.empty()
              && (infStr.find('i') != std::string::npos
                  || infStr.find('I') != std::string::npos);
    report("formatPct(+Inf) produces an Inf-like token", infOk);

    std::string negInfStr = formatPct(-std::numeric_limits<float>::infinity());
    bool negInfOk = !negInfStr.empty()
                 && negInfStr[0] == '-'
                 && (negInfStr.find('i') != std::string::npos
                     || negInfStr.find('I') != std::string::npos);
    report("formatPct(-Inf) keeps the minus sign", negInfOk);
}

static void testCompareByDiskAscendingTie()
{
    // Equal disk totals must yield `false` for both orderings (strict-weak
    // order requirement of std::sort: !(a<b) && !(b<a) -> equivalent).
    ProcessInfo first = makeIo(1, 100, 200);
    ProcessInfo second = makeIo(2, 200, 100);
    report("compareProcessByColumn DISK asc tie returns false",
           compareProcessByColumn(first, second, SORT_DISK, true) == false);
    report("compareProcessByColumn DISK asc tie returns false (swapped)",
           compareProcessByColumn(second, first, SORT_DISK, true) == false);
    report("compareProcessByColumn DISK desc tie returns false",
           compareProcessByColumn(first, second, SORT_DISK, false) == false);
}

static void testCompareByDiskAscendingOrders()
{
    // Mirror of testCompareByDisk but exercising the ascending branch
    // explicitly with non-tied totals.
    ProcessInfo quiet = makeIo(1, 0, 100);
    ProcessInfo loud = makeIo(2, 5000, 5000);
    report("compareProcessByColumn DISK asc puts smaller total first",
           compareProcessByColumn(quiet, loud, SORT_DISK, true));
    report("compareProcessByColumn DISK asc rejects larger first",
           compareProcessByColumn(loud, quiet, SORT_DISK, true) == false);
}

static void testProcessNameMatchesFilterUnicodeBytes()
{
    // The matcher does a byte-wise lower + substring search. UTF-8 bytes
    // above 0x7F are passed through untouched, so identical UTF-8 names
    // still match, and an ASCII substring of a UTF-8 name still matches.
    report("filter matches identical UTF-8 name (cafe with accent)",
           processNameMatchesFilter("caf\xc3\xa9", "caf\xc3\xa9") == true);
    report("filter matches ASCII substring inside UTF-8 name",
           processNameMatchesFilter("caf\xc3\xa9-server", "server") == true);
    report("filter is case-insensitive on ASCII prefix of UTF-8 name",
           processNameMatchesFilter("Caf\xc3\xa9-Server", "caf") == true);
}

static void testProcessNameMatchesFilterBothEmpty()
{
    // Already covered indirectly, but pin it explicitly: two empties are
    // a match (empty needle is "match anything", including empty name).
    report("filter both empty is a match",
           processNameMatchesFilter("", "") == true);
}

static void testProcessNameMatchesFilterNeedleLongerThanName()
{
    // Non-empty name shorter than the needle never matches; this guards
    // against any accidental find()-on-reversed-args refactor.
    report("filter needle longer than name does not match",
           processNameMatchesFilter("ab", "abcdef") == false);
}

static void testProcessNameMatchesFilterUnicodeNoMatch()
{
    // Two distinct UTF-8 names share no ASCII or byte overlap: the matcher
    // must say "no match" rather than coincidentally hit on shared
    // continuation bytes.
    report("filter UTF-8 name vs unrelated UTF-8 needle does not match",
           processNameMatchesFilter("\xe4\xb8\xad\xe6\x96\x87", "caf\xc3\xa9") == false);
    report("filter empty needle against UTF-8 name still matches",
           processNameMatchesFilter("\xe4\xb8\xad\xe6\x96\x87", "") == true);
}

static void testComputeIoKbPerSecWrapByOne()
{
    // Cumulative counter rolled back by 1 (very common on Windows when a
    // PID is reused). Underflow would produce ~uint64 max -> huge KB/s.
    report("computeIoKbPerSec curr=prev-1 returns 0",
           computeIoKbPerSec(1000, 999, 1.0) == 0);
}

static void testComputeIoKbPerSecWrapFromMaxUint()
{
    // Extreme wrap: previous was near uint64 max, current is 0.
    // computeIoKbPerSec must return 0 rather than a colossal delta.
    std::uint64_t nearMax = std::numeric_limits<std::uint64_t>::max() - 1ULL;
    report("computeIoKbPerSec wrap from near-max to 0 returns 0",
           computeIoKbPerSec(nearMax, 0, 1.0) == 0);
}

static void testClampPollIntervalMsExactLowerBoundary()
{
    // 100 is the minimum allowed value (inclusive). Off-by-one here would
    // surface as a UX bug where the documented minimum gets bumped.
    report("clampPollIntervalMs exactly 100 passes through",
           clampPollIntervalMs("100", 500) == 100);
    report("clampPollIntervalMs 99 clamps up to 100",
           clampPollIntervalMs("99", 500) == 100);
    report("clampPollIntervalMs 101 passes through",
           clampPollIntervalMs("101", 500) == 101);
}

static void testClampPollIntervalMsExactUpperBoundary()
{
    // 10000 is the maximum allowed value (inclusive).
    report("clampPollIntervalMs exactly 10000 passes through",
           clampPollIntervalMs("10000", 500) == 10000);
    report("clampPollIntervalMs 9999 passes through",
           clampPollIntervalMs("9999", 500) == 9999);
    report("clampPollIntervalMs 10001 clamps down to 10000",
           clampPollIntervalMs("10001", 500) == 10000);
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
    testCompareByDisk();
    testFormatDiskIoIdle();
    testFormatDiskIoKilobytes();
    testFormatDiskIoBoundary1023KB();
    testFormatDiskIoMegabytesThreshold();
    testComputeIoKbPerSecSubKBFloors();
    testComputeIoKbPerSecLargeDelta();
    testFormatGbString();
    testComputeDiskUsedPct();
    testClampPollIntervalMsValid();
    testClampPollIntervalMsClampLow();
    testClampPollIntervalMsClampHigh();
    testClampPollIntervalMsInvalidInputs();
    testPointInRect();

    testFormatUptimeStringSubMinute();
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

    testFormatPctNegative();
    testFormatPctNonFinite();
    testCompareByDiskAscendingTie();
    testCompareByDiskAscendingOrders();
    testProcessNameMatchesFilterUnicodeBytes();
    testProcessNameMatchesFilterUnicodeNoMatch();
    testProcessNameMatchesFilterBothEmpty();
    testProcessNameMatchesFilterNeedleLongerThanName();
    testComputeIoKbPerSecWrapByOne();
    testComputeIoKbPerSecWrapFromMaxUint();
    testClampPollIntervalMsExactLowerBoundary();
    testClampPollIntervalMsExactUpperBoundary();

    std::cout << '\n';
    std::cout << "  Failures: " << g_failures << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  ProcessUtils tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
