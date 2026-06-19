#ifndef __ARGPARSE_HPP__
#define __ARGPARSE_HPP__

#include <string>

#include "TopProcessesPrinter.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 17 of the
// modernization roadmap. After the HeadlessRunner extraction (slice
// 16) the first ~200 lines of main() were three sequential argv
// scans that populated a fan-out of local variables. Those scans
// share no state with the live UI loop or the headless dispatch
// path; they just translate argv into a typed Result struct that
// the rest of main() consumes. Moving them here keeps main() free
// to focus on dispatch and reduces coremetrics.cpp by ~150 lines
// net.
//
// The parser is intentionally pure-data: it owns no globals, reads
// no externs, and produces no side effects beyond printing the
// --help / --version banner. The caller writes the parsed values
// into the live-UI globals after Settings::load so the CLI override
// beats the persisted setting, exactly the way the in-place loops
// used to.
namespace ArgParse
{
    struct Result
    {
        // Caller exit-code contract. -1 means "continue main", 0
        // means "help or version was printed, exit cleanly", any
        // other value would be an error. The current parser only
        // ever produces -1 or 0; the field is kept signed-int so
        // future error paths can drop in without changing the type.
        int exitCode = -1;
        bool sparklinesEnabled = false;
        bool treeMode = false;
        std::string seedFilter;
        std::string screenshotPath;
        // "system" / "processes" / "about". Empty means main()
        // should fall back to "system" the way the screenshot
        // runner already does.
        std::string screenshotTab;
        // --debug-layout PATH dumps the captured paint rects (text +
        // boxes) for the requested tab as JSON. Consumed by the Python
        // layout-audit script in CI to detect text-vs-text overlaps,
        // out-of-bounds rects, and other visual regressions without
        // depending on OCR. Empty means the flag was not passed.
        std::string debugLayoutPath;
        std::string exportCsvPath;
        std::string exportJsonPath;
        int topCount = 0;
        bool watchMode = false;
        TopSortKey topSortKey = TopSortKey::Mem;
        // 0 = none (force off), 1 = auto (resolve via isatty in
        // main), 2 = always (force on). Default is auto so the
        // pre-CLI behavior of probing stdout for a TTY is the
        // unchanged baseline; the caller maps these three states
        // onto the bool the printer wants.
        int topColorize = 1;
        double durationSeconds = 0.0;
        // 0 means "no CLI override"; the caller leaves
        // g_pollIntervalMs at whatever Settings::load seeded.
        unsigned long long pollIntervalMs = 0;
    };

    Result parse(int argc, char **argv);
}

#endif
