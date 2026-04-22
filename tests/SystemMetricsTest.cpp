#include <iostream>
#include "SystemMetricsTest.hpp"
#include "SystemMetrics.hpp"

void testCpuPercentInRange()
{
    std::cout << "SystemMetrics : readCpuPercent returns [0, 100]: ";
    float first = SystemMetrics::readCpuPercent();
    float second = SystemMetrics::readCpuPercent();
    bool passed = (first >= 0.0f && first <= 100.0f && second >= 0.0f && second <= 100.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testMemPercentInRange()
{
    std::cout << "SystemMetrics : readMemPercent returns [0, 100]: ";
    float pct = SystemMetrics::readMemPercent();
    bool passed = (pct >= 0.0f && pct <= 100.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTopProcessesReturnsSomething()
{
    std::cout << "SystemMetrics : topProcesses returns non-empty: ";
    std::vector<ProcessInfo> list = SystemMetrics::topProcesses(5);
    bool passed = !list.empty();
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTopProcessesRespectsLimit()
{
    std::cout << "SystemMetrics : topProcesses respects n: ";
    std::vector<ProcessInfo> list = SystemMetrics::topProcesses(3);
    bool passed = (list.size() <= 3);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTopProcessesFieldsPopulated()
{
    std::cout << "SystemMetrics : topProcesses fields populated: ";
    std::vector<ProcessInfo> list = SystemMetrics::topProcesses(1);
    bool passed = true;
    if (list.empty())
    {
        passed = false;
    }
    else
    {
        const ProcessInfo &info = list.front();
        if (info.pid <= 0 || info.name.empty())
        {
            passed = false;
        }
    }
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void systemMetricsTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  SystemMetrics : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testCpuPercentInRange();
    testMemPercentInRange();
    testTopProcessesReturnsSomething();
    testTopProcessesRespectsLimit();
    testTopProcessesFieldsPopulated();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  SystemMetrics tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
