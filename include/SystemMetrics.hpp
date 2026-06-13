#ifndef __SYSTEMMETRICS_HPP__
#define __SYSTEMMETRICS_HPP__

#include <cstddef>
#include <string>
#include <vector>

struct ProcessInfo
{
    int pid;
    std::string name;
    float cpuPct;
    float memPct;
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
    static std::vector<ProcessInfo> topProcesses(std::size_t n = 20);
};

#endif
