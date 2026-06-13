#ifdef __linux__

#include "SystemMetrics.hpp"
#include "ProcParsers.hpp"
#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <map>
#include <sstream>

extern bool systemMetricsCompareByMemDesc(const ProcessInfo &a, const ProcessInfo &b);

static unsigned long long g_lastTotal = 0;
static unsigned long long g_lastIdle = 0;

// Per-process CPU% needs a delta: jiffies a process used between two samples,
// over the system-wide jiffies that elapsed in the same window.
static std::map<int, unsigned long long> g_lastProcTicks;
static unsigned long long g_lastProcSampleTotal = 0;

static std::string slurpFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return "";
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static bool readProcStatTotals(unsigned long long &total, unsigned long long &idle)
{
    return ProcParsers::parseProcStat(slurpFile("/proc/stat"), total, idle);
}

static bool isPidDir(const std::string &name)
{
    if (name.empty())
    {
        return false;
    }
    for (char c : name)
    {
        if (c < '0' || c > '9')
        {
            return false;
        }
    }
    return true;
}

static std::string readProcComm(int pid)
{
    std::ostringstream path;
    path << "/proc/" << pid << "/comm";
    std::string content = slurpFile(path.str());
    if (content.empty())
    {
        return "";
    }
    if (!content.empty() && content.back() == '\n')
    {
        content.pop_back();
    }
    return content;
}

static unsigned long long readProcCpuTicks(int pid)
{
    std::ostringstream path;
    path << "/proc/" << pid << "/stat";
    unsigned long long ticks = 0;
    ProcParsers::parseProcPidStatCpuTicks(slurpFile(path.str()), ticks);
    return ticks;
}

static unsigned long long readProcMemKb(int pid)
{
    std::ostringstream path;
    path << "/proc/" << pid << "/status";
    unsigned long long kb = 0;
    ProcParsers::parseProcStatusVmRssKb(slurpFile(path.str()), kb);
    return kb;
}

float SystemMetrics::readCpuPercent()
{
    unsigned long long total = 0;
    unsigned long long idle = 0;
    if (!readProcStatTotals(total, idle))
    {
        return 0.0f;
    }

    if (g_lastTotal == 0)
    {
        g_lastTotal = total;
        g_lastIdle = idle;
        return 0.0f;
    }

    unsigned long long totalDiff = total - g_lastTotal;
    unsigned long long idleDiff = idle - g_lastIdle;
    g_lastTotal = total;
    g_lastIdle = idle;

    if (totalDiff == 0)
    {
        return 0.0f;
    }
    float usage = 1.0f - (static_cast<float>(idleDiff) / static_cast<float>(totalDiff));
    return usage * 100.0f;
}

float SystemMetrics::readGpuPercent()
{
    std::ifstream file("/sys/class/drm/card0/device/gpu_busy_percent");
    if (!file.is_open())
    {
        return 0.0f;
    }
    int pct = 0;
    file >> pct;
    if (pct < 0)
    {
        pct = 0;
    }
    if (pct > 100)
    {
        pct = 100;
    }
    return static_cast<float>(pct);
}

float SystemMetrics::readMemPercent()
{
    std::ifstream file("/proc/meminfo");
    if (!file.is_open())
    {
        return 0.0f;
    }

    unsigned long long memTotal = 0;
    unsigned long long memAvailable = 0;
    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string key;
        unsigned long long value = 0;
        iss >> key >> value;
        if (key == "MemTotal:")
        {
            memTotal = value;
        }
        else if (key == "MemAvailable:")
        {
            memAvailable = value;
        }
        if (memTotal > 0 && memAvailable > 0)
        {
            break;
        }
    }

    if (memTotal == 0)
    {
        return 0.0f;
    }
    float used = static_cast<float>(memTotal - memAvailable);
    return (used / static_cast<float>(memTotal)) * 100.0f;
}

std::vector<ProcessInfo> SystemMetrics::topProcesses(std::size_t n)
{
    std::vector<ProcessInfo> result;
    DIR *dir = opendir("/proc");
    if (dir == nullptr)
    {
        return result;
    }

    unsigned long long memTotalKb = 0;
    {
        std::ifstream mem("/proc/meminfo");
        std::string line;
        if (std::getline(mem, line))
        {
            std::istringstream iss(line);
            std::string key;
            iss >> key >> memTotalKb;
        }
    }

    // Sample the system-wide total jiffies once, as the denominator for every
    // process's CPU% this round.
    unsigned long long sysTotal = 0;
    unsigned long long sysIdle = 0;
    readProcStatTotals(sysTotal, sysIdle);
    unsigned long long sysTotalDiff = (g_lastProcSampleTotal == 0 || sysTotal < g_lastProcSampleTotal)
                                          ? 0
                                          : sysTotal - g_lastProcSampleTotal;

    std::map<int, unsigned long long> currentProcTicks;

    struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr)
    {
        std::string name = entry->d_name;
        if (!isPidDir(name))
        {
            continue;
        }
        int pid = std::atoi(name.c_str());
        std::string procName = readProcComm(pid);
        if (procName.empty())
        {
            continue;
        }
        unsigned long long cpuTicks = readProcCpuTicks(pid);
        currentProcTicks[pid] = cpuTicks;

        float cpuPct = 0.0f;
        if (sysTotalDiff > 0)
        {
            auto prev = g_lastProcTicks.find(pid);
            if (prev != g_lastProcTicks.end() && cpuTicks >= prev->second)
            {
                unsigned long long procDiff = cpuTicks - prev->second;
                cpuPct = (static_cast<float>(procDiff) / static_cast<float>(sysTotalDiff)) * 100.0f;
            }
        }

        unsigned long long memKb = readProcMemKb(pid);
        float memPct = 0.0f;
        if (memTotalKb > 0)
        {
            memPct = (static_cast<float>(memKb) / static_cast<float>(memTotalKb)) * 100.0f;
        }
        ProcessInfo info;
        info.pid = pid;
        info.name = procName;
        info.cpuPct = cpuPct;
        info.memPct = memPct;
        result.push_back(info);
    }
    closedir(dir);

    g_lastProcTicks = std::move(currentProcTicks);
    g_lastProcSampleTotal = sysTotal;

    std::sort(result.begin(), result.end(), systemMetricsCompareByMemDesc);
    if (result.size() > n)
    {
        result.resize(n);
    }
    return result;
}

#endif
