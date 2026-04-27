#ifdef _WIN32

#include "SystemMetrics.hpp"
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

extern bool systemMetricsCompareByMemDesc(const ProcessInfo &a, const ProcessInfo &b);

static ULONGLONG g_lastTotal = 0;
static ULONGLONG g_lastIdle = 0;

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
    // TODO: PDH counter "\GPU Engine(*)\Utilization Percentage"
    return 0.0f;
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
                info.cpuPct = static_cast<float>(fileTimeToULL(kernelFt) + fileTimeToULL(userFt));
            }
            CloseHandle(hProc);
        }
        result.push_back(info);
    } while (Process32Next(snap, &entry));

    CloseHandle(snap);

    std::sort(result.begin(), result.end(), systemMetricsCompareByMemDesc);
    if (result.size() > n)
    {
        result.resize(n);
    }
    return result;
}

#endif
