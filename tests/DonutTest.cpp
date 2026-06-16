#include <iostream>
#include "DonutTest.hpp"
#include "Donut.hpp"
#include "screen.hpp"

void testDonutConstruction()
{
    std::cout << "Donut : construction initializes value to minVal and label empty: ";
    Donut donut(ivec2(64, 64), 30, 18,
                vec3(0.2f, 0.8f, 0.4f), vec3(0.1f, 0.1f, 0.1f), vec3(1.0f, 1.0f, 1.0f),
                0.0f, 100.0f);
    bool passed = (donut.getValue() == 0.0f
                   && donut.getLabel().empty()
                   && donut.getOuterRadius() == 30
                   && donut.getInnerRadius() == 18
                   && donut.getCenter().x == 64
                   && donut.getCenter().y == 64);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testDonutSetValueInRange()
{
    std::cout << "Donut : setValue in range: ";
    Donut donut(ivec2(64, 64), 30, 18,
                vec3(0.2f, 0.8f, 0.4f), vec3(0.1f, 0.1f, 0.1f), vec3(1.0f, 1.0f, 1.0f),
                0.0f, 100.0f);
    donut.setValue(73.0f);
    bool passed = (donut.getValue() == 73.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testDonutSetValueClampLow()
{
    std::cout << "Donut : setValue clamps below minVal: ";
    Donut donut(ivec2(64, 64), 30, 18,
                vec3(0.2f, 0.8f, 0.4f), vec3(0.1f, 0.1f, 0.1f), vec3(1.0f, 1.0f, 1.0f),
                0.0f, 100.0f);
    donut.setValue(-25.0f);
    bool passed = (donut.getValue() == 0.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testDonutSetValueClampHigh()
{
    std::cout << "Donut : setValue clamps above maxVal: ";
    Donut donut(ivec2(64, 64), 30, 18,
                vec3(0.2f, 0.8f, 0.4f), vec3(0.1f, 0.1f, 0.1f), vec3(1.0f, 1.0f, 1.0f),
                0.0f, 100.0f);
    donut.setValue(250.0f);
    bool passed = (donut.getValue() == 100.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testDonutSetLabel()
{
    std::cout << "Donut : setLabel stores override text: ";
    Donut donut(ivec2(64, 64), 30, 18,
                vec3(0.2f, 0.8f, 0.4f), vec3(0.1f, 0.1f, 0.1f), vec3(1.0f, 1.0f, 1.0f),
                0.0f, 100.0f);
    donut.setLabel("CPU");
    bool passed = (donut.getLabel() == "CPU");
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testDonutDrawDoesNotCrash()
{
    std::cout << "Donut : draw does not crash: ";
    Screen screen(160, 120);
    Donut donut(ivec2(80, 60), 30, 18,
                vec3(0.2f, 0.8f, 0.4f), vec3(0.1f, 0.1f, 0.1f), vec3(1.0f, 1.0f, 1.0f),
                0.0f, 100.0f);
    donut.setValue(42.0f);
    donut.draw(screen);
    std::cout << "PASS" << '\n';
}

void testDonutDrawZeroRange()
{
    std::cout << "Donut : draw handles zero range: ";
    Screen screen(160, 120);
    Donut donut(ivec2(80, 60), 30, 18,
                vec3(0.2f, 0.8f, 0.4f), vec3(0.1f, 0.1f, 0.1f), vec3(1.0f, 1.0f, 1.0f),
                50.0f, 50.0f);
    donut.draw(screen);
    std::cout << "PASS" << '\n';
}

void testDonutDrawCustomLabel()
{
    std::cout << "Donut : draw renders custom centerLabel: ";
    Screen screen(160, 120);
    Donut donut(ivec2(80, 60), 30, 18,
                vec3(0.2f, 0.8f, 0.4f), vec3(0.1f, 0.1f, 0.1f), vec3(1.0f, 1.0f, 1.0f),
                0.0f, 100.0f);
    donut.setLabel("RAM");
    donut.setValue(80.0f);
    donut.draw(screen);
    std::cout << "PASS" << '\n';
}

void donutTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Donut : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testDonutConstruction();
    testDonutSetValueInRange();
    testDonutSetValueClampLow();
    testDonutSetValueClampHigh();
    testDonutSetLabel();
    testDonutDrawDoesNotCrash();
    testDonutDrawZeroRange();
    testDonutDrawCustomLabel();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Donut tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
