#ifdef __linux__

#include "SystemMetrics.hpp"
#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sstream>

extern bool systemMetricsCompareByMemDesc(const ProcessInfo &a, const ProcessInfo &b);

static unsigned long long g_lastTotal = 0;
static unsigned long long g_lastIdle = 0;

static bool readProcStatTotals(unsigned long long &total, unsigned long long &idle)
{
    std::ifstream file("/proc/stat");
    if (!file.is_open())
    {
        return false;
    }
    std::string label;
    file >> label;
    if (label != "cpu")
    {
        return false;
    }

    unsigned long long user = 0;
    unsigned long long nice = 0;
    unsigned long long system = 0;
    unsigned long long idleVal = 0;
    unsigned long long iowait = 0;
    unsigned long long irq = 0;
    unsigned long long softirq = 0;
    unsigned long long steal = 0;
    file >> user >> nice >> system >> idleVal >> iowait >> irq >> softirq >> steal;

    idle = idleVal + iowait;
    total = user + nice + system + idle + irq + softirq + steal;
    return true;
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
    std::ifstream file(path.str());
    if (!file.is_open())
    {
        return "";
    }
    std::string name;
    std::getline(file, name);
    return name;
}

static unsigned long long readProcCpuTicks(int pid)
{
    std::ostringstream path;
    path << "/proc/" << pid << "/stat";
    std::ifstream file(path.str());
    if (!file.is_open())
    {
        return 0;
    }
    std::string content;
    std::getline(file, content);
    std::size_t rparen = content.rfind(')');
    if (rparen == std::string::npos)
    {
        return 0;
    }
    std::istringstream iss(content.substr(rparen + 1));
    std::string token;
    for (int i = 0; i < 12; ++i)
    {
        if (!(iss >> token))
        {
            return 0;
        }
    }
    unsigned long long utime = 0;
    unsigned long long stime = 0;
    iss >> utime >> stime;
    return utime + stime;
}

static unsigned long long readProcMemKb(int pid)
{
    std::ostringstream path;
    path << "/proc/" << pid << "/status";
    std::ifstream file(path.str());
    if (!file.is_open())
    {
        return 0;
    }
    std::string line;
    while (std::getline(file, line))
    {
        if (line.rfind("VmRSS:", 0) == 0)
        {
            std::istringstream iss(line.substr(6));
            unsigned long long kb = 0;
            iss >> kb;
            return kb;
        }
    }
    return 0;
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
        unsigned long long memKb = readProcMemKb(pid);
        float memPct = 0.0f;
        if (memTotalKb > 0)
        {
            memPct = (static_cast<float>(memKb) / static_cast<float>(memTotalKb)) * 100.0f;
        }
        ProcessInfo info;
        info.pid = pid;
        info.name = procName;
        info.cpuPct = static_cast<float>(cpuTicks);
        info.memPct = memPct;
        result.push_back(info);
    }
    closedir(dir);

    std::sort(result.begin(), result.end(), systemMetricsCompareByMemDesc);
    if (result.size() > n)
    {
        result.resize(n);
    }
    return result;
}

#endif
