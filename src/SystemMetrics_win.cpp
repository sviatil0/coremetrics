#ifdef _WIN32

#include "SystemMetrics.hpp"
#include "ProcessUtils.hpp"
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
// GetIfTable2 / MIB_IF_TABLE2 are exposed only when _WIN32_WINNT is at
// least Vista (0x0600). Bump to Win7 (0x0601) so we get the newer
// 64-bit-counter interface table before windows.h is pulled in.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <map>
#include <vector>

extern bool systemMetricsCompareByMemDesc(const ProcessInfo &a, const ProcessInfo &b);

static ULONGLONG g_lastTotal = 0;
static ULONGLONG g_lastIdle = 0;

// Per-process CPU% needs a delta: process ticks used between two samples over the
// system ticks that elapsed in the same window.
static std::map<DWORD, ULONGLONG> g_lastProcTicks;
static ULONGLONG g_lastProcSampleSysTotal = 0;

// Per-process disk I/O. GetProcessIoCounters returns cumulative bytes;
// we diff against the prior sample and convert via the elapsed time
// since the previous topProcesses() call.
struct WinProcIoCounters
{
    ULONGLONG readBytes;
    ULONGLONG writeBytes;
};
static std::map<DWORD, WinProcIoCounters> g_lastProcIo;
static ULONGLONG g_lastProcIoSampleTickMs = 0;

static ULONGLONG fileTimeToULL(const FILETIME &ft)
{
    ULARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    return li.QuadPart;
}

float SystemMetrics::readCpuPercent()
{
    FILETIME idleFt;
    FILETIME kernelFt;
    FILETIME userFt;
    if (!GetSystemTimes(&idleFt, &kernelFt, &userFt))
    {
        return 0.0f;
    }
    ULONGLONG idle = fileTimeToULL(idleFt);
    ULONGLONG kernel = fileTimeToULL(kernelFt);
    ULONGLONG user = fileTimeToULL(userFt);
    ULONGLONG total = kernel + user;

    if (g_lastTotal == 0)
    {
        g_lastTotal = total;
        g_lastIdle = idle;
        return 0.0f;
    }

    ULONGLONG totalDiff = total - g_lastTotal;
    ULONGLONG idleDiff = idle - g_lastIdle;
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
    ULONGLONG ticks = GetTickCount64();
    return static_cast<unsigned long long>(ticks / 1000ULL);
}

std::vector<float> SystemMetrics::readLoadAverages()
{
    // No Windows analog for /proc/loadavg in the standard runtime. A real
    // implementation would sample the system queue length over time via
    // PDH and emulate the 1/5/15 EMA the way procps does. Out of scope
    // for now; return zeros so the UI string just shows '--'.
    return std::vector<float>{0.0f, 0.0f, 0.0f};
}

