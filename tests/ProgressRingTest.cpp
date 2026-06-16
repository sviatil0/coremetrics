#include <iostream>
#include "ProgressRingTest.hpp"
#include "ProgressRing.hpp"
#include "screen.hpp"

void testProgressRingConstruction()
{
    std::cout << "ProgressRing : construction initializes value to minValue: ";
    ProgressRing ring(ivec2(50, 50), 30, 20,
                      vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f),
                      0.0f, 100.0f);
    ivec2 c = ring.getCenter();
    bool passed = (ring.getValue() == 0.0f
                   && ring.getOuterRadius() == 30
                   && ring.getInnerRadius() == 20
                   && c.x == 50
                   && c.y == 50);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testProgressRingSetValueInRange()
{
    std::cout << "ProgressRing : setValue in range: ";
    ProgressRing ring(ivec2(50, 50), 30, 20,
                      vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f),
                      0.0f, 100.0f);
    ring.setValue(42.0f);
    bool passed = (ring.getValue() == 42.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testProgressRingSetValueClampLow()
{
    std::cout << "ProgressRing : setValue clamps below minValue: ";
    ProgressRing ring(ivec2(50, 50), 30, 20,
                      vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f),
                      0.0f, 100.0f);
    ring.setValue(-25.0f);
    bool passed = (ring.getValue() == 0.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testProgressRingSetValueClampHigh()
{
    std::cout << "ProgressRing : setValue clamps above maxValue: ";
    ProgressRing ring(ivec2(50, 50), 30, 20,
                      vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f),
                      0.0f, 100.0f);
    ring.setValue(250.0f);
    bool passed = (ring.getValue() == 100.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testProgressRingDrawSmoke()
{
    std::cout << "ProgressRing : draw on 100x100 screen does not crash: ";
    Screen screen(100, 100);
    ProgressRing ring(ivec2(50, 50), 30, 20,
                      vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f),
                      0.0f, 100.0f);
    ring.setValue(75.0f);
    ring.draw(screen);
    std::cout << "PASS" << '\n';
}

void testProgressRingDrawZeroValue()
{
    std::cout << "ProgressRing : draw at zero value renders only the track: ";
    Screen screen(100, 100);
    ProgressRing ring(ivec2(50, 50), 30, 20,
                      vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f),
                      0.0f, 100.0f);
    ring.draw(screen);
    std::cout << "PASS" << '\n';
}

void testProgressRingDrawFullValue()
{
    std::cout << "ProgressRing : draw at full value renders only the ring: ";
    Screen screen(100, 100);
    ProgressRing ring(ivec2(50, 50), 30, 20,
                      vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f),
                      0.0f, 100.0f);
    ring.setValue(100.0f);
    ring.draw(screen);
    std::cout << "PASS" << '\n';
}

void testProgressRingDrawDegenerateGeometry()
{
    std::cout << "ProgressRing : draw with innerRadius >= outerRadius is a no-op: ";
    Screen screen(100, 100);
    ProgressRing ring(ivec2(50, 50), 20, 30,
                      vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f),
                      0.0f, 100.0f);
    ring.setValue(50.0f);
    ring.draw(screen);
    std::cout << "PASS" << '\n';
}

void testProgressRingSetColors()
{
    std::cout << "ProgressRing : setRingColor and setTrackColor do not crash on redraw: ";
    Screen screen(100, 100);
    ProgressRing ring(ivec2(50, 50), 30, 20,
                      vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f),
                      0.0f, 100.0f);
    ring.setRingColor(vec3(1.0f, 0.5f, 0.0f));
    ring.setTrackColor(vec3(0.2f, 0.2f, 0.2f));
    ring.setValue(33.0f);
    ring.draw(screen);
    std::cout << "PASS" << '\n';
}

void progressRingTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  ProgressRing : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testProgressRingConstruction();
    testProgressRingSetValueInRange();
    testProgressRingSetValueClampLow();
    testProgressRingSetValueClampHigh();
    testProgressRingDrawSmoke();
    testProgressRingDrawZeroValue();
    testProgressRingDrawFullValue();
    testProgressRingDrawDegenerateGeometry();
    testProgressRingSetColors();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  ProgressRing tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
