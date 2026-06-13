#ifdef __linux__

#include "SystemMetrics.hpp"
#include "ProcParsers.hpp"
#include "ProcessUtils.hpp"
#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <map>
#include <sstream>

extern bool systemMetricsCompareByMemDesc(const ProcessInfo &a, const ProcessInfo &b);

static std::vector<unsigned long long> g_lastPerCoreTotal;
static std::vector<unsigned long long> g_lastPerCoreIdle;

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

unsigned long long SystemMetrics::readUptimeSeconds()
{
    std::ifstream file("/proc/uptime");
    if (!file.is_open())
    {
        return 0;
    }
    double secs = 0.0;
    file >> secs;
    return secs > 0.0 ? static_cast<unsigned long long>(secs) : 0;
}

std::vector<float> SystemMetrics::readLoadAverages()
{
    std::ifstream file("/proc/loadavg");
    if (!file.is_open())
    {
        return std::vector<float>{0.0f, 0.0f, 0.0f};
    }
    float l1 = 0.0f, l5 = 0.0f, l15 = 0.0f;
    file >> l1 >> l5 >> l15;
    return std::vector<float>{l1, l5, l15};
}

MemBreakdown SystemMetrics::readMemBreakdown()
{
    MemBreakdown out{0, 0, 0, 0, 0};
    std::ifstream file("/proc/meminfo");
    if (!file.is_open())
    {
        return out;
    }

    unsigned long long memTotal = 0;
    unsigned long long memFree = 0;
    unsigned long long buffers = 0;
    unsigned long long cached = 0;
    unsigned long long sReclaimable = 0;
    unsigned long long shmem = 0;
    unsigned long long active = 0;

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string key;
        unsigned long long value = 0;
        iss >> key >> value;
        if (key == "MemTotal:") memTotal = value;
        else if (key == "MemFree:") memFree = value;
        else if (key == "Buffers:") buffers = value;
        else if (key == "Cached:") cached = value;
        else if (key == "SReclaimable:") sReclaimable = value;
        else if (key == "Shmem:") shmem = value;
        else if (key == "Active:") active = value;
    }
    if (memTotal == 0)
    {
        return out;
    }

    out.totalKb = memTotal;
    out.freeKb = memFree;
    // htop's cached bucket: page cache + reclaimable slab + buffers,
    // minus shmem which is double-counted between Cached and Shmem on
    // Linux. Guarded against underflow.
    unsigned long long cachedAll = cached + sReclaimable + buffers;
    if (shmem <= cachedAll) cachedAll -= shmem;
    out.cachedKb = cachedAll;
    out.activeKb = active;
    // Wired ~= total - (active + cached + free). Bounded at zero so the
    // math cannot go negative on an unusual layout.
    unsigned long long accounted = out.cachedKb + out.activeKb + out.freeKb;
    out.wiredKb = (memTotal > accounted) ? (memTotal - accounted) : 0;
    return out;
}

std::vector<float> SystemMetrics::readPerCoreCpu()
{
    std::vector<ProcParsers::CpuTicks> ticks;
    if (!ProcParsers::parseProcStatPerCore(slurpFile("/proc/stat"), ticks))
    {
        return std::vector<float>();
    }

    std::vector<float> result(ticks.size(), 0.0f);
    bool firstSample = (g_lastPerCoreTotal.size() != ticks.size());
    if (firstSample)
    {
        g_lastPerCoreTotal.assign(ticks.size(), 0);
        g_lastPerCoreIdle.assign(ticks.size(), 0);
    }

    for (std::size_t i = 0; i < ticks.size(); ++i)
    {
        if (!firstSample)
        {
            unsigned long long totalDiff = (ticks[i].total >= g_lastPerCoreTotal[i])
                                               ? ticks[i].total - g_lastPerCoreTotal[i]
                                               : 0;
            unsigned long long idleDiff = (ticks[i].idle >= g_lastPerCoreIdle[i])
                                              ? ticks[i].idle - g_lastPerCoreIdle[i]
                                              : 0;
            if (totalDiff > 0)
            {
                float usage = 1.0f - (static_cast<float>(idleDiff) / static_cast<float>(totalDiff));
                if (usage < 0.0f) usage = 0.0f;
                if (usage > 1.0f) usage = 1.0f;
                result[i] = usage * 100.0f;
            }
        }
        g_lastPerCoreTotal[i] = ticks[i].total;
        g_lastPerCoreIdle[i] = ticks[i].idle;
    }
    return result;
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
        auto prev = g_lastProcTicks.find(pid);
        if (prev != g_lastProcTicks.end())
        {
            cpuPct = computeCpuPercentDelta(cpuTicks, prev->second, sysTotalDiff);
        }

        unsigned long long memKb = readProcMemKb(pid);
        float memPct = 0.0f;
        if (memTotalKb > 0)
        {
            memPct = (static_cast<float>(memKb) / static_cast<float>(memTotalKb)) * 100.0f;
        }
        // /proc/[pid]/stat field 4 is the parent pid (1-based after the
        // ppid_position offset that ProcParsers uses for the ticks). The
        // parser already grabs utime+stime; we read ppid here without
        // adding a second pure helper since it's a one-off field.
        int parentPid = 0;
        {
            std::ostringstream statPath;
            statPath << "/proc/" << pid << "/stat";
            std::ifstream statFile(statPath.str());
            if (statFile.is_open())
            {
                std::string content;
                std::getline(statFile, content);
                std::size_t rparen = content.rfind(')');
                if (rparen != std::string::npos && rparen + 1 < content.size())
                {
                    std::istringstream iss(content.substr(rparen + 1));
                    std::string state;
                    int ppid = 0;
                    if (iss >> state >> ppid)
                    {
                        parentPid = ppid;
                    }
                }
            }
        }

        ProcessInfo info;
        info.pid = pid;
        info.parentPid = parentPid;
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
