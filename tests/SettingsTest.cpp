#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <system_error>
#include <unordered_set>
#include "SettingsTest.hpp"
#include "Settings.hpp"

// Round-trip tests for the Settings persistence facility. The public API
// resolves its on-disk location from HOME / APPDATA, so each test points
// those env vars at a freshly-allocated subdirectory under the system's
// temp directory and tears it down on the way out. That keeps the tests
// hermetic without requiring a path-taking overload on the public API.

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

    // Generate a unique scratch directory under the OS temp dir. The
    // suffix is a random 64-bit value so concurrent test binaries on the
    // same machine cannot collide. The directory is created lazily by
    // the caller via std::filesystem::create_directories.
    std::filesystem::path makeScratchHome(const char *label)
    {
        std::random_device rd;
        std::uint64_t mixed = (static_cast<std::uint64_t>(rd()) << 32)
                              | static_cast<std::uint64_t>(rd());
        std::filesystem::path base = std::filesystem::temp_directory_path();
        std::string name = "coremetrics_settings_test_";
        name += label;
        name += '_';
        name += std::to_string(mixed);
        return base / name;
    }

    // Set HOME (POSIX) and APPDATA (Windows) to the given path so a
    // subsequent Settings::load/save call resolves its file under it.
    // Both vars are set unconditionally so the test does not depend on
    // which one the build target uses.
    void redirectConfigHome(const std::filesystem::path &home)
    {
        std::string homeStr = home.string();
#ifdef _WIN32
        _putenv_s("APPDATA", homeStr.c_str());
        _putenv_s("HOME", homeStr.c_str());
#else
        setenv("HOME", homeStr.c_str(), 1);
        setenv("APPDATA", homeStr.c_str(), 1);
#endif
    }

    // Where Settings.cpp will actually write the file given the redirected
    // HOME. Mirrors the layout documented in include/Settings.hpp so a
    // change to that layout fails the tests loudly.
    std::filesystem::path configFileFor(const std::filesystem::path &home)
    {
#ifdef _WIN32
        return home / "coremetrics" / "config";
#else
        return home / ".config" / "coremetrics" / "config";
#endif
    }

    // Best-effort scratch dir cleanup so a long test run does not leave
    // hundreds of orphan directories under /tmp.
    void removeScratch(const std::filesystem::path &home)
    {
        std::error_code ec;
        std::filesystem::remove_all(home, ec);
    }
}

// Missing file means load() must return false and leave every output
// untouched. This is the contract that lets compiled defaults and CLI
// flags remain authoritative on a fresh install.
static void testLoadMissingFilePreservesDefaults()
{
    std::filesystem::path home = makeScratchHome("missing");
    redirectConfigHome(home);

    std::uint64_t pollMs = 750;
    bool sparklines = true;
    int sortColumn = 2;
    bool sortAscending = false;
    std::unordered_set<int> collapsed = { 11, 22 };

    bool result = Settings::load(pollMs, sparklines, sortColumn,
                                 sortAscending, collapsed);

    bool passed = !result
                  && pollMs == 750
                  && sparklines == true
                  && sortColumn == 2
                  && sortAscending == false
                  && collapsed.size() == 2
                  && collapsed.count(11) == 1
                  && collapsed.count(22) == 1;
    report("load on missing file returns false and preserves defaults",
           passed);

    removeScratch(home);
}

// Save then load with no collapsed pids must restore every scalar field
// exactly. The save path omits the collapsed_pids line entirely when the
// set is empty, so this is also coverage for that branch.
static void testRoundTripEmptyCollapsed()
{
    std::filesystem::path home = makeScratchHome("rt_empty");
    redirectConfigHome(home);

    std::unordered_set<int> emptySet;
    bool wrote = Settings::save(1500, false, 3, true, emptySet);

    std::uint64_t pollMs = 100;
    bool sparklines = true;
    int sortColumn = 0;
    bool sortAscending = false;
    std::unordered_set<int> collapsed;
    bool read = Settings::load(pollMs, sparklines, sortColumn,
                               sortAscending, collapsed);

    bool passed = wrote
                  && read
                  && pollMs == 1500
                  && sparklines == false
                  && sortColumn == 3
                  && sortAscending == true
                  && collapsed.empty();
    report("save then load round-trips scalars with empty collapsed set",
           passed);

    removeScratch(home);
}

// Save then load with a non-empty collapsed-pid set must restore every
// pid. The writer sorts the pids before emitting them so the on-disk
// format is stable; the loader builds an unordered_set so iteration
// order is irrelevant - only set equality matters.
static void testRoundTripCollapsedPids()
{
    std::filesystem::path home = makeScratchHome("rt_pids");
    redirectConfigHome(home);

    std::unordered_set<int> collapsedIn = { 1, 42, 7, 9999, 123456 };
    bool wrote = Settings::save(500, true, 2, false, collapsedIn);

    std::uint64_t pollMs = 100;
    bool sparklines = false;
    int sortColumn = 0;
    bool sortAscending = true;
    std::unordered_set<int> collapsedOut;
    bool read = Settings::load(pollMs, sparklines, sortColumn,
                               sortAscending, collapsedOut);

    bool passed = wrote
                  && read
                  && pollMs == 500
                  && sparklines == true
                  && sortColumn == 2
                  && sortAscending == false
                  && collapsedOut == collapsedIn;
    report("save then load round-trips collapsed pid set exactly", passed);

    removeScratch(home);
}

