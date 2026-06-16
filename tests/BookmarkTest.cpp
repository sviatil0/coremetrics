#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <unordered_set>

#include "BookmarkTest.hpp"
#include "Bookmark.hpp"

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

    // Build a unique scratch path under the OS temp dir so the suite
    // can run from a sandboxed CI runner without depending on $HOME.
    // The high-resolution clock plus a per-call counter keeps each
    // sub-test on its own file even when the suite is re-run quickly.
    std::string scratchPath(const char *tag)
    {
        static unsigned long long counter = 0;
        ++counter;
        std::filesystem::path tmp = std::filesystem::temp_directory_path();
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        long long stamp = static_cast<long long>(now.time_since_epoch().count());
        std::string name = "coremetrics_bookmark_test_";
        name += tag;
        name += "_";
        name += std::to_string(stamp);
        name += "_";
        name += std::to_string(counter);
        return (tmp / name).string();
    }

    void removeIfExists(const std::string &path)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
}

static void testRoundTripPreservesBothFlavours()
{
    std::string path = scratchPath("roundtrip");
    removeIfExists(path);

    std::unordered_set<std::string> original;
    original.insert("firefox|1234");
    original.insert("nginx");
    original.insert("postgres|987654");

    bool saved = Bookmark::save(path, original);

    std::unordered_set<std::string> loaded;
    bool loadedOk = Bookmark::load(path, loaded);

    bool passed = saved
                  && loadedOk
                  && loaded.size() == original.size()
                  && loaded.count("firefox|1234") == 1
                  && loaded.count("nginx") == 1
                  && loaded.count("postgres|987654") == 1;
    report("round-trip preserves both name and name|pid records", passed);

    removeIfExists(path);
}

static void testMalformedLinesAreSkipped()
{
    std::string path = scratchPath("malformed");
    removeIfExists(path);

    // Hand-craft a file with a mix of good and bad records. The loader
    // must keep the two valid ones and silently drop the rest.
    {
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        out << "firefox|1234\n";        // valid exact form
        out << "\n";                     // blank line, skip
        out << "|9999\n";                // empty name, skip
        out << "nginx|\n";               // empty pid, skip
        out << "nginx|abc\n";            // non-digit pid, skip
        out << "a|b|c\n";                // extra pipe, skip
        out << "   \n";                  // whitespace only, skip
        out << "  bare-name  \n";        // trimmed wildcard, keep
    }

    std::unordered_set<std::string> loaded;
    bool ok = Bookmark::load(path, loaded);

    bool passed = ok
                  && loaded.size() == 2
                  && loaded.count("firefox|1234") == 1
                  && loaded.count("bare-name") == 1;
    report("malformed lines are silently skipped", passed);

    removeIfExists(path);
}

static void testMissingFileHandledCleanly()
{
    std::string path = scratchPath("missing");
    removeIfExists(path);

    // Seed the destination so we can prove `load` clears it even on a
    // missing-file return path.
    std::unordered_set<std::string> dest;
    dest.insert("stale|1");

    bool ok = Bookmark::load(path, dest);

    bool passed = !ok && dest.empty();
    report("missing file returns false and clears destination", passed);
}

static void testEmptySetYieldsEmptyFile()
{
    std::string path = scratchPath("empty");
    removeIfExists(path);

    std::unordered_set<std::string> empty;
    bool saved = Bookmark::save(path, empty);

    std::error_code ec;
    bool exists = std::filesystem::exists(path, ec);
    std::uintmax_t size = exists ? std::filesystem::file_size(path, ec) : 1;

    std::unordered_set<std::string> loaded;
    loaded.insert("ghost|7");
    bool loadedOk = Bookmark::load(path, loaded);

    bool passed = saved && exists && size == 0 && loadedOk && loaded.empty();
    report("empty set writes a zero-byte file that round-trips empty", passed);

    removeIfExists(path);
}

static void testDefaultPathHasExpectedShape()
{
    // We cannot pin the exact value (CI runners vary) but we can
    // sanity-check that the resolver produced a non-empty path whose
    // last component is "bookmarks" when HOME / APPDATA is set. If the
    // env var is missing the suite still passes by short-circuiting.
    std::string path = Bookmark::defaultPath();
    bool envSet = false;
#ifdef _WIN32
    const char *root = std::getenv("APPDATA");
#else
    const char *root = std::getenv("HOME");
#endif
    envSet = (root != nullptr && root[0] != '\0');

    bool passed = false;
    if (!envSet)
    {
        passed = path.empty();
    }
    else
    {
        std::filesystem::path p(path);
        passed = !path.empty()
                 && p.filename().string() == "bookmarks"
                 && p.parent_path().filename().string() == "coremetrics";
    }
    report("defaultPath resolves to <config>/coremetrics/bookmarks", passed);
}

void bookmarkTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Bookmark : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    g_failures = 0;

    testRoundTripPreservesBothFlavours();
    testMalformedLinesAreSkipped();
    testMissingFileHandledCleanly();
    testEmptySetYieldsEmptyFile();
    testDefaultPathHasExpectedShape();

    std::cout << '\n';
    std::cout << "  Failures: " << g_failures << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Bookmark tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
