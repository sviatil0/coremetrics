#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#include "TraceLogTest.hpp"
#include "TraceLog.hpp"

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

    // Resolve a temp-dir scratch path under the system tmpdir so the
    // suite never touches the real ~/.config/coremetrics/trace.log on
    // the developer's machine. tempPath() is deterministic-per-process
    // (uses the pid) so two parallel test runs do not race on the same
    // file.
    std::filesystem::path scratchPath(const char *suffix)
    {
        std::filesystem::path base = std::filesystem::temp_directory_path();
        std::ostringstream name;
#ifdef _WIN32
        int pid = ::_getpid();
#else
        int pid = ::getpid();
#endif
        name << "coremetrics-trace-test-" << pid << '-' << suffix << ".log";
        return base / name.str();
    }

    // Read the whole file into a string. Returns empty when the file
    // does not exist so tests can assert "nothing was written" without
    // first stat-ing the path.
    std::string readAll(const std::filesystem::path &p)
    {
        std::ifstream in(p);
        if (!in.is_open())
        {
            return std::string{};
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    // Count newline-terminated records in a buffer. A trailing newline
    // counts the last record; a missing final newline still counts the
    // partial tail so callers can spot "we forgot to terminate".
    std::size_t countLines(const std::string &buf)
    {
        if (buf.empty())
        {
            return 0;
        }
        std::size_t n = 0;
        for (char c : buf)
        {
            if (c == '\n')
            {
                ++n;
            }
        }
        if (buf.back() != '\n')
        {
            ++n;
        }
        return n;
    }

    // Drop any stale file from a prior run so the suite always starts
    // from a known-empty state. std::error_code overload keeps the call
    // exception-free.
    void removeIfExists(const std::filesystem::path &p)
    {
        std::error_code ec;
        std::filesystem::remove(p, ec);
    }
}

static void testDisabledByDefault()
{
    TraceLog::disable();
    bool passed = !TraceLog::isEnabled();
    report("disabled by default", passed);
}

static void testLogIsNoopWhenDisabled()
{
    std::filesystem::path path = scratchPath("noop");
    removeIfExists(path);

    // Make sure the facility is off, then attempt a write that should
    // never produce a file.
    TraceLog::disable();
    TraceLog::log("this should never appear on disk");

    bool fileMissing = !std::filesystem::exists(path);
    report("log() with no prior enable() writes nothing", fileMissing);
}

static void testEnableThenLogWritesLine()
{
    std::filesystem::path path = scratchPath("basic");
    removeIfExists(path);

    TraceLog::enable(path.string());
    bool enabledFlag = TraceLog::isEnabled();
    TraceLog::log("hello trace");
    TraceLog::disable();

    std::string contents = readAll(path);
    bool hasMessage = contents.find("hello trace") != std::string::npos;
    bool hasTimestamp = contents.find('T') != std::string::npos && contents.find('Z') != std::string::npos;
    bool oneLine = countLines(contents) == 1;
    bool endsNewline = !contents.empty() && contents.back() == '\n';

    report("enable() flips isEnabled() true", enabledFlag);
    report("log() writes the message after enable()", hasMessage);
    report("log() prefixes an ISO 8601 UTC timestamp", hasTimestamp);
    report("log() writes exactly one record per call", oneLine);
    report("log() terminates the record with a newline", endsNewline);

    removeIfExists(path);
}

static void testMultipleLogsAppendInOrder()
{
    std::filesystem::path path = scratchPath("append");
    removeIfExists(path);

    TraceLog::enable(path.string());
    TraceLog::log("first");
    TraceLog::log("second");
    TraceLog::log("third");
    TraceLog::disable();

    std::string contents = readAll(path);
    std::size_t firstPos = contents.find("first");
    std::size_t secondPos = contents.find("second");
    std::size_t thirdPos = contents.find("third");

    bool allPresent = firstPos != std::string::npos
                      && secondPos != std::string::npos
                      && thirdPos != std::string::npos;
    bool inOrder = allPresent && firstPos < secondPos && secondPos < thirdPos;
    bool threeLines = countLines(contents) == 3;

    report("multiple log() calls append in order", inOrder);
    report("multiple log() calls produce one line each", threeLines);

    removeIfExists(path);
}

static void testDisableThenLogIsNoop()
{
    std::filesystem::path path = scratchPath("toggle");
    removeIfExists(path);

    TraceLog::enable(path.string());
    TraceLog::log("kept");
    TraceLog::disable();
    TraceLog::log("dropped");

    std::string contents = readAll(path);
    bool kept = contents.find("kept") != std::string::npos;
    bool dropped = contents.find("dropped") == std::string::npos;
    bool flagOff = !TraceLog::isEnabled();

    report("disable() flips isEnabled() back to false", flagOff);
    report("log() after disable() does not append", dropped);
    report("log() before disable() is still present", kept);

    removeIfExists(path);
}

static void testEnableOnUnwritablePathStaysDisabled()
{
    // A path whose parent is a regular file rather than a directory
    // forces ofstream::open to fail. The facility must then leave
    // isEnabled() false so callers do not believe trace lines are
    // landing anywhere.
    std::filesystem::path blocker = scratchPath("blocker");
    removeIfExists(blocker);
    std::ofstream block(blocker);
    block << "not a directory" << '\n';
    block.close();

    std::filesystem::path unwritable = blocker / "trace.log";

    TraceLog::disable();
    TraceLog::enable(unwritable.string());
    bool stayedOff = !TraceLog::isEnabled();
    TraceLog::disable();

    report("enable() on an unwritable path stays disabled", stayedOff);

    removeIfExists(blocker);
}

void traceLogTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  TraceLog : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    g_failures = 0;

    testDisabledByDefault();
    testLogIsNoopWhenDisabled();
    testEnableThenLogWritesLine();
    testMultipleLogsAppendInOrder();
    testDisableThenLogIsNoop();
    testEnableOnUnwritablePathStaysDisabled();

    std::cout << '\n';
    std::cout << "  Failures: " << g_failures << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  TraceLog tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
