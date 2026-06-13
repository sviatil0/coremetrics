#ifdef __APPLE__

#include "SystemMetrics.hpp"
#include "ProcessUtils.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <libproc.h>

// The default IOKit port is kIOMainPortDefault (macOS 12+) or the deprecated
// kIOMasterPortDefault on older SDKs; both are documented as MACH_PORT_NULL.
// Passing the literal null port avoids depending on either name (and its
// deprecation warning) while behaving identically on every macOS version.
static const mach_port_t MS_IO_DEFAULT_PORT = MACH_PORT_NULL;
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <mach/processor_info.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

extern bool systemMetricsCompareByMemDesc(const ProcessInfo &a, const ProcessInfo &b);

static uint64_t g_lastTotal = 0;
static uint64_t g_lastIdle = 0;

static std::unordered_map<pid_t, uint64_t> g_lastProcCpuNs;
static uint64_t g_lastSampleNs = 0;

// Per-core tick history. Vector size = logical CPU count. Two slots per
// core: [previousTotalTicks, previousIdleTicks]. Resized lazily on the
// first call to readPerCoreCpu().
static std::vector<uint64_t> g_lastPerCoreTotal;
static std::vector<uint64_t> g_lastPerCoreIdle;

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

std::vector<float> SystemMetrics::readPerCoreCpu()
{
    natural_t cpuCount = 0;
    processor_info_array_t infoArray = nullptr;
    mach_msg_type_number_t infoCount = 0;
    if (host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO,
                            &cpuCount, &infoArray, &infoCount) != KERN_SUCCESS)
    {
        return std::vector<float>();
    }

    std::vector<float> result(cpuCount, 0.0f);
    bool firstSample = (g_lastPerCoreTotal.size() != cpuCount);
    if (firstSample)
    {
        g_lastPerCoreTotal.assign(cpuCount, 0);
        g_lastPerCoreIdle.assign(cpuCount, 0);
    }

    processor_cpu_load_info_t loads = reinterpret_cast<processor_cpu_load_info_t>(infoArray);
    for (natural_t i = 0; i < cpuCount; ++i)
    {
        uint64_t user = loads[i].cpu_ticks[CPU_STATE_USER];
        uint64_t sys = loads[i].cpu_ticks[CPU_STATE_SYSTEM];
        uint64_t nice = loads[i].cpu_ticks[CPU_STATE_NICE];
        uint64_t idle = loads[i].cpu_ticks[CPU_STATE_IDLE];
        uint64_t total = user + sys + nice + idle;

        if (!firstSample)
        {
            uint64_t totalDiff = total - g_lastPerCoreTotal[i];
            uint64_t idleDiff = idle - g_lastPerCoreIdle[i];
            if (totalDiff > 0)
            {
                float usage = 1.0f - (static_cast<float>(idleDiff) / static_cast<float>(totalDiff));
                if (usage < 0.0f) usage = 0.0f;
                if (usage > 1.0f) usage = 1.0f;
                result[i] = usage * 100.0f;
            }
        }
        g_lastPerCoreTotal[i] = total;
        g_lastPerCoreIdle[i] = idle;
    }

    vm_deallocate(mach_task_self(),
                  reinterpret_cast<vm_address_t>(infoArray),
                  static_cast<vm_size_t>(infoCount * sizeof(integer_t)));
    return result;
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
    if (IOServiceGetMatchingServices(MS_IO_DEFAULT_PORT, match, &iter) != KERN_SUCCESS)
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
        if (it != g_lastProcCpuNs.end())
        {
            cpuPct = computeCpuPercentDelta(procCpuNs, it->second, wallDiffNs);
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
