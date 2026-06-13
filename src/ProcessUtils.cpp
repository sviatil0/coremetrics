#include "ProcessUtils.hpp"

std::string formatPct(float value)
{
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed << value;
    return oss.str();
}

float computeCpuPercentDelta(std::uint64_t procCurrent,
                             std::uint64_t procPrevious,
                             std::uint64_t denom)
{
    if (denom == 0)
    {
        return 0.0f;
    }
    if (procCurrent < procPrevious)
    {
        return 0.0f;
    }
    std::uint64_t procDiff = procCurrent - procPrevious;
    float pct = (static_cast<float>(procDiff) / static_cast<float>(denom)) * 100.0f;
    if (pct < 0.0f)
    {
        return 0.0f;
    }
    if (pct > 100.0f)
    {
        return 100.0f;
    }
    return pct;
}

bool compareProcessByColumn(const ProcessInfo &a,
                            const ProcessInfo &b,
                            SortColumn column,
                            bool ascending)
{
    switch (column)
    {
    case SORT_PID:
        return ascending ? (a.pid < b.pid) : (a.pid > b.pid);
    case SORT_NAME:
        return ascending ? (a.name < b.name) : (a.name > b.name);
    case SORT_CPU:
        return ascending ? (a.cpuPct < b.cpuPct) : (a.cpuPct > b.cpuPct);
    case SORT_MEM:
        return ascending ? (a.memPct < b.memPct) : (a.memPct > b.memPct);
    }
    return false;
}
