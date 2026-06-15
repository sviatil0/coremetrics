#include "ProcessUtils.hpp"
#include <cctype>

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
    case SORT_DISK:
    {
        unsigned long long ai = a.diskReadKbPerSec + a.diskWriteKbPerSec;
        unsigned long long bi = b.diskReadKbPerSec + b.diskWriteKbPerSec;
        return ascending ? (ai < bi) : (ai > bi);
    }
    }
    return false;
}

std::string formatDiskIo(unsigned long long readKbPerSec,
                         unsigned long long writeKbPerSec)
{
    unsigned long long total = readKbPerSec + writeKbPerSec;
    if (total == 0)
    {
        return "";
    }
    if (total >= 1024)
    {
        std::ostringstream oss;
        oss.precision(1);
        oss << std::fixed
            << (static_cast<double>(total) / 1024.0)
            << " MB/s";
        return oss.str();
    }
    return std::to_string(total) + " KB/s";
}

std::uint64_t computeIoKbPerSec(std::uint64_t prev,
                                std::uint64_t curr,
                                double elapsedSec)
{
    if (elapsedSec <= 0.0 || curr < prev)
    {
        return 0;
    }
    std::uint64_t delta = curr - prev;
    return static_cast<std::uint64_t>(
        (static_cast<double>(delta) / 1024.0) / elapsedSec);
}

bool processNameMatchesFilter(const std::string &name,
                              const std::string &needle)
{
    if (needle.empty())
    {
        return true;
    }
    if (name.empty())
    {
        return false;
    }
    std::string lowerNeedle;
    lowerNeedle.reserve(needle.size());
    for (char c : needle)
    {
        lowerNeedle.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(c))));
    }
    std::string lowerName;
    lowerName.reserve(name.size());
    for (char c : name)
    {
        lowerName.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(c))));
    }
    return lowerName.find(lowerNeedle) != std::string::npos;
}

std::string formatGbString(unsigned long long kb)
{
    unsigned long long gb = kb / (1024ULL * 1024ULL);
    return std::to_string(gb);
}

float computeDiskUsedPct(unsigned long long totalKb,
                         unsigned long long freeKb)
{
    if (totalKb == 0 || freeKb >= totalKb)
    {
        return 0.0f;
    }
    unsigned long long usedKb = totalKb - freeKb;
    float pct = 100.0f * static_cast<float>(usedKb) / static_cast<float>(totalKb);
    if (pct < 0.0f) return 0.0f;
    if (pct > 100.0f) return 100.0f;
    return pct;
}

std::string formatUptimeString(unsigned long long seconds)
{
    if (seconds == 0)
    {
        return "Up --";
    }
    unsigned long long days = seconds / 86400;
    seconds %= 86400;
    unsigned long long hrs = seconds / 3600;
    seconds %= 3600;
    unsigned long long mins = seconds / 60;
    std::string out = "Up ";
    if (days > 0)
    {
        out += std::to_string(days) + "d ";
    }
    if (days > 0 || hrs > 0)
    {
        out += std::to_string(hrs) + "h ";
    }
    out += std::to_string(mins) + "m";
    return out;
}
