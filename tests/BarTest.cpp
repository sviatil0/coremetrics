#include <iostream>
#include "BarTest.hpp"
#include "Bar.hpp"
#include "screen.hpp"

void testBarConstruction()
{
    std::cout << "Bar : construction initializes value to minVal: ";
    Bar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 0.0f, 100.0f, "cpu");
    bool passed = (bar.getValue() == 0.0f && bar.getMetricName() == "cpu");
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testBarSetValueInRange()
{
    std::cout << "Bar : setValue in range: ";
    Bar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 0.0f, 100.0f);
    bar.setValue(50.0f);
    bool passed = (bar.getValue() == 50.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testBarSetValueClampLow()
{
    std::cout << "Bar : setValue clamps below minVal: ";
    Bar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 0.0f, 100.0f);
    bar.setValue(-10.0f);
    bool passed = (bar.getValue() == 0.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testBarSetValueClampHigh()
{
    std::cout << "Bar : setValue clamps above maxVal: ";
    Bar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 0.0f, 100.0f);
    bar.setValue(150.0f);
    bool passed = (bar.getValue() == 100.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testBarDrawDoesNotCrash()
{
    std::cout << "Bar : draw does not crash: ";
    Screen screen(128, 64);
    Bar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 0.0f, 100.0f);
    bar.setValue(42.0f);
    bar.draw(screen);
    std::cout << "PASS" << '\n';
}

void testBarDrawZeroRange()
{
    std::cout << "Bar : draw handles zero range: ";
    Screen screen(128, 64);
    Bar bar(ivec2(0, 0), ivec2(100, 20), vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 50.0f, 50.0f);
    bar.draw(screen);
    std::cout << "PASS" << '\n';
}

void testBarGetters()
{
    std::cout << "Bar : getMinPos/getMaxPos return stored coords: ";
    Bar bar(ivec2(10, 20), ivec2(110, 40), vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 0.0f, 100.0f);
    ivec2 minP = bar.getMinPos();
    ivec2 maxP = bar.getMaxPos();
    bool passed = (minP.x == 10 && minP.y == 20 && maxP.x == 110 && maxP.y == 40);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void barTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Bar : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testBarConstruction();
    testBarSetValueInRange();
    testBarSetValueClampLow();
    testBarSetValueClampHigh();
    testBarDrawDoesNotCrash();
    testBarDrawZeroRange();
    testBarGetters();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Bar tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
