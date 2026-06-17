#include "TopProcessesPrinter.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "SystemMetrics.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 10 of the
// modernization roadmap. Before extraction the printer plus its three
// helpers (the two comparators and the ANSI color picker) accounted
// for ~70 lines of file-local state at the bottom of coremetrics.cpp;
// folding them into a dedicated TU lets the entry-point file stay
// focused on argv parsing, SDL_Init, and the main loop.
//
// Behavior is preserved verbatim: same column widths, same sort
// semantics (over-fetch by 4x then re-sort), same ANSI 24-bit escape
// codes keyed to the 60% / 80% thresholds used elsewhere in the GUI
// (see Thresholds.hpp). The one-shot --top path and the --watch tail
// both call into print() so the output stays byte-identical across
// modes.

namespace TopProcessesPrinter
{
    namespace
    {
        // topProcesses() returns by mem% desc; we re-sort here for the
        // Cpu / Io variants. Over-fetch by 4x so the secondary sort has
        // a wide enough candidate pool even when the user asked for a
        // small N. Floor of 50 keeps tiny --top 5 calls from missing
        // high-cpu processes that ranked low on the mem-sorted page.
        constexpr int kOverFetchMultiplier = 4;
        constexpr std::size_t kMinFetchCount = 50;

        // Column widths. PID gets 7 to handle the 32-bit max linux
        // pid_t (7 digits) with one trailing space; NAME 32 matches
        // the in-GUI process row name cap so the CLI output mirrors
        // what the user sees on screen. CPU% / MEM% are %6.1f
        // (XXX.X). IO is %12llu so even multi-GB/s bursts fit
        // without realignment.
        constexpr int kNameCap = 32;

        // ANSI 24-bit threshold colors. The 60% / 80% breakpoints are
        // the same ones Thresholds.hpp uses for the live UI cells.
        constexpr float kColorYellowPct = 60.0f;
        constexpr float kColorRedPct = 80.0f;

        bool sortByCpuDesc(const ProcessInfo &a, const ProcessInfo &b)
        {
            return a.cpuPct > b.cpuPct;
        }

        bool sortByIoDesc(const ProcessInfo &a, const ProcessInfo &b)
        {
            unsigned long long aIo = a.diskReadKbPerSec + a.diskWriteKbPerSec;
            unsigned long long bIo = b.diskReadKbPerSec + b.diskWriteKbPerSec;
            return aIo > bIo;
        }

        // Pick an ANSI 24-bit color escape for a percent value using
        // the same green/yellow/red threshold palette the GUI uses
        // (see Thresholds.hpp). Returns "" when colorization is off so
        // the caller can splice it in unconditionally without
        // surrounding if/else noise.
        const char *colorForPct(float pct, bool colorize)
        {
            if (!colorize)
            {
                return "";
            }
            if (pct >= kColorRedPct)
            {
                return "\033[31m"; // red
            }
            if (pct >= kColorYellowPct)
            {
                return "\033[33m"; // yellow
            }
            return "\033[32m"; // green
        }
    }

    void print(int topCount, TopSortKey sortKey, bool colorize)
    {
        std::size_t fetchN = static_cast<std::size_t>(topCount)
                             * static_cast<std::size_t>(kOverFetchMultiplier);
        if (fetchN < kMinFetchCount)
        {
            fetchN = kMinFetchCount;
        }
        std::vector<ProcessInfo> procs = SystemMetrics::topProcesses(fetchN);
        if (sortKey == TopSortKey::Cpu)
        {
            std::sort(procs.begin(), procs.end(), sortByCpuDesc);
        }
        else if (sortKey == TopSortKey::Io)
        {
            std::sort(procs.begin(), procs.end(), sortByIoDesc);
        }
        if (procs.size() > static_cast<std::size_t>(topCount))
        {
            procs.resize(static_cast<std::size_t>(topCount));
        }
        const char *reset = colorize ? "\033[0m" : "";
        const char *headerColor = colorize ? "\033[1m" : "";
        std::printf("%s%-7s %-32s %6s %6s %12s%s\n",
                    headerColor, "PID", "NAME", "CPU%", "MEM%", "IO KB/s", reset);
        for (const auto &p : procs)
        {
            std::string name = p.name;
            if (name.size() > static_cast<std::size_t>(kNameCap))
            {
                name.resize(static_cast<std::size_t>(kNameCap));
            }
            unsigned long long ioKbPerSec = p.diskReadKbPerSec + p.diskWriteKbPerSec;
            const char *cpuC = colorForPct(p.cpuPct, colorize);
            const char *memC = colorForPct(p.memPct, colorize);
            std::printf("%-7d %-32s %s%6.1f%s %s%6.1f%s %12llu\n",
                        p.pid, name.c_str(),
                        cpuC, p.cpuPct, reset,
                        memC, p.memPct, reset,
                        ioKbPerSec);
        }
        std::fflush(stdout);
    }
}
