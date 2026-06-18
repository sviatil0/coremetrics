#include "ArgParse.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

#include "ProcessUtils.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 17 of the
// modernization roadmap. The three sequential argv scans + the
// --help / --version printer used to live inline at the top of
// main(); the extraction moves them into a single namespace
// function that returns a typed Result. The parser is the cleanest
// extraction in the roadmap so far: no externs, no globals, no
// side effects beyond printing the help and version banners.
//
// Numeric clamps + the --top-sort / --top-color decoding match
// the previous in-place behavior bit-for-bit so smoke tests do
// not regress. clampPollIntervalMs() is the same helper the old
// in-place loop called, so the [100, 10000] poll window and the
// fallback-to-default semantics on garbage input are unchanged.
namespace ArgParse
{
    Result parse(int argc, char **argv)
    {
        Result result;

        // --help / -h print the same flag reference the manpage carries.
        // --version / -V print a one-line semver string. Both exit 0
        // before any other state is touched.
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--version" || arg == "-V")
            {
                // Mirrors base.xml's footer label; bumped by the release
                // flow that touches the XML.
                std::printf("coremetrics 0.3.0\n");
                result.exitCode = 0;
                return result;
            }
            if (arg == "--help" || arg == "-h")
            {
                std::printf(
                    "Usage: coremetrics [options]\n"
                    "\n"
                    "Live UI flags:\n"
                    "  --sparklines              show CPU/RAM/GPU/NET history sparklines\n"
                    "  --tree                    open Processes tab in parent/child tree mode\n"
                    "  --filter PATTERN          seed the Processes-tab name filter\n"
                    "  --poll-ms N               refresh cadence in ms (clamped 100..10000)\n"
                    "  --duration SECONDS        auto-exit after N seconds (live UI)\n"
                    "\n"
                    "Headless modes:\n"
                    "  --screenshot PATH [system|processes|about]\n"
                    "                            render one frame to PATH and exit\n"
                    "  --export-csv PATH         dump one-shot CSV and exit\n"
                    "  --export-json PATH        dump one-shot JSON and exit\n"
                    "  --top N                   print top-N processes to stdout and exit\n"
                    "  --watch                   used with --top: re-print every poll interval\n"
                    "  --top-sort cpu|mem|io     re-order --top output (default mem)\n"
                    "  --top-color auto|always|never\n"
                    "                            colorize --top output (default auto via isatty)\n"
                    "\n"
                    "Other:\n"
                    "  -h, --help                print this help and exit\n"
                    "  -V, --version             print version and exit\n"
                    "\n"
                    "See coremetrics(1) for the full manpage and key bindings.\n"
                );
                result.exitCode = 0;
                return result;
            }
        }

        for (int i = 1; i < argc; ++i)
        {
            if (std::string(argv[i]) == "--screenshot" && i + 1 < argc)
            {
                result.screenshotPath = argv[i + 1];
            }
            if (std::string(argv[i]) == "--duration" && i + 1 < argc)
            {
                result.durationSeconds = std::atof(argv[i + 1]);
                if (result.durationSeconds < 0.0)
                {
                    result.durationSeconds = 0.0;
                }
            }
            if (std::string(argv[i]) == "--sparklines")
            {
                result.sparklinesEnabled = true;
            }
            // --tree opens the Processes tab in parent/child indented hierarchy
            // mode, same as pressing 't' on that tab interactively. Lets the
            // screenshot pipeline capture tree mode without simulating keystrokes.
            if (std::string(argv[i]) == "--tree")
            {
                result.treeMode = true;
            }
            // --filter <substring> seeds the Processes-tab name filter so the
            // screenshot pipeline can demonstrate search. Case-insensitive
            // substring match against process names, applied during pollMetrics.
            if (std::string(argv[i]) == "--filter" && i + 1 < argc)
            {
                result.seedFilter = argv[i + 1];
            }
            // --poll-ms <N> overrides the default 500ms refresh cadence.
            // Validation + clamp delegated to ProcessUtils so the same
            // logic gets unit coverage and negative / non-numeric inputs
            // fall back instead of wrapping. The fallback here is 0 so
            // garbage input collapses to "no CLI override" and the
            // saved-setting value Settings::load seeded survives. main()
            // gates the g_pollIntervalMs write on pollIntervalMs > 0 for
            // the same reason.
            if (std::string(argv[i]) == "--poll-ms" && i + 1 < argc)
            {
                result.pollIntervalMs = clampPollIntervalMs(argv[i + 1], 0);
            }
            // --export-csv <path> / --export-json <path>: one-shot dump of the
            // live aggregates + top-N process list, written to `path` and then
            // the process exits 0. Lets external tools consume CoreMetrics data
            // without scraping screenshots.
            if (std::string(argv[i]) == "--export-csv" && i + 1 < argc)
            {
                result.exportCsvPath = argv[i + 1];
            }
            if (std::string(argv[i]) == "--export-json" && i + 1 < argc)
            {
                result.exportJsonPath = argv[i + 1];
            }
            if (std::string(argv[i]) == "--top")
            {
                int parsed = 20;
                if (i + 1 < argc)
                {
                    parsed = std::atoi(argv[i + 1]);
                    if (parsed < 1)
                    {
                        parsed = 20;
                    }
                    if (parsed > 999)
                    {
                        parsed = 999;
                    }
                }
                result.topCount = parsed;
            }
            if (std::string(argv[i]) == "--watch")
            {
                result.watchMode = true;
            }
            if (std::string(argv[i]) == "--top-sort" && i + 1 < argc)
            {
                std::string key = argv[i + 1];
                if (key == "cpu")
                {
                    result.topSortKey = TopSortKey::Cpu;
                }
                else if (key == "io")
                {
                    result.topSortKey = TopSortKey::Io;
                }
                else
                {
                    result.topSortKey = TopSortKey::Mem;
                }
            }
            if (std::string(argv[i]) == "--top-color" && i + 1 < argc)
            {
                std::string mode = argv[i + 1];
                if (mode == "always")
                {
                    result.topColorize = 2;
                }
                else if (mode == "never")
                {
                    result.topColorize = 0;
                }
                else
                {
                    result.topColorize = 1;
                }
            }
        }

        // The screenshot positional arg lives in its own pass because
        // the loop above only resolves --screenshot's first arg (the
        // output path); the optional second positional selects which
        // tab to render. Mirrors the old in-place loop bound (i + 2
        // < argc) so a trailing --screenshot path without a tab keeps
        // the default "system" behavior.
        if (!result.screenshotPath.empty())
        {
            for (int i = 1; i < argc - 1; ++i)
            {
                if (std::string(argv[i]) == "--screenshot" && i + 2 < argc)
                {
                    result.screenshotTab = argv[i + 2];
                }
            }
        }

        return result;
    }
}
