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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <unordered_map>
#include <vector>

extern bool systemMetricsCompareByMemDesc(const ProcessInfo &a, const ProcessInfo &b);

static uint64_t g_lastTotal = 0;
static uint64_t g_lastIdle = 0;

static std::unordered_map<pid_t, uint64_t> g_lastProcCpuNs;
static uint64_t g_lastSampleNs = 0;

// Per-process disk I/O rate. proc_pid_rusage gives cumulative bytes; we
// diff against the previous sample and divide by elapsed wall-clock
// seconds to land at KB/sec.
struct MacProcIoCounters
{
    uint64_t readBytes;
    uint64_t writeBytes;
};
static std::unordered_map<pid_t, MacProcIoCounters> g_lastProcIo;

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

unsigned long long SystemMetrics::readUptimeSeconds()
{
    struct timeval bootTime;
    size_t len = sizeof(bootTime);
    int mib[2] = {CTL_KERN, KERN_BOOTTIME};
    if (sysctl(mib, 2, &bootTime, &len, nullptr, 0) != 0 || bootTime.tv_sec == 0)
    {
        return 0;
    }
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    long long diff = static_cast<long long>(now) - static_cast<long long>(bootTime.tv_sec);
    return diff > 0 ? static_cast<unsigned long long>(diff) : 0;
}

std::vector<float> SystemMetrics::readLoadAverages()
{
    double loads[3] = {0.0, 0.0, 0.0};
    if (getloadavg(loads, 3) != 3)
    {
        return std::vector<float>{0.0f, 0.0f, 0.0f};
    }
    return std::vector<float>{
        static_cast<float>(loads[0]),
        static_cast<float>(loads[1]),
        static_cast<float>(loads[2])
    };
}

MemBreakdown SystemMetrics::readMemBreakdown()
{
    MemBreakdown out{0, 0, 0, 0, 0};

    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t memSize = 0;
    size_t memSizeLen = sizeof(memSize);
    if (sysctl(mib, 2, &memSize, &memSizeLen, nullptr, 0) != 0 || memSize == 0)
    {
        return out;
    }

    vm_statistics64_data_t vmStats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          reinterpret_cast<host_info64_t>(&vmStats), &count) != KERN_SUCCESS)
    {
        return out;
    }

    vm_size_t pageSize = 0;
    host_page_size(mach_host_self(), &pageSize);
    if (pageSize == 0)
    {
        return out;
    }

    auto pagesToKb = [pageSize](uint64_t pages) -> unsigned long long {
        return (static_cast<unsigned long long>(pages) * pageSize) / 1024ULL;
    };

    out.totalKb = memSize / 1024ULL;
    out.activeKb = pagesToKb(static_cast<uint64_t>(vmStats.active_count));
    out.wiredKb = pagesToKb(static_cast<uint64_t>(vmStats.wire_count));
    // External page count is Darwin's file-backed reclaimable bucket; fall
    // back to inactive_count on older kernels that do not expose it.
    uint64_t cachedPages = static_cast<uint64_t>(vmStats.external_page_count);
    if (cachedPages == 0)
    {
        cachedPages = static_cast<uint64_t>(vmStats.inactive_count);
    }
    out.cachedKb = pagesToKb(cachedPages);
    out.freeKb = pagesToKb(static_cast<uint64_t>(vmStats.free_count));

    // Fold any rounding remainder into 'free' so segments sum to total.
    unsigned long long sum = out.activeKb + out.wiredKb + out.cachedKb + out.freeKb;
    if (sum < out.totalKb)
    {
        out.freeKb += (out.totalKb - sum);
    }
    else if (sum > out.totalKb)
    {
        unsigned long long over = sum - out.totalKb;
        unsigned long long shave = (over <= out.cachedKb) ? over : out.cachedKb;
        out.cachedKb -= shave;
        over -= shave;
        if (over > 0 && over <= out.freeKb)
        {
            out.freeKb -= over;
        }
    }
    return out;
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
    std::unordered_map<pid_t, MacProcIoCounters> currentProcIo;
    double ioElapsedSec = (wallDiffNs > 0)
                              ? static_cast<double>(wallDiffNs) / 1.0e9
                              : 0.0;

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

        // PROC_PIDTBSDINFO has pbi_ppid which is the parent pid the
        // BSD layer tracks. Falls back to 0 if the call fails so the
        // tree-view treats unknown parents as roots.
        int parentPid = 0;
        struct proc_bsdinfo bsdInfo;
        if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &bsdInfo, sizeof(bsdInfo)) > 0)
        {
            parentPid = static_cast<int>(bsdInfo.pbi_ppid);
        }

        // proc_pid_rusage v4 carries cumulative disk-IO byte counts; older
        // SDKs only define v2 but the field offsets are stable. Failure
        // is non-fatal: the rate stays at zero for this pid.
        MacProcIoCounters io{0, 0};
        rusage_info_current rusage;
        if (proc_pid_rusage(pid, RUSAGE_INFO_CURRENT,
                            reinterpret_cast<rusage_info_t *>(&rusage)) == 0)
        {
            io.readBytes = rusage.ri_diskio_bytesread;
            io.writeBytes = rusage.ri_diskio_byteswritten;
        }
        currentProcIo[pid] = io;
        unsigned long long readKbPerSec = 0;
        unsigned long long writeKbPerSec = 0;
        if (ioElapsedSec > 0.0)
        {
            auto prevIo = g_lastProcIo.find(pid);
            if (prevIo != g_lastProcIo.end())
            {
                uint64_t readDelta = (io.readBytes >= prevIo->second.readBytes)
                                         ? io.readBytes - prevIo->second.readBytes
                                         : 0;
                uint64_t writeDelta = (io.writeBytes >= prevIo->second.writeBytes)
                                          ? io.writeBytes - prevIo->second.writeBytes
                                          : 0;
                readKbPerSec = static_cast<unsigned long long>(
                    static_cast<double>(readDelta) / 1024.0 / ioElapsedSec);
                writeKbPerSec = static_cast<unsigned long long>(
                    static_cast<double>(writeDelta) / 1024.0 / ioElapsedSec);
            }
        }

        ProcessInfo info;
        info.pid = pid;
        info.parentPid = parentPid;
        info.name = name;
        info.cpuPct = cpuPct;
        if (memSize > 0)
        {
            // pti_resident_size is bytes (uint64_t). Casting to float before
            // the divide loses ~3 significant digits for processes over a
            // few hundred MB; do the ratio in double then narrow.
            info.memPct = static_cast<float>(
                (static_cast<double>(taskInfo.pti_resident_size)
                 / static_cast<double>(memSize)) * 100.0);
        }
        else
        {
            info.memPct = 0.0f;
        }
        info.diskReadKbPerSec = readKbPerSec;
        info.diskWriteKbPerSec = writeKbPerSec;
        result.push_back(info);
    }

    g_lastProcCpuNs = std::move(currentProcCpuNs);
    g_lastSampleNs = nowNs;
    g_lastProcIo = std::move(currentProcIo);

    std::sort(result.begin(), result.end(), systemMetricsCompareByMemDesc);
    if (result.size() > n)
    {
        result.resize(n);
    }
    return result;
}

#endif
