#include <iostream>
#include "RingBufferTest.hpp"
#include "RingBuffer.hpp"

namespace
{
    int g_failures = 0;

    void report(const char *label, bool passed)
    {
        std::cout << "  " << label << ": " << (passed ? "PASS" : "FAIL") << '\n';
        if (!passed)
        {
            ++g_failures;
        }
    }
}

static void testEmptyOnConstruction()
{
    RingBuffer<float> buf(4);
    bool passed = buf.isEmpty()
                  && !buf.isFull()
                  && buf.getSize() == 0
                  && buf.getCapacity() == 4;
    report("empty on construction", passed);
}

static void testZeroCapacityClampsToOne()
{
    RingBuffer<int> buf(0);
    report("zero capacity is clamped to 1", buf.getCapacity() == 1);
}

static void testPushUntilFull()
{
    RingBuffer<int> buf(3);
    buf.push(10);
    buf.push(20);
    buf.push(30);
    bool passed = buf.isFull()
                  && buf.getSize() == 3
                  && buf.oldest() == 10
                  && buf.newest() == 30
                  && buf.at(0) == 10
                  && buf.at(1) == 20
                  && buf.at(2) == 30;
    report("push fills the buffer in order", passed);
}

static void testWrapDropsOldest()
{
    RingBuffer<int> buf(3);
    buf.push(10);
    buf.push(20);
    buf.push(30);
    buf.push(40);
    buf.push(50);
    bool passed = buf.isFull()
                  && buf.getSize() == 3
                  && buf.oldest() == 30
                  && buf.newest() == 50
                  && buf.at(0) == 30
                  && buf.at(1) == 40
                  && buf.at(2) == 50;
    report("wrap drops the oldest entry", passed);
}

static void testAtOutOfRangeReturnsDefault()
{
    RingBuffer<int> buf(4);
    buf.push(7);
    buf.push(8);
    report("at(out-of-range) returns default", buf.at(99) == 0);
}

static void testNewestOldestEmpty()
{
    RingBuffer<int> buf(4);
    report("newest on empty returns default", buf.newest() == 0);
    report("oldest on empty returns default", buf.oldest() == 0);
}

static void testClearResets()
{
    RingBuffer<int> buf(3);
    buf.push(1);
    buf.push(2);
    buf.clear();
    bool passed = buf.isEmpty()
                  && buf.getSize() == 0
                  && !buf.isFull();
    report("clear resets the buffer", passed);
}

static void testManyWraps()
{
    RingBuffer<int> buf(5);
    for (int i = 0; i < 1000; ++i)
    {
        buf.push(i);
    }
    bool passed = buf.isFull()
                  && buf.newest() == 999
                  && buf.oldest() == 995
                  && buf.at(0) == 995
                  && buf.at(4) == 999;
    report("after many wraps, window stays correct", passed);
}

void ringBufferTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  RingBuffer : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    g_failures = 0;

    testEmptyOnConstruction();
    testZeroCapacityClampsToOne();
    testPushUntilFull();
    testWrapDropsOldest();
    testAtOutOfRangeReturnsDefault();
    testNewestOldestEmpty();
    testClearResets();
    testManyWraps();

    std::cout << '\n';
    std::cout << "  Failures: " << g_failures << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  RingBuffer tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
