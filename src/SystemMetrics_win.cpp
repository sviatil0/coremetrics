#ifdef _WIN32

#include "SystemMetrics.hpp"
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <map>
#include <vector>

extern bool systemMetricsCompareByMemDesc(const ProcessInfo &a, const ProcessInfo &b);

static ULONGLONG g_lastTotal = 0;
static ULONGLONG g_lastIdle = 0;

// Per-process CPU% needs a delta: process ticks used between two samples over the
// system ticks that elapsed in the same window.
static std::map<DWORD, ULONGLONG> g_lastProcTicks;
static ULONGLONG g_lastProcSampleSysTotal = 0;

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

float SystemMetrics::readGpuPercent()
{
    static PDH_HQUERY query = nullptr;
    static PDH_HCOUNTER counter = nullptr;
    static bool initialized = false;

    if (!initialized)
    {
        if (PdhOpenQuery(NULL, 0, &query) != ERROR_SUCCESS)
            return 0.0f;

        if (PdhAddEnglishCounter(query,
            "\\GPU Engine(*)\\Utilization Percentage",
            0,
            &counter) != ERROR_SUCCESS)
            return 0.0f;

        PdhCollectQueryData(query);

        initialized = true;
        return 0.0f;
    }

    if (PdhCollectQueryData(query) != ERROR_SUCCESS)
        return 0.0f;

    DWORD bufferSize = 0;
    DWORD itemCount = 0;

    PdhGetFormattedCounterArray(
        counter,
        PDH_FMT_DOUBLE,
        &bufferSize,
        &itemCount,
        nullptr
    );

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
                if (sysTotalDiff > 0)
                {
                    auto prev = g_lastProcTicks.find(entry.th32ProcessID);
                    if (prev != g_lastProcTicks.end() && procTicks >= prev->second)
                    {
                        ULONGLONG procDiff = procTicks - prev->second;
                        info.cpuPct = (static_cast<float>(procDiff) / static_cast<float>(sysTotalDiff)) * 100.0f;
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

    std::sort(result.begin(), result.end(), systemMetricsCompareByMemDesc);
    if (result.size() > n)
    {
        result.resize(n);
    }
    return result;
}

#endif
