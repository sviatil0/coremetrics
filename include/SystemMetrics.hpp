#ifndef __SYSTEMMETRICS_HPP__
#define __SYSTEMMETRICS_HPP__

#include <cstddef>
#include <string>
#include <vector>

struct ProcessInfo
{
    int pid;
    int parentPid;
    std::string name;
    float cpuPct;
    float memPct;
};

// htop-style memory breakdown. All fields are kilobytes. The four
// segments (active, wired, cached, free) cover the total: any byte not
// accounted for elsewhere is folded into `free` so the four always sum
// to `total` for the UI's segmented-bar math.
//
// "active"  = currently in use by running processes (anon + dirty)
// "wired"   = kernel + drivers + memory pinned by mlock / IOKit
// "cached"  = file-backed page cache, reclaimable on demand
// "free"    = immediately available
struct MemBreakdown
{
    unsigned long long totalKb;
    unsigned long long activeKb;
    unsigned long long wiredKb;
    unsigned long long cachedKb;
    unsigned long long freeKb;
};

class SystemMetrics
{
public:
    static float readCpuPercent();
    static float readMemPercent();
    static float readGpuPercent();
    // Per-logical-CPU utilization in [0, 100]. Same delta semantics as
    // readCpuPercent: returns the busy share since the previous call.
    // The first call after process start returns all zeros (no prior
    // sample to diff against). Vector size equals the number of logical
    // cores reported by the OS at call time.
    static std::vector<float> readPerCoreCpu();
    // Memory split into active / wired / cached / free segments. Used by
    // the htop-style segmented bar under the RAM row. Returns a zeroed
    // struct if the platform call fails.
    static MemBreakdown readMemBreakdown();
    // Seconds the system has been up. Returns 0 on platforms where the
    // call fails (no signal value other than 'unknown', the UI hides the
    // strip in that case).
    static unsigned long long readUptimeSeconds();
    // 1-, 5-, 15-minute load averages. Vector size is always 3. Falls
    // back to {0, 0, 0} on Windows since there is no portable analog.
    static std::vector<float> readLoadAverages();
    static std::vector<ProcessInfo> topProcesses(std::size_t n = 20);
};

#endif
