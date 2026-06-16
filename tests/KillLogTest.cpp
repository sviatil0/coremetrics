#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#include "KillLogTest.hpp"
#include "KillLog.hpp"

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

    // Resolve a unique-per-pid temp directory we own end-to-end. Each
    // KillLog test deletes-then-recreates this dir to guarantee a clean
    // slate even if a prior crash left a stale log behind. We deliberately
    // do not call std::tmpnam (deprecated, security-warning on glibc and
    // MSVC) and instead splice the current pid into a constant name so
    // the path is reproducible from the test output if anything fails.
    std::filesystem::path uniqueTempDir()
    {
        std::error_code ec;
        std::filesystem::path base = std::filesystem::temp_directory_path(ec);
        if (ec || base.empty())
        {
            // Fallback for sandboxed CI where temp_directory_path can
            // fail: drop into the CWD so the suite still has somewhere
            // safe to write.
            base = std::filesystem::path(".");
        }
#ifdef _WIN32
        int pid = static_cast<int>(::_getpid());
#else
        int pid = static_cast<int>(::getpid());
#endif
        std::string dirName = "coremetrics-killlog-" + std::to_string(pid);
        return base / dirName;
    }

    // Drop any pre-existing temp dir and report whether the cleanup
    // succeeded. Used both as setup (before every test) and as teardown
    // (after the suite finishes) so a developer running the suite twice
    // in a row never reads a stale line from a prior run.
    void resetTempDir(const std::filesystem::path &dir)
    {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        // We tolerate "no such directory" — the goal is "dir is gone",
        // not "we deleted something". Any other error surfaces as a
        // test failure on the next filesystem::exists() check.
    }

    // Read every line of the produced kill.log into a vector so per-line
    // assertions stay readable. Returns an empty vector when the log file
    // does not exist; callers distinguish that from "0 lines written" by
    // checking std::filesystem::exists() separately.
    std::vector<std::string> readLines(const std::filesystem::path &logPath)
    {
        std::vector<std::string> lines;
        std::ifstream in(logPath);
        if (!in.is_open())
        {
            return lines;
        }
        std::string line;
        while (std::getline(in, line))
        {
            lines.push_back(line);
        }
        return lines;
    }

    // True when `s` is exactly ISO 8601 "YYYY-MM-DDTHH:MM:SSZ" (20 chars,
    // digits where digits belong, fixed separators, trailing 'Z'). Used to
    // pin the on-disk timestamp shape without coupling the test to the
    // current wall-clock value (which would be flaky).
    bool isIsoTimestamp(const std::string &s)
    {
        if (s.size() != 20)
        {
            return false;
        }
        for (std::size_t i = 0; i < s.size(); ++i)
        {
            char c = s[i];
            if (i == 4 || i == 7)
            {
                if (c != '-')
                {
                    return false;
                }
            }
            else if (i == 10)
            {
                if (c != 'T')
                {
                    return false;
                }
            }
            else if (i == 13 || i == 16)
            {
                if (c != ':')
                {
                    return false;
                }
            }
            else if (i == 19)
            {
                if (c != 'Z')
                {
                    return false;
                }
            }
            else
            {
                if (c < '0' || c > '9')
                {
                    return false;
                }
            }
        }
        return true;
    }

    // Split a single audit line on spaces. The format is
    //   "<iso-ts> <signal> <pid> <name>"
    // and the name itself never contains a space (sanitizeProcessName
    // converts tabs/newlines but a space-bearing name would split into
    // extra fields here). We return all fields and let each test assert
    // on the size + individual fields.
    std::vector<std::string> splitFields(const std::string &line)
    {
        std::vector<std::string> fields;
        std::istringstream iss(line);
        std::string tok;
        while (iss >> tok)
        {
            fields.push_back(tok);
        }
        return fields;
    }
}

static void testAppendCreatesDirectory()
{
    // Pin the contract: when the target directory does not exist yet,
    // appendToDir must mkdir -p it before writing. This is the path the
    // first-ever kill on a fresh user install hits.
    std::filesystem::path dir = uniqueTempDir();
    resetTempDir(dir);

    bool dirGoneBefore = !std::filesystem::exists(dir);
    KillLog::appendToDir(dir, 12345, "firefox", "TERM");
    bool dirExistsAfter = std::filesystem::is_directory(dir);
    bool logExistsAfter = std::filesystem::exists(dir / "kill.log");

    report("appendToDir starts with no directory", dirGoneBefore);
    report("appendToDir creates the parent directory", dirExistsAfter);
    report("appendToDir writes the kill.log file", logExistsAfter);

    resetTempDir(dir);
}

