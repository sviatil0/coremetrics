#ifndef __PROC_PARSERS_HPP__
#define __PROC_PARSERS_HPP__

#include <string>
#include <vector>

namespace ProcParsers
{
    bool parseProcStat(const std::string &content,
                       unsigned long long &totalOut,
                       unsigned long long &idleOut);

    bool parseProcPidStatCpuTicks(const std::string &content,
                                  unsigned long long &ticksOut);

    bool parseProcStatusVmRssKb(const std::string &content,
                                unsigned long long &kbOut);

    struct CpuTicks
    {
        unsigned long long total;
        unsigned long long idle;
    };

    // Parses every per-core `cpuN` line from /proc/stat (skips the aggregate
    // `cpu` line on row 0). Out vector is in the file's order: index N maps
    // to cpuN. Returns false on empty input. Lines that don't parse are
    // skipped silently, so a truncated file produces a shorter vector
    // rather than an error.
    bool parseProcStatPerCore(const std::string &content,
                              std::vector<CpuTicks> &out);
}

#endif