NetIo SystemMetrics::readNetIo()
{
    static ULONGLONG lastRx = 0;
    static ULONGLONG lastTx = 0;
    static ULONGLONG lastTickMs = 0;

    // GetIfTable is the legacy DWORD-counter path; MinGW headers do not
    // expose GetIfTable2 (which would give 64-bit counters) even with
    // _WIN32_WINNT=0x0601. Live with the 32-bit wrap for now and
    // diff-clamp at the delta below; a saturated gigabit link wraps
    // every ~34 s, so a 500ms poll catches the rollback cleanly.
    DWORD size = 0;
    GetIfTable(nullptr, &size, FALSE);
    if (size == 0)
    {
        return NetIo{0, 0};
    }
    std::vector<BYTE> buf(size);
    PMIB_IFTABLE table = reinterpret_cast<PMIB_IFTABLE>(buf.data());
    if (GetIfTable(table, &size, FALSE) != NO_ERROR)
    {
        return NetIo{0, 0};
    }

    ULONGLONG curRx = 0;
    ULONGLONG curTx = 0;
    for (DWORD i = 0; i < table->dwNumEntries; ++i)
    {
        const MIB_IFROW &row = table->table[i];
        if (row.dwType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        curRx += row.dwInOctets;
        curTx += row.dwOutOctets;
    }

    ULONGLONG nowMs = GetTickCount64();
    NetIo out{0, 0};
    if (lastTickMs != 0 && nowMs > lastTickMs)
    {
        double elapsedSec = static_cast<double>(nowMs - lastTickMs) / 1000.0;
        if (elapsedSec > 0.0)
        {
            ULONGLONG rxDelta = (curRx >= lastRx) ? curRx - lastRx : 0;
            ULONGLONG txDelta = (curTx >= lastTx) ? curTx - lastTx : 0;
            out.rxKbPerSec = static_cast<unsigned long long>(
                (static_cast<double>(rxDelta) / 1024.0) / elapsedSec);
            out.txKbPerSec = static_cast<unsigned long long>(
                (static_cast<double>(txDelta) / 1024.0) / elapsedSec);
        }
    }
    lastRx = curRx;
    lastTx = curTx;
    lastTickMs = nowMs;
    return out;
}

DiskUsage SystemMetrics::readDiskUsage()
{
    DiskUsage out{0, 0};
    ULARGE_INTEGER freeAvailable;
    ULARGE_INTEGER totalBytes;
    ULARGE_INTEGER totalFree;
    // "C:\\" is the historical Windows root volume; on systems where C:
    // is not the OS drive the user can still see relative space pressure
    // on their main data volume, which is the useful signal here.
    if (!GetDiskFreeSpaceExA("C:\\", &freeAvailable, &totalBytes, &totalFree))
    {
        return out;
    }
    out.totalKb = totalBytes.QuadPart / 1024ULL;
    out.freeKb = freeAvailable.QuadPart / 1024ULL;
    return out;
}

MemBreakdown SystemMetrics::readMemBreakdown()
{
    MemBreakdown out{0, 0, 0, 0, 0};
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms))
    {
        return out;
    }
    out.totalKb = ms.ullTotalPhys / 1024ULL;
    out.freeKb = ms.ullAvailPhys / 1024ULL;
    // GlobalMemoryStatusEx does not give an active / wired / cached split.
    // GetPerformanceInfo() does (Cached + Kernel*) but pulls in PsApi.h and
    // additional ntdll wiring; for now fold the whole 'used' bucket into
    // activeKb so the segmented bar still renders something meaningful.
    out.activeKb = (out.totalKb > out.freeKb) ? (out.totalKb - out.freeKb) : 0;
    return out;
}

std::vector<float> SystemMetrics::readPerCoreCpu()
{
    // PDH exposes a wildcard counter path that expands to one instance per
    // logical core: "\Processor(*)\% Processor Time". The "_Total" instance
    // is included by PDH and must be filtered out so the UI strip matches
    // the mac (host_processor_info) and linux (/proc/stat cpuN) backends,
    // which only emit physical core entries.
    //
    // PDH rate counters need two collects to compute a delta, so the first
    // call after init returns empty. Subsequent calls return one float per
    // core, clamped to [0, 100].
    static PDH_HQUERY query = nullptr;
    static PDH_HCOUNTER counter = nullptr;
    static bool initialized = false;

    if (!initialized)
    {
        if (PdhOpenQuery(NULL, 0, &query) != ERROR_SUCCESS)
        {
            query = nullptr;
            return std::vector<float>();
        }

        if (PdhAddEnglishCounter(query,
            "\\Processor(*)\\% Processor Time",
            0,
            &counter) != ERROR_SUCCESS)
        {
            // Mirror the GPU init: close the query before bailing so a
            // failed AddCounter does not leak the open handle on every
            // subsequent poll (audit-wave-5 finding).
            PdhCloseQuery(query);
            query = nullptr;
            counter = nullptr;
            return std::vector<float>();
        }

        // Prime the counter; rate counters need a prior sample to diff
        // against. The next call is the first one that can yield data.
        PdhCollectQueryData(query);

        initialized = true;
        return std::vector<float>();
    }

    if (PdhCollectQueryData(query) != ERROR_SUCCESS)
    {
        // Defensive reset: if a transient PDH error trips a collect (e.g.
        // perf counter subsystem hiccup), tear the query down so the next
        // call rebuilds it from scratch instead of polling a dead handle
        // forever. Matches the audit-wave-5 expectation for PDH paths.
        PdhCloseQuery(query);
        query = nullptr;
        counter = nullptr;
        initialized = false;
        return std::vector<float>();
    }

    DWORD bufferSize = 0;
    DWORD itemCount = 0;

    // Two-pass PDH read: first call asks for required sizes (returns
    // PDH_MORE_DATA), second call fills the buffer. Guard against the
    // "no data this round" case where bufferSize / itemCount are zero;
    // allocating a zero-length vector and writing through .data() is UB.
    PDH_STATUS probeStatus = PdhGetFormattedCounterArray(
        counter,
        PDH_FMT_DOUBLE,
        &bufferSize,
        &itemCount,
        nullptr
    );
    if (probeStatus != PDH_MORE_DATA || bufferSize == 0 || itemCount == 0)
    {
        return std::vector<float>();
    }

    std::vector<BYTE> buffer(bufferSize);
    auto items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buffer.data());

    if (PdhGetFormattedCounterArray(
            counter,
            PDH_FMT_DOUBLE,
            &bufferSize,
            &itemCount,
            items) != ERROR_SUCCESS)
    {
        return std::vector<float>();
    }

    std::vector<float> out;
    out.reserve(itemCount);
    for (DWORD i = 0; i < itemCount; ++i)
    {
        // PDH emits a synthetic "_Total" instance alongside per-core ones.
        // Skip it so the strip count matches the logical-core count the
        // other backends report.
        const char *name = items[i].szName;
        if (name != nullptr && std::string(name) == "_Total")
        {
            continue;
        }
        double v = items[i].FmtValue.doubleValue;
        if (v < 0.0)
        {
            v = 0.0;
        }
        if (v > 100.0)
        {
            v = 100.0;
        }
        out.push_back(static_cast<float>(v));
    }
    return out;
}

