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
