#ifndef __PROCESS_UTILS_HPP__
#define __PROCESS_UTILS_HPP__

#include "SystemMetrics.hpp"
#include <cstdint>
#include <string>
#include <sstream>

enum SortColumn
{
    SORT_PID = 0,
    SORT_NAME = 1,
    SORT_CPU = 2,
    SORT_MEM = 3
};

std::string formatPct(float value);

// Pure delta-based CPU% computation, identical on every platform.
// procCurrent / procPrevious are this pid's accumulated CPU ticks (or ns)
// at the current and previous samples; denom is the system-wide or wall-clock
// elapsed in the same units between those two samples.
// Returns a value clamped to [0, 100]. Returns 0 when denom is 0 or when
// procCurrent < procPrevious (pid reused, counter reset, etc.).
float computeCpuPercentDelta(std::uint64_t procCurrent,
                             std::uint64_t procPrevious,
                             std::uint64_t denom);

// Pure comparator for the Processes table. Mirrors the in-app behavior:
// SORT_PID/CPU/MEM compare numerically; SORT_NAME compares lexicographically.
// `ascending = true` puts smaller values first; false puts larger first.
bool compareProcessByColumn(const ProcessInfo &a,
                            const ProcessInfo &b,
                            SortColumn column,
                            bool ascending);

// Pure delta-based disk-I/O rate. prev/curr are cumulative byte counters
// taken at two samples; elapsedSec is the wall-clock seconds between them.
// Returns kilobytes/second. Returns 0 when elapsedSec is <= 0 or when curr
// < prev (the only sane response to a counter that went backwards, which
// happens on pid reuse and on some counter resets).
std::uint64_t computeIoKbPerSec(std::uint64_t prev,
                                std::uint64_t curr,
                                double elapsedSec);

// Case-insensitive substring match used by the Processes-tab filter.
// An empty needle always matches (the filter is treated as disabled).
// A non-empty needle and an empty name never matches.
bool processNameMatchesFilter(const std::string &name,
                              const std::string &needle);

// Format an uptime in seconds as "Up <d>d <h>h <m>m" / "Up <h>h <m>m" /
// "Up <m>m". Seconds == 0 yields "Up --" so a failed read renders the
// known-unavailable state instead of "Up 0m".
std::string formatUptimeString(unsigned long long seconds);

#endif
