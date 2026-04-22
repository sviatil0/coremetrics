#ifdef __APPLE__

#include "SystemMetrics.hpp"
#include <algorithm>
#include <cstring>
#include <libproc.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

extern bool systemMetricsCompareByMemDesc(const ProcessInfo &a, const ProcessInfo &b);

static uint64_t g_lastTotal = 0;
static uint64_t g_lastIdle = 0;

float SystemMetrics::readCpuPercent()
{
    host_cpu_load_info_data_t info;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                        reinterpret_cast<host_info_t>(&info), &count) != KERN_SUCCESS)
    {
        return 0.0f;
    }

    uint64_t user = info.cpu_ticks[CPU_STATE_USER];
    uint64_t sys = info.cpu_ticks[CPU_STATE_SYSTEM];
    uint64_t nice = info.cpu_ticks[CPU_STATE_NICE];
    uint64_t idle = info.cpu_ticks[CPU_STATE_IDLE];
    uint64_t total = user + sys + nice + idle;

    if (g_lastTotal == 0)
    {
        g_lastTotal = total;
        g_lastIdle = idle;
        return 0.0f;
    }

    uint64_t totalDiff = total - g_lastTotal;
    uint64_t idleDiff = idle - g_lastIdle;
    g_lastTotal = total;
    g_lastIdle = idle;

    if (totalDiff == 0)
    {
        return 0.0f;
    }
    float usage = 1.0f - (static_cast<float>(idleDiff) / static_cast<float>(totalDiff));
    return usage * 100.0f;
}

float SystemMetrics::readMemPercent()
{
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t memSize = 0;
    size_t memSizeLen = sizeof(memSize);
    if (sysctl(mib, 2, &memSize, &memSizeLen, nullptr, 0) != 0 || memSize == 0)
    {
        return 0.0f;
    }

    vm_statistics64_data_t vmStats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          reinterpret_cast<host_info64_t>(&vmStats), &count) != KERN_SUCCESS)
    {
        return 0.0f;
    }

    vm_size_t pageSize = 0;
    host_page_size(mach_host_self(), &pageSize);

    uint64_t freeBytes = static_cast<uint64_t>(vmStats.free_count + vmStats.inactive_count) * pageSize;
    if (freeBytes >= memSize)
    {
        return 0.0f;
    }
    float used = static_cast<float>(memSize - freeBytes);
    return (used / static_cast<float>(memSize)) * 100.0f;
}

std::vector<ProcessInfo> SystemMetrics::topProcesses(std::size_t n)
{
    std::vector<ProcessInfo> result;

    int pidCount = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
    if (pidCount <= 0)
    {
        return result;
    }
    std::vector<pid_t> pids(pidCount / sizeof(pid_t) + 8, 0);
    int actualBytes = proc_listpids(PROC_ALL_PIDS, 0, pids.data(),
                                    static_cast<int>(pids.size() * sizeof(pid_t)));
    if (actualBytes <= 0)
    {
        return result;
    }
    int actualCount = actualBytes / static_cast<int>(sizeof(pid_t));

    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t memSize = 0;
    size_t memSizeLen = sizeof(memSize);
    sysctl(mib, 2, &memSize, &memSizeLen, nullptr, 0);

    for (int i = 0; i < actualCount; ++i)
    {
        pid_t pid = pids[i];
        if (pid == 0)
        {
            continue;
        }
        char name[PROC_PIDPATHINFO_MAXSIZE];
        std::memset(name, 0, sizeof(name));
        int nameLen = proc_name(pid, name, sizeof(name));
        if (nameLen <= 0)
        {
            continue;
        }
        struct proc_taskinfo taskInfo;
        if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &taskInfo, sizeof(taskInfo)) <= 0)
        {
            continue;
        }
        ProcessInfo info;
        info.pid = pid;
        info.name = name;
        info.cpuPct = static_cast<float>(taskInfo.pti_total_user + taskInfo.pti_total_system);
        if (memSize > 0)
        {
            info.memPct = (static_cast<float>(taskInfo.pti_resident_size) / static_cast<float>(memSize)) * 100.0f;
        }
        else
        {
            info.memPct = 0.0f;
        }
        result.push_back(info);
    }

    std::sort(result.begin(), result.end(), systemMetricsCompareByMemDesc);
    if (result.size() > n)
    {
        result.resize(n);
    }
    return result;
}

#endif
