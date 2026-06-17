#ifndef __TOPPROCESSESPRINTER_HPP__
#define __TOPPROCESSESPRINTER_HPP__

// Pretty-prints the top-N process table to stdout for the headless
// `--top` and `--watch` CLI modes. Both the one-shot path and the
// watch loop in coremetrics.cpp call into the same formatter so the
// output stays byte-identical across runs.
//
// Phase 1.2 slice 10 of the modernization roadmap. Extracted from
// coremetrics.cpp; the enum and helpers were previously file-local to
// the same TU. The enum now lives in this header so the CLI parser
// and the printer share the exact variant set, and the comparators +
// ANSI color picker are buried in the .cpp's anonymous namespace
// where no other TU can reach them.

// `--top-sort cpu|mem|io` maps to one of these. The default in main()
// is Mem, which mirrors the natural ordering of
// SystemMetrics::topProcesses(N); the Cpu and Io variants trigger a
// secondary std::sort inside print() over an over-fetched buffer so
// the secondary column has enough candidates to choose from.
enum class TopSortKey
{
    Cpu,
    Mem,
    Io
};

namespace TopProcessesPrinter
{
    // topCount is clamped by the caller to [1, 999]. sortKey picks the
    // re-sort column applied after SystemMetrics::topProcesses() over-
    // fetches by mem%. colorize toggles ANSI 24-bit threshold coloring;
    // resolved from --top-color auto|always|never + isatty(stdout) up
    // in main() so this helper stays terminal-agnostic.
    void print(int topCount, TopSortKey sortKey, bool colorize);
}

#endif
