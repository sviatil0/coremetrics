#include "ProcParsers.hpp"
#include <sstream>

namespace ProcParsers
{

bool parseProcStat(const std::string &content,
                   unsigned long long &totalOut,
                   unsigned long long &idleOut)
{
    totalOut = 0;
    idleOut = 0;
    if (content.empty())
    {
        return false;
    }

    std::istringstream iss(content);
    std::string label;
    if (!(iss >> label) || label != "cpu")
    {
        return false;
    }

    unsigned long long user = 0;
    unsigned long long nice = 0;
    unsigned long long system = 0;
    unsigned long long idleVal = 0;
    unsigned long long iowait = 0;
    unsigned long long irq = 0;
    unsigned long long softirq = 0;
    unsigned long long steal = 0;
    if (!(iss >> user >> nice >> system >> idleVal))
    {
        return false;
    }
    iss >> iowait >> irq >> softirq >> steal;

    idleOut = idleVal + iowait;
    totalOut = user + nice + system + idleOut + irq + softirq + steal;
    return true;
}

bool parseProcPidStatCpuTicks(const std::string &content,
                              unsigned long long &ticksOut)
{
    ticksOut = 0;
    std::size_t rparen = content.rfind(')');
    if (rparen == std::string::npos || rparen + 1 >= content.size())
    {
        return false;
    }

    std::istringstream iss(content.substr(rparen + 1));
    std::string token;
    for (int i = 0; i < 11; ++i)
    {
        if (!(iss >> token))
        {
            return false;
        }
    }
    unsigned long long utime = 0;
    unsigned long long stime = 0;
    if (!(iss >> utime >> stime))
    {
        return false;
    }
    ticksOut = utime + stime;
    return true;
}

bool parseProcStatPerCore(const std::string &content,
                          std::vector<CpuTicks> &out)
{
    out.clear();
    if (content.empty())
    {
        return false;
    }

    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line))
    {
        // Per-core lines look like "cpuN user nice system idle iowait irq softirq steal"
        // where N is the logical core index. The aggregate line "cpu " on
        // row 0 is intentionally skipped; readCpuPercent already handles
        // that one and the per-core view would double-count it.
        if (line.size() < 4 || line.compare(0, 3, "cpu") != 0)
        {
            continue;
        }
        // Reject the aggregate "cpu " label (space after "cpu"): we want
        // only the indexed cpuN entries.
        if (line[3] < '0' || line[3] > '9')
        {
            continue;
        }
        std::istringstream lineStream(line);
        std::string label;
        unsigned long long user = 0;
        unsigned long long nice = 0;
        unsigned long long system = 0;
        unsigned long long idleVal = 0;
        unsigned long long iowait = 0;
        unsigned long long irq = 0;
        unsigned long long softirq = 0;
        unsigned long long steal = 0;
        if (!(lineStream >> label >> user >> nice >> system >> idleVal))
        {
            continue;
        }
        lineStream >> iowait >> irq >> softirq >> steal;

        CpuTicks t;
        t.idle = idleVal + iowait;
        t.total = user + nice + system + t.idle + irq + softirq + steal;
        out.push_back(t);
    }
    return !out.empty();
}

bool parseProcStatusVmRssKb(const std::string &content,
                            unsigned long long &kbOut)
{
    kbOut = 0;
    if (content.empty())
    {
        return false;
    }

    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line))
    {
        if (line.rfind("VmRSS:", 0) == 0)
        {
            std::istringstream lineStream(line.substr(6));
            unsigned long long kb = 0;
            if (lineStream >> kb)
            {
                kbOut = kb;
                return true;
            }
            return false;
        }
    }
    return false;
}

}