float SystemMetrics::readGpuPercent()
{
    static PDH_HQUERY query = nullptr;
    static PDH_HCOUNTER counter = nullptr;
    static bool initialized = false;

    if (!initialized)
    {
        if (PdhOpenQuery(NULL, 0, &query) != ERROR_SUCCESS)
        {
            query = nullptr;
            return 0.0f;
        }

        if (PdhAddEnglishCounter(query,
            "\\GPU Engine(*)\\Utilization Percentage",
            0,
            &counter) != ERROR_SUCCESS)
        {
            // Close the open query before bailing; otherwise the next
            // call (initialized still false) reopens, leaking the prior
            // handle every 500 ms and exhausting PDH over time.
            PdhCloseQuery(query);
            query = nullptr;
            return 0.0f;
        }

        PdhCollectQueryData(query);

        initialized = true;
        return 0.0f;
    }

    if (PdhCollectQueryData(query) != ERROR_SUCCESS)
        return 0.0f;

    DWORD bufferSize = 0;
    DWORD itemCount = 0;

    // PDH expects a two-pass call: first probe asks for sizes, second
    // call fills the buffer. PDH_MORE_DATA is the expected return for
    // the probe; ERROR_SUCCESS on the probe means there is nothing to
    // read (no counters yet). Treat bufferSize == 0 as "no data this
    // round" instead of allocating a zero-length vector and writing
    // through its data() pointer (UB; MSVC iterator-debug crashes).
    PDH_STATUS probeStatus = PdhGetFormattedCounterArray(
        counter,
        PDH_FMT_DOUBLE,
        &bufferSize,
        &itemCount,
        nullptr
    );
    if (probeStatus != PDH_MORE_DATA || bufferSize == 0 || itemCount == 0)
    {
        return 0.0f;
    }

    std::vector<BYTE> buffer(bufferSize);

    auto items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buffer.data());

    if (PdhGetFormattedCounterArray(
            counter,
            PDH_FMT_DOUBLE,
            &bufferSize,
            &itemCount,
            items) != ERROR_SUCCESS)
    {
        return 0.0f;
    }

    double maxEngine = 0.0;

    for (DWORD i = 0; i < itemCount; ++i)
    {
        if (items[i].FmtValue.doubleValue > maxEngine)
        {
            maxEngine = items[i].FmtValue.doubleValue;
        }
    }

    if (maxEngine > 100.0)
    {
        maxEngine = 100.0;
    }
    return static_cast<float>(maxEngine);
}

float SystemMetrics::readMemPercent()
{
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms))
    {
        return 0.0f;
    }
    return static_cast<float>(ms.dwMemoryLoad);
}

