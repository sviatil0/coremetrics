#include <iostream>
#include "SparklineTest.hpp"
#include "Sparkline.hpp"
#include "screen.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

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

    Sparkline makeDefaultSparkline(std::size_t capacity)
    {
        return Sparkline(ivec2(0, 0), ivec2(100, 40),
                         vec3(0.0f, 1.0f, 0.0f),
                         0.0f, 100.0f, capacity);
    }
}

static void testConstructionStoresGeometryAndCapacity()
{
    Sparkline sp = makeDefaultSparkline(64);
    bool passed = sp.getCapacity() == 64
                  && sp.getSize() == 0
                  && sp.getMinPos().x == 0
                  && sp.getMinPos().y == 0
                  && sp.getMaxPos().x == 100
                  && sp.getMaxPos().y == 40
                  && sp.getThresholdMode() == false
                  && sp.getAutoScale() == false;
    report("construction stores geometry, capacity, defaults", passed);
}

static void testFirstPushFillsBuffer()
{
    Sparkline sp = makeDefaultSparkline(8);
    sp.push(42.0f);
    bool passed = sp.getSize() == sp.getCapacity()
                  && sp.getSize() == 8;
    report("first push fills the ring buffer to capacity", passed);
}

static void testSubsequentPushesDoNotGrowBeyondCapacity()
{
    Sparkline sp = makeDefaultSparkline(4);
    sp.push(10.0f);
    sp.push(20.0f);
    sp.push(30.0f);
    sp.push(40.0f);
    sp.push(50.0f);
    bool passed = sp.getSize() == 4
                  && sp.getCapacity() == 4;
    report("subsequent pushes rotate without exceeding capacity", passed);
}

static void testClearEmptiesBuffer()
{
    Sparkline sp = makeDefaultSparkline(8);
    sp.push(25.0f);
    sp.push(75.0f);
    sp.clear();
    bool passed = sp.getSize() == 0
                  && sp.getCapacity() == 8;
    report("clear empties samples but preserves capacity", passed);
}

static void testRefillAfterClear()
{
    Sparkline sp = makeDefaultSparkline(6);
    sp.push(10.0f);
    sp.clear();
    sp.push(80.0f);
    bool passed = sp.getSize() == 6;
    report("first push after clear refills the buffer", passed);
}

static void testThresholdModeRoundTrip()
{
    Sparkline sp = makeDefaultSparkline(16);
    bool startsOff = sp.getThresholdMode() == false;
    sp.setThresholdMode(true);
    bool flippedOn = sp.getThresholdMode() == true;
    sp.setThresholdMode(false);
    bool flippedOff = sp.getThresholdMode() == false;
    report("setThresholdMode round-trips through the getter",
           startsOff && flippedOn && flippedOff);
}

static void testAutoScaleRoundTrip()
{
    Sparkline sp = makeDefaultSparkline(16);
    bool startsOff = sp.getAutoScale() == false;
    sp.setAutoScale(true);
    bool flippedOn = sp.getAutoScale() == true;
    sp.setAutoScale(false);
    bool flippedOff = sp.getAutoScale() == false;
    report("setAutoScale round-trips through the getter",
           startsOff && flippedOn && flippedOff);
}

static void testAutoScaleAcceptsOverCeilingSamples()
{
    Sparkline sp = makeDefaultSparkline(8);
    sp.setAutoScale(true);
    sp.push(50.0f);
    sp.push(250.0f);
    sp.push(1000.0f);
    bool passed = sp.getSize() == 8
                  && sp.getAutoScale() == true;
    report("autoScale on accepts samples past initial ceiling", passed);
}

static void testDrawEmptyIsNoop()
{
    Screen screen(100, 40);
    Sparkline sp = makeDefaultSparkline(32);
    sp.draw(screen);
    report("draw with no samples does not crash", true);
}

static void testDrawWithSamplesDoesNotCrash()
{
    Screen screen(100, 40);
    Sparkline sp = makeDefaultSparkline(32);
    sp.push(10.0f);
    sp.push(50.0f);
    sp.push(90.0f);
    sp.draw(screen);
    report("draw with samples does not crash", true);
}

static void testDrawWithThresholdModeDoesNotCrash()
{
    Screen screen(100, 40);
    Sparkline sp = makeDefaultSparkline(32);
    sp.setThresholdMode(true);
    sp.push(15.0f);
    sp.push(65.0f);
    sp.push(85.0f);
    sp.draw(screen);
    report("draw with threshold mode does not crash", true);
}

void sparklineTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Sparkline : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    g_failures = 0;

    testConstructionStoresGeometryAndCapacity();
    testFirstPushFillsBuffer();
    testSubsequentPushesDoNotGrowBeyondCapacity();
    testClearEmptiesBuffer();
    testRefillAfterClear();
    testThresholdModeRoundTrip();
    testAutoScaleRoundTrip();
    testAutoScaleAcceptsOverCeilingSamples();
    testDrawEmptyIsNoop();
    testDrawWithSamplesDoesNotCrash();
    testDrawWithThresholdModeDoesNotCrash();

    std::cout << '\n';
    std::cout << "  Failures: " << g_failures << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Sparkline tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
