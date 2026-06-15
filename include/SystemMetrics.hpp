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
    // Disk I/O throughput in kilobytes per second since the previous
    // topProcesses() sample. Zero on the very first sample for a pid
    // (nothing to diff against) and on platforms where the rate cannot
    // be derived (Windows GetProcessIoCounters reports cumulative bytes
    // and is fully supported; mac proc_pidinfo PROC_PIDRUSAGE_V2 and
    // linux /proc/<pid>/io are also supported).
    unsigned long long diskReadKbPerSec = 0;
    unsigned long long diskWriteKbPerSec = 0;
};

// Aggregate network I/O throughput summed across every active
// interface. Both fields are kilobytes per second sampled between
// successive readNetIo() calls. Zero on the first call (no prior
// sample to diff) and on platforms where the call fails.
struct NetIo
{
    unsigned long long rxKbPerSec;
    unsigned long long txKbPerSec;
};

// Disk usage for the root volume. totalKb is the total capacity; freeKb
// is the user-visible free space (after reserved blocks). Zero on both
// fields when the platform call fails so the UI can decide whether to
// render the strip at all.
struct DiskUsage
{
    unsigned long long totalKb;
    unsigned long long freeKb;
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
    // Root volume capacity. Returns zeroed totals on platforms where the
    // call fails (in which case the UI hides the strip).
    static DiskUsage readDiskUsage();
    // Aggregate network rx + tx rate in KB/sec since the previous call.
    // First call returns zeros (no prior sample to diff). Backends sum
    // every interface returned by the platform call; the loopback
    // interface is excluded so localhost traffic does not inflate the
    // visible rate.
    static NetIo readNetIo();
    static std::vector<ProcessInfo> topProcesses(std::size_t n = 20);
};

#endif
