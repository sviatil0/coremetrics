#include <iostream>
#include "GaugeTest.hpp"
#include "Gauge.hpp"
#include "screen.hpp"

void testGaugeConstruction()
{
    std::cout << "Gauge : construction initializes value to minValue, retains center/radius/thickness: ";
    Gauge gauge(ivec2(64, 64), 40, 6,
                vec3(0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec3(0.1f, 0.1f, 0.1f),
                0.0f, 100.0f);
    bool passed = (gauge.getValue() == 0.0f
                && gauge.getCenter().x == 64
                && gauge.getCenter().y == 64
                && gauge.getRadius() == 40
                && gauge.getArcThickness() == 6);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testGaugeSetValueInRange()
{
    std::cout << "Gauge : setValue in range: ";
    Gauge gauge(ivec2(64, 64), 40, 6,
                vec3(0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec3(0.1f, 0.1f, 0.1f),
                0.0f, 100.0f);
    gauge.setValue(75.0f);
    bool passed = (gauge.getValue() == 75.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testGaugeSetValueClampLow()
{
    std::cout << "Gauge : setValue clamps below minValue: ";
    Gauge gauge(ivec2(64, 64), 40, 6,
                vec3(0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec3(0.1f, 0.1f, 0.1f),
                0.0f, 100.0f);
    gauge.setValue(-25.0f);
    bool passed = (gauge.getValue() == 0.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testGaugeSetValueClampHigh()
{
    std::cout << "Gauge : setValue clamps above maxValue: ";
    Gauge gauge(ivec2(64, 64), 40, 6,
                vec3(0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec3(0.1f, 0.1f, 0.1f),
                0.0f, 100.0f);
    gauge.setValue(500.0f);
    bool passed = (gauge.getValue() == 100.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testGaugeDrawSmoke()
{
    std::cout << "Gauge : draw does not crash at mid value: ";
    Screen screen(160, 100);
    Gauge gauge(ivec2(80, 80), 40, 6,
                vec3(0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec3(0.1f, 0.1f, 0.1f),
                0.0f, 100.0f);
    gauge.setValue(50.0f);
    gauge.draw(screen);
    std::cout << "PASS" << '\n';
}

void testGaugeDrawEndpoints()
{
    std::cout << "Gauge : draw does not crash at min and max values: ";
    Screen screen(160, 100);
    Gauge gauge(ivec2(80, 80), 40, 6,
                vec3(0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec3(0.1f, 0.1f, 0.1f),
                0.0f, 100.0f);
    gauge.setValue(0.0f);
    gauge.draw(screen);
    gauge.setValue(100.0f);
    gauge.draw(screen);
    std::cout << "PASS" << '\n';
}

void testGaugeDrawZeroRange()
{
    std::cout << "Gauge : draw handles zero range without crashing: ";
    Screen screen(160, 100);
    Gauge gauge(ivec2(80, 80), 40, 6,
                vec3(0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec3(0.1f, 0.1f, 0.1f),
                50.0f, 50.0f);
    gauge.draw(screen);
    std::cout << "PASS" << '\n';
}

void gaugeTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Gauge : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testGaugeConstruction();
    testGaugeSetValueInRange();
    testGaugeSetValueClampLow();
    testGaugeSetValueClampHigh();
    testGaugeDrawSmoke();
    testGaugeDrawEndpoints();
    testGaugeDrawZeroRange();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Gauge tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
