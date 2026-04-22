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
    static std::vector<ProcessInfo> topProcesses(std::size_t n = 20);
};

#endif
