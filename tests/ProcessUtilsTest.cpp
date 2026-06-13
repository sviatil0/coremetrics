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

    std::cout << '\n';
    std::cout << "  Failures: " << g_failures << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  ProcessUtils tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