static void testAppendProducesSingleLine()
{
    // One append -> exactly one newline-terminated line with four
    // space-separated fields. Guards against accidental extra writes
    // (e.g. a flush that emits a header).
    std::filesystem::path dir = uniqueTempDir();
    resetTempDir(dir);

    KillLog::appendToDir(dir, 4242, "bash", "TERM");
    std::vector<std::string> lines = readLines(dir / "kill.log");

    report("single append produces exactly one line", lines.size() == 1);
    if (lines.size() == 1)
    {
        std::vector<std::string> fields = splitFields(lines[0]);
        report("single append line has 4 space-separated fields", fields.size() == 4);
    }

    resetTempDir(dir);
}

static void testMultiAppendProducesMultipleLines()
{
    // Three appends -> three lines, in call order. Pins the append-only
    // contract: every kill event is its own line, none are coalesced.
    std::filesystem::path dir = uniqueTempDir();
    resetTempDir(dir);

    KillLog::appendToDir(dir, 1, "alpha", "TERM");
    KillLog::appendToDir(dir, 2, "beta", "KILL");
    KillLog::appendToDir(dir, 3, "gamma", "HUP");

    std::vector<std::string> lines = readLines(dir / "kill.log");
    report("three appends produce three lines", lines.size() == 3);

    if (lines.size() == 3)
    {
        std::vector<std::string> first = splitFields(lines[0]);
        std::vector<std::string> second = splitFields(lines[1]);
        std::vector<std::string> third = splitFields(lines[2]);
        bool sizesOk = first.size() == 4 && second.size() == 4 && third.size() == 4;
        report("each multi-append line has 4 fields", sizesOk);
        if (sizesOk)
        {
            report("line 1 carries pid 1 and signal TERM",
                   first[1] == "TERM" && first[2] == "1" && first[3] == "alpha");
            report("line 2 carries pid 2 and signal KILL",
                   second[1] == "KILL" && second[2] == "2" && second[3] == "beta");
            report("line 3 carries pid 3 and signal HUP",
                   third[1] == "HUP" && third[2] == "3" && third[3] == "gamma");
        }
    }

    resetTempDir(dir);
}

static void testIso8601TimestampFormat()
{
    // Pin the leading timestamp shape without coupling to a specific
    // wall-clock value (which would be flaky across CI clocks). We only
    // assert "looks like ISO 8601 UTC with Z suffix".
    std::filesystem::path dir = uniqueTempDir();
    resetTempDir(dir);

    KillLog::appendToDir(dir, 777, "node", "TERM");
    std::vector<std::string> lines = readLines(dir / "kill.log");

    bool haveOne = lines.size() == 1;
    report("iso timestamp test produced one line", haveOne);

    if (haveOne)
    {
        std::vector<std::string> fields = splitFields(lines[0]);
        bool fieldOk = fields.size() == 4;
        report("iso timestamp line splits into 4 fields", fieldOk);
        if (fieldOk)
        {
            report("field 1 is ISO 8601 UTC (YYYY-MM-DDTHH:MM:SSZ)",
                   isIsoTimestamp(fields[0]));
        }
    }

    resetTempDir(dir);
}

static void testEmptyNameFlattensToDash()
{
    // Production contract: an empty process name column becomes '-' so
    // the field is never missing and `awk '{print $4}'` keeps working.
    std::filesystem::path dir = uniqueTempDir();
    resetTempDir(dir);

    KillLog::appendToDir(dir, 99, std::string{}, "TERM");
    std::vector<std::string> lines = readLines(dir / "kill.log");

    bool haveOne = lines.size() == 1;
    report("empty-name test produced one line", haveOne);

    if (haveOne)
    {
        std::vector<std::string> fields = splitFields(lines[0]);
        bool fieldOk = fields.size() == 4;
        report("empty-name line splits into 4 fields (name -> '-')", fieldOk);
        if (fieldOk)
        {
            report("empty name field is rendered as '-'", fields[3] == "-");
            report("empty-name line still pins pid 99", fields[2] == "99");
            report("empty-name line still pins signal TERM", fields[1] == "TERM");
        }
    }

    resetTempDir(dir);
}

