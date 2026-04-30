#ifndef __PROCESS_UTILS_HPP__
#define __PROCESS_UTILS_HPP__

#include "SystemMetrics.hpp"
#include <string>
#include <sstream>

enum SortColumn
{
    SORT_PID = 0,
    SORT_NAME = 1,
    SORT_CPU = 2,
    SORT_MEM = 3
};

std::string formatPct(float value);

#endif
