#include <iostream>
#include "StackedBarTest.hpp"
#include "StackedBar.hpp"
#include "screen.hpp"

void testStackedBarConstruction()
{
    std::cout << "StackedBar : construction defaults to no segments and stored total: ";
    StackedBar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.1f, 0.1f, 0.1f), 100.0f);
    bool passed = (bar.getSegmentCount() == 0 && bar.getTotal() == 100.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testStackedBarAddSegment()
{
    std::cout << "StackedBar : addSegment grows the segment list: ";
    StackedBar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.1f, 0.1f, 0.1f), 100.0f);
    bar.addSegment(25.0f, vec3(1.0f, 0.0f, 0.0f));
    bar.addSegment(40.0f, vec3(0.0f, 1.0f, 0.0f));
    bool passed = (bar.getSegmentCount() == 2);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testStackedBarClearSegments()
{
    std::cout << "StackedBar : clearSegments drops all stored segments: ";
    StackedBar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.1f, 0.1f, 0.1f), 100.0f);
    bar.addSegment(10.0f, vec3(1.0f, 0.0f, 0.0f));
    bar.addSegment(20.0f, vec3(0.0f, 1.0f, 0.0f));
    bar.clearSegments();
    bool passed = (bar.getSegmentCount() == 0);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testStackedBarSetTotal()
{
    std::cout << "StackedBar : setTotal updates the divisor: ";
    StackedBar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.1f, 0.1f, 0.1f), 100.0f);
    bar.setTotal(256.0f);
    bool passed = (bar.getTotal() == 256.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testStackedBarGetters()
{
    std::cout << "StackedBar : getMinPos/getMaxPos return stored coords: ";
    StackedBar bar(ivec2(10, 20), ivec2(110, 40), vec3(0.1f, 0.1f, 0.1f), 100.0f);
    ivec2 minP = bar.getMinPos();
    ivec2 maxP = bar.getMaxPos();
    bool passed = (minP.x == 10 && minP.y == 20 && maxP.x == 110 && maxP.y == 40);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testStackedBarDrawDoesNotCrash()
{
    std::cout << "StackedBar : draw does not crash: ";
    Screen screen(128, 64);
    StackedBar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.1f, 0.1f, 0.1f), 100.0f);
    bar.addSegment(30.0f, vec3(1.0f, 0.0f, 0.0f));
    bar.addSegment(20.0f, vec3(0.0f, 1.0f, 0.0f));
    bar.addSegment(10.0f, vec3(0.0f, 0.0f, 1.0f));
    bar.draw(screen);
    std::cout << "PASS" << '\n';
}

void testStackedBarDrawSkipsNonPositive()
{
    std::cout << "StackedBar : draw skips zero and negative segments: ";
    Screen screen(128, 64);
    StackedBar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.1f, 0.1f, 0.1f), 100.0f);
    bar.addSegment(0.0f, vec3(1.0f, 0.0f, 0.0f));
    bar.addSegment(-5.0f, vec3(0.0f, 1.0f, 0.0f));
    bar.addSegment(50.0f, vec3(0.0f, 0.0f, 1.0f));
    bar.draw(screen);
    std::cout << "PASS" << '\n';
}

void testStackedBarDrawZeroTotal()
{
    std::cout << "StackedBar : draw handles zero total: ";
    Screen screen(128, 64);
    StackedBar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.1f, 0.1f, 0.1f), 0.0f);
    bar.addSegment(10.0f, vec3(1.0f, 0.0f, 0.0f));
    bar.draw(screen);
    std::cout << "PASS" << '\n';
}

void stackedBarTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  StackedBar : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testStackedBarConstruction();
    testStackedBarAddSegment();
    testStackedBarClearSegments();
    testStackedBarSetTotal();
    testStackedBarGetters();
    testStackedBarDrawDoesNotCrash();
    testStackedBarDrawSkipsNonPositive();
    testStackedBarDrawZeroTotal();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  StackedBar tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
