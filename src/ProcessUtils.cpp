#include "ProcessUtils.hpp"

std::string formatPct(float value)
{
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed << value;
    return oss.str();
}