std::vector<ProcessInfo> SystemMetrics::topProcesses(std::size_t n)
{
    std::vector<ProcessInfo> result;

    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    DWORDLONG totalBytes = ms.ullTotalPhys;

    // Sample system-wide busy ticks once, as the denominator for per-process CPU%.
    FILETIME sysIdleFt;
    FILETIME sysKernelFt;
    FILETIME sysUserFt;
    ULONGLONG sysTotal = 0;
    if (GetSystemTimes(&sysIdleFt, &sysKernelFt, &sysUserFt))
    {
        sysTotal = fileTimeToULL(sysKernelFt) + fileTimeToULL(sysUserFt);
    }
    ULONGLONG sysTotalDiff = (g_lastProcSampleSysTotal == 0 || sysTotal < g_lastProcSampleSysTotal)
                                 ? 0
                                 : sysTotal - g_lastProcSampleSysTotal;
    std::map<DWORD, ULONGLONG> currentProcTicks;
    std::map<DWORD, WinProcIoCounters> currentProcIo;
    ULONGLONG nowTickMs = GetTickCount64();
    double ioElapsedSec = 0.0;
    if (g_lastProcIoSampleTickMs != 0 && nowTickMs > g_lastProcIoSampleTickMs)
    {
        ioElapsedSec = static_cast<double>(nowTickMs - g_lastProcIoSampleTickMs) / 1000.0;
    }

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
    {
        return result;
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(entry);
    if (!Process32First(snap, &entry))
    {
        CloseHandle(snap);
        return result;
    }

    do
    {
        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                                   FALSE, entry.th32ProcessID);
        ProcessInfo info;
        info.pid = static_cast<int>(entry.th32ProcessID);
        // PROCESSENTRY32.th32ParentProcessID is the parent pid as the
        // Toolhelp snapshot saw it; same source the Task Manager hierarchy
        // view uses.
        info.parentPid = static_cast<int>(entry.th32ParentProcessID);
        info.name = entry.szExeFile;
        info.cpuPct = 0.0f;
        info.memPct = 0.0f;

        if (hProc != nullptr)
        {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc)))
            {
                if (totalBytes > 0)
                {
                    info.memPct = (static_cast<float>(pmc.WorkingSetSize) / static_cast<float>(totalBytes)) * 100.0f;
                }
            }
            FILETIME createFt;
            FILETIME exitFt;
            FILETIME kernelFt;
            FILETIME userFt;
            if (GetProcessTimes(hProc, &createFt, &exitFt, &kernelFt, &userFt))
            {
                ULONGLONG procTicks = fileTimeToULL(kernelFt) + fileTimeToULL(userFt);
                currentProcTicks[entry.th32ProcessID] = procTicks;
                auto prev = g_lastProcTicks.find(entry.th32ProcessID);
                if (prev != g_lastProcTicks.end())
                {
                    info.cpuPct = computeCpuPercentDelta(
                        static_cast<std::uint64_t>(procTicks),
                        static_cast<std::uint64_t>(prev->second),
                        static_cast<std::uint64_t>(sysTotalDiff));
                }
            }
            IO_COUNTERS ioc;
            if (GetProcessIoCounters(hProc, &ioc))
            {
                WinProcIoCounters io{ioc.ReadTransferCount, ioc.WriteTransferCount};
                currentProcIo[entry.th32ProcessID] = io;
                if (ioElapsedSec > 0.0)
                {
                    auto prevIo = g_lastProcIo.find(entry.th32ProcessID);
                    if (prevIo != g_lastProcIo.end())
                    {
                        ULONGLONG readDelta = (io.readBytes >= prevIo->second.readBytes)
                                                  ? io.readBytes - prevIo->second.readBytes
                                                  : 0;
                        ULONGLONG writeDelta = (io.writeBytes >= prevIo->second.writeBytes)
                                                   ? io.writeBytes - prevIo->second.writeBytes
                                                   : 0;
                        info.diskReadKbPerSec = static_cast<unsigned long long>(
                            static_cast<double>(readDelta) / 1024.0 / ioElapsedSec);
                        info.diskWriteKbPerSec = static_cast<unsigned long long>(
                            static_cast<double>(writeDelta) / 1024.0 / ioElapsedSec);
                    }
                }
            }
            CloseHandle(hProc);
        }
        result.push_back(info);
    } while (Process32Next(snap, &entry));

    CloseHandle(snap);

    g_lastProcTicks = std::move(currentProcTicks);
    g_lastProcSampleSysTotal = sysTotal;
    g_lastProcIo = std::move(currentProcIo);
    g_lastProcIoSampleTickMs = nowTickMs;

    std::sort(result.begin(), result.end(), systemMetricsCompareByMemDesc);
    if (result.size() > n)
    {
        result.resize(n);
    }
    return result;
}

#endif