static void testEmptySignalFlattensToDash()
{
    // Same '-' contract on the signal column. Production never calls with
    // an empty signal but the helper still has to keep the field present.
    std::filesystem::path dir = uniqueTempDir();
    resetTempDir(dir);

    KillLog::appendToDir(dir, 55, "redis", std::string{});
    std::vector<std::string> lines = readLines(dir / "kill.log");

    bool haveOne = lines.size() == 1;
    report("empty-signal test produced one line", haveOne);

    if (haveOne)
    {
        std::vector<std::string> fields = splitFields(lines[0]);
        bool fieldOk = fields.size() == 4;
        report("empty-signal line splits into 4 fields", fieldOk);
        if (fieldOk)
        {
            report("empty signal field is rendered as '-'", fields[1] == "-");
            report("empty-signal line still pins pid 55", fields[2] == "55");
            report("empty-signal line still pins name redis", fields[3] == "redis");
        }
    }

    resetTempDir(dir);
}

static void testUppercaseSignalNameRoundTrip()
{
    // SignalUtils::name() returns "TERM" / "KILL" / "HUP" / "INT" — all
    // uppercase. Pin that the audit line preserves the uppercase exactly,
    // so a future case-fold refactor of name() would not silently demote
    // the audit field to lowercase.
    std::filesystem::path dir = uniqueTempDir();
    resetTempDir(dir);

    KillLog::appendToDir(dir, 11, "p1", "TERM");
    KillLog::appendToDir(dir, 22, "p2", "KILL");
    KillLog::appendToDir(dir, 33, "p3", "INT");
    KillLog::appendToDir(dir, 44, "p4", "HUP");

    std::vector<std::string> lines = readLines(dir / "kill.log");
    bool haveFour = lines.size() == 4;
    report("uppercase-signal test produced four lines", haveFour);

    if (haveFour)
    {
        std::vector<std::string> a = splitFields(lines[0]);
        std::vector<std::string> b = splitFields(lines[1]);
        std::vector<std::string> c = splitFields(lines[2]);
        std::vector<std::string> d = splitFields(lines[3]);
        bool sizesOk = a.size() == 4 && b.size() == 4 && c.size() == 4 && d.size() == 4;
        report("uppercase-signal lines all split into 4 fields", sizesOk);
        if (sizesOk)
        {
            report("signal field for TERM is 'TERM' (uppercase preserved)",
                   a[1] == "TERM");
            report("signal field for KILL is 'KILL' (uppercase preserved)",
                   b[1] == "KILL");
            report("signal field for INT is 'INT' (uppercase preserved)",
                   c[1] == "INT");
            report("signal field for HUP is 'HUP' (uppercase preserved)",
                   d[1] == "HUP");
        }
    }

    resetTempDir(dir);
}

static void testTreeIndentStrippedFromName()
{
    // When the user invokes the kill from the Processes-tab tree view,
    // the name cell carries decoration like "  |- firefox". The audit
    // line must record the bare executable name. Pins that contract on
    // the round-trip path.
    std::filesystem::path dir = uniqueTempDir();
    resetTempDir(dir);

    KillLog::appendToDir(dir, 7, "  |- firefox", "TERM");
    std::vector<std::string> lines = readLines(dir / "kill.log");

    bool haveOne = lines.size() == 1;
    report("tree-indent test produced one line", haveOne);

    if (haveOne)
    {
        std::vector<std::string> fields = splitFields(lines[0]);
        bool fieldOk = fields.size() == 4;
        report("tree-indent line splits into 4 fields", fieldOk);
        if (fieldOk)
        {
            report("tree-indent decoration is stripped from name",
                   fields[3] == "firefox");
        }
    }

    resetTempDir(dir);
}

void killLogTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  KillLog : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    g_failures = 0;

    testAppendCreatesDirectory();
    testAppendProducesSingleLine();
    testMultiAppendProducesMultipleLines();
    testIso8601TimestampFormat();
    testEmptyNameFlattensToDash();
    testEmptySignalFlattensToDash();
    testUppercaseSignalNameRoundTrip();
    testTreeIndentStrippedFromName();

    // Final teardown: make sure no temp dir lingers after the suite.
    std::filesystem::path dir = uniqueTempDir();
    resetTempDir(dir);

    std::cout << '\n';
    std::cout << "  Failures: " << g_failures << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  KillLog tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
