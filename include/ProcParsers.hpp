#ifndef __PROC_PARSERS_HPP__
#define __PROC_PARSERS_HPP__

#include <string>

namespace ProcParsers
{
    bool parseProcStat(const std::string &content,
                       unsigned long long &totalOut,
                       unsigned long long &idleOut);

    bool parseProcPidStatCpuTicks(const std::string &content,
                                  unsigned long long &ticksOut);

    bool parseProcStatusVmRssKb(const std::string &content,
                                unsigned long long &kbOut);
}

#endif
