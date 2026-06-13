#include <iostream>
#include "SystemMetricsParseTest.hpp"
#include "ProcParsers.hpp"

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
}

static void testProcStatWellFormed()
{
    const std::string content =
        "cpu  100 50 30 800 20 10 5 0 0 0\n"
        "cpu0 50 25 15 400 10 5 2 0 0 0\n"
        "intr 12345\n";

    unsigned long long total = 0;
    unsigned long long idle = 0;
    bool ok = ProcParsers::parseProcStat(content, total, idle);
    bool passed = ok
                  && idle == 820
                  && total == (100ULL + 50 + 30 + 820 + 10 + 5);
    report("parseProcStat well-formed totals", passed);
}

static void testProcStatNoIowait()
{
    const std::string content = "cpu 10 5 5 80\n";
    unsigned long long total = 0;
    unsigned long long idle = 0;
    bool ok = ProcParsers::parseProcStat(content, total, idle);
    bool passed = ok && idle == 80 && total == 100;
    report("parseProcStat handles short line (no iowait/irq/...)", passed);
}

static void testProcStatEmpty()
{
    unsigned long long total = 99;
    unsigned long long idle = 99;
    bool ok = ProcParsers::parseProcStat("", total, idle);
    report("parseProcStat rejects empty input", !ok && total == 0 && idle == 0);
}

static void testProcStatWrongLabel()
{
    unsigned long long total = 99;
    unsigned long long idle = 99;
    bool ok = ProcParsers::parseProcStat("not-a-cpu-line 1 2 3 4\n", total, idle);
    report("parseProcStat rejects non-cpu label", !ok);
}

static void testProcPidStatSimpleName()
{
    const std::string content =
        "1234 (bash) S 1 1234 1234 0 -1 4194304 "
        "1 2 3 4 5 6 7 8 9 10 11 12 13 14\n";
    unsigned long long ticks = 0;
    bool ok = ProcParsers::parseProcPidStatCpuTicks(content, ticks);
    bool passed = ok && ticks == (5ULL + 6);
    report("parseProcPidStatCpuTicks simple name", passed);
}

static void testProcPidStatNameWithSpace()
{
    const std::string content =
        "9999 (Web Content) S 1 1 1 0 -1 0 "
        "0 0 0 0 100 250 0 0 20 0 1 0 0 0 0\n";
    unsigned long long ticks = 0;
    bool ok = ProcParsers::parseProcPidStatCpuTicks(content, ticks);
    bool passed = ok && ticks == 350;
    report("parseProcPidStatCpuTicks name with space", passed);
}

static void testProcPidStatNameWithParens()
{
    const std::string content =
        "42 (((odd))) S 1 1 1 0 -1 0 "
        "0 0 0 0 7 8 0 0 0 0 0 0 0 0 0\n";
    unsigned long long ticks = 0;
    bool ok = ProcParsers::parseProcPidStatCpuTicks(content, ticks);
    bool passed = ok && ticks == 15;
    report("parseProcPidStatCpuTicks rfind handles nested parens", passed);
}

static void testProcPidStatNoRparen()
{
    unsigned long long ticks = 0;
    bool ok = ProcParsers::parseProcPidStatCpuTicks("garbage no paren", ticks);
    report("parseProcPidStatCpuTicks rejects missing rparen", !ok && ticks == 0);
}

static void testProcPidStatEmpty()
{
    unsigned long long ticks = 99;
    bool ok = ProcParsers::parseProcPidStatCpuTicks("", ticks);
    report("parseProcPidStatCpuTicks rejects empty", !ok && ticks == 0);
}

static void testProcPidStatTruncated()
{
    const std::string content = "1 (bash) S 1 1 1 0 -1 0 0 0\n";
    unsigned long long ticks = 99;
    bool ok = ProcParsers::parseProcPidStatCpuTicks(content, ticks);
    report("parseProcPidStatCpuTicks rejects truncated line", !ok && ticks == 0);
}

static void testProcStatusVmRssPresent()
{
    const std::string content =
        "Name:\tbash\n"
        "Umask:\t0022\n"
        "VmRSS:\t   4096 kB\n"
        "VmData:\t   1024 kB\n";
    unsigned long long kb = 0;
    bool ok = ProcParsers::parseProcStatusVmRssKb(content, kb);
    report("parseProcStatusVmRssKb finds VmRSS", ok && kb == 4096);
}

static void testProcStatusVmRssAbsent()
{
    const std::string content = "Name:\tbash\nVmData:\t1024 kB\n";
    unsigned long long kb = 99;
    bool ok = ProcParsers::parseProcStatusVmRssKb(content, kb);
    report("parseProcStatusVmRssKb missing VmRSS returns false", !ok && kb == 0);
}

static void testProcStatusEmpty()
{
    unsigned long long kb = 99;
    bool ok = ProcParsers::parseProcStatusVmRssKb("", kb);
    report("parseProcStatusVmRssKb rejects empty", !ok && kb == 0);
}

void systemMetricsParseTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  SystemMetrics Parsers : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    g_failures = 0;

    testProcStatWellFormed();
    testProcStatNoIowait();
    testProcStatEmpty();
    testProcStatWrongLabel();

    testProcPidStatSimpleName();
    testProcPidStatNameWithSpace();
    testProcPidStatNameWithParens();
    testProcPidStatNoRparen();
    testProcPidStatEmpty();
    testProcPidStatTruncated();

    testProcStatusVmRssPresent();
    testProcStatusVmRssAbsent();
    testProcStatusEmpty();

    std::cout << '\n';
    std::cout << "  Failures: " << g_failures << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  SystemMetrics Parsers tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