// Hand-rolled file with one valid record per known key plus a malformed
// line in the middle. The malformed line must be skipped without
// blocking later valid records, and the valid keys must all populate.
static void testMalformedLinesAreSkipped()
{
    std::filesystem::path home = makeScratchHome("malformed");
    redirectConfigHome(home);

    std::filesystem::path cfg = configFileFor(home);
    std::error_code ec;
    std::filesystem::create_directories(cfg.parent_path(), ec);

    std::ofstream out(cfg);
    out << "# header comment\n";
    out << "poll_ms=1200\n";
    out << "this line has no equals sign\n";
    out << "sparklines_enabled=true\n";
    out << "=value-with-empty-key\n";
    out << "key-with-empty-value=\n";
    out << "sort_column=4\n";
    out << "sort_ascending=false\n";
    out << "poll_ms=not-a-number\n";
    out << "unknown_key=ignored\n";
    out.close();

    std::uint64_t pollMs = 250;
    bool sparklines = false;
    int sortColumn = 0;
    bool sortAscending = true;
    std::unordered_set<int> collapsed;
    bool read = Settings::load(pollMs, sparklines, sortColumn,
                               sortAscending, collapsed);

    bool passed = read
                  && pollMs == 1200
                  && sparklines == true
                  && sortColumn == 4
                  && sortAscending == false
                  && collapsed.empty();
    report("load skips malformed lines and applies valid keys", passed);

    removeScratch(home);
}

// Direct test of the collapsed_pids comma-list parser path. A hand-
// authored file with the comma-list line must populate the set with
// every listed pid. This covers the persistence wire format independently
// of any save() output so a change to the writer cannot silently
// invalidate the reader.
static void testCollapsedPidsCommaListParses()
{
    std::filesystem::path home = makeScratchHome("comma_list");
    redirectConfigHome(home);

    std::filesystem::path cfg = configFileFor(home);
    std::error_code ec;
    std::filesystem::create_directories(cfg.parent_path(), ec);

    std::ofstream out(cfg);
    out << "collapsed_pids=10,20,30,40,50\n";
    out.close();

    std::uint64_t pollMs = 250;
    bool sparklines = false;
    int sortColumn = 0;
    bool sortAscending = true;
    std::unordered_set<int> collapsed;
    bool read = Settings::load(pollMs, sparklines, sortColumn,
                               sortAscending, collapsed);

    std::unordered_set<int> expected = { 10, 20, 30, 40, 50 };
    bool passed = read && collapsed == expected;
    report("collapsed_pids comma list parses every pid", passed);

    removeScratch(home);
}

// Mixed-validity comma list: empty tokens, whitespace, and a
// non-numeric entry must be skipped while every valid pid is still
// inserted. This is the worst-case happy path for the parser.
static void testCollapsedPidsCommaListSkipsInvalidTokens()
{
    std::filesystem::path home = makeScratchHome("comma_mixed");
    redirectConfigHome(home);

    std::filesystem::path cfg = configFileFor(home);
    std::error_code ec;
    std::filesystem::create_directories(cfg.parent_path(), ec);

    std::ofstream out(cfg);
    out << "collapsed_pids= 1 , ,3,abc,5,,7\n";
    out.close();

    std::uint64_t pollMs = 250;
    bool sparklines = false;
    int sortColumn = 0;
    bool sortAscending = true;
    std::unordered_set<int> collapsed;
    bool read = Settings::load(pollMs, sparklines, sortColumn,
                               sortAscending, collapsed);

    std::unordered_set<int> expected = { 1, 3, 5, 7 };
    bool passed = read && collapsed == expected;
    report("collapsed_pids comma list skips empty and non-numeric tokens",
           passed);

    removeScratch(home);
}

// Out-of-range scalars must be clamped (poll_ms) or rejected
// (sort_column) so a hand-edited file cannot push the live UI past the
// limits the CLI parser enforces.
static void testOutOfRangeScalarsAreClampedOrRejected()
{
    std::filesystem::path home = makeScratchHome("out_of_range");
    redirectConfigHome(home);

    std::filesystem::path cfg = configFileFor(home);
    std::error_code ec;
    std::filesystem::create_directories(cfg.parent_path(), ec);

    std::ofstream out(cfg);
    // poll_ms must clamp into [100, 10000]; sort_column outside [0, 4]
    // must be ignored so the in-memory default survives.
    out << "poll_ms=50\n";
    out << "sort_column=99\n";
    out.close();

    std::uint64_t pollMs = 250;
    bool sparklines = false;
    int sortColumn = 1;
    bool sortAscending = true;
    std::unordered_set<int> collapsed;
    bool read = Settings::load(pollMs, sparklines, sortColumn,
                               sortAscending, collapsed);

    bool passed = read
                  && pollMs == 100
                  && sortColumn == 1;
    report("out-of-range scalars are clamped or rejected", passed);

    removeScratch(home);
}

void settingsTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Settings : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    g_failures = 0;

    testLoadMissingFilePreservesDefaults();
    testRoundTripEmptyCollapsed();
    testRoundTripCollapsedPids();
    testMalformedLinesAreSkipped();
    testCollapsedPidsCommaListParses();
    testCollapsedPidsCommaListSkipsInvalidTokens();
    testOutOfRangeScalarsAreClampedOrRejected();

    std::cout << '\n';
    std::cout << "  Failures: " << g_failures << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Settings tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
