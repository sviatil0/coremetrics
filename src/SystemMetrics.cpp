#include "SystemMetrics.hpp"
#include <algorithm>

bool systemMetricsCompareByMemDesc(const ProcessInfo &a, const ProcessInfo &b)
{
    return a.memPct > b.memPct;
}
