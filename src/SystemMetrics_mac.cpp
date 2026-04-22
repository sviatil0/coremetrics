#ifdef __APPLE__

#include "SystemMetrics.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <libproc.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

extern bool systemMetricsCompareByMemDesc(const ProcessInfo &a, const ProcessInfo &b);

static uint64_t g_lastTotal = 0;
static uint64_t g_lastIdle = 0;

static std::unordered_map<pid_t, uint64_t> g_lastProcCpuNs;
static uint64_t g_lastSampleNs = 0;

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

float SystemMetrics::readGpuPercent()
{
    float result = 0.0f;
    CFMutableDictionaryRef match = IOServiceMatching("IOAccelerator");
    if (match == nullptr)
    {
        return 0.0f;
    }
    io_iterator_t iter = 0;
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, match, &iter) != KERN_SUCCESS)
    {
        return 0.0f;
    }
    io_registry_entry_t entry = 0;
    while ((entry = IOIteratorNext(iter)) != 0)
    {
        CFMutableDictionaryRef props = nullptr;
        if (IORegistryEntryCreateCFProperties(entry, &props, kCFAllocatorDefault, 0) == KERN_SUCCESS
            && props != nullptr)
        {
            CFDictionaryRef perf = static_cast<CFDictionaryRef>(
                CFDictionaryGetValue(props, CFSTR("PerformanceStatistics")));
            if (perf != nullptr)
            {
                CFNumberRef util = static_cast<CFNumberRef>(
                    CFDictionaryGetValue(perf, CFSTR("Device Utilization %")));
                if (util != nullptr)
                {
                    int value = 0;
                    if (CFNumberGetValue(util, kCFNumberIntType, &value))
                    {
                        if (static_cast<float>(value) > result)
                        {
                            result = static_cast<float>(value);
                        }
                    }
                }
            }
            CFRelease(props);
        }
        IOObjectRelease(entry);
    }
    IOObjectRelease(iter);
    return result;
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

    uint64_t nowNs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    uint64_t wallDiffNs = 0;
    if (g_lastSampleNs != 0 && nowNs > g_lastSampleNs)
    {
        wallDiffNs = nowNs - g_lastSampleNs;
    }

    std::unordered_map<pid_t, uint64_t> currentProcCpuNs;

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
        uint64_t procCpuNs = taskInfo.pti_total_user + taskInfo.pti_total_system;
        currentProcCpuNs[pid] = procCpuNs;

        float cpuPct = 0.0f;
        auto it = g_lastProcCpuNs.find(pid);
        if (it != g_lastProcCpuNs.end() && wallDiffNs > 0 && procCpuNs >= it->second)
        {
            uint64_t procDiffNs = procCpuNs - it->second;
            cpuPct = (static_cast<float>(procDiffNs) / static_cast<float>(wallDiffNs)) * 100.0f;
        }

        ProcessInfo info;
        info.pid = pid;
        info.name = name;
        info.cpuPct = cpuPct;
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

    g_lastProcCpuNs = std::move(currentProcCpuNs);
    g_lastSampleNs = nowNs;

    std::sort(result.begin(), result.end(), systemMetricsCompareByMemDesc);
    if (result.size() > n)
    {
        result.resize(n);
    }
    return result;
}

#endif
