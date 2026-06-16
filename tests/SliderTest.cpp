#include <iostream>
#include "SliderTest.hpp"
#include "Slider.hpp"
#include "ClickEvent.hpp"
#include "screen.hpp"

void testSliderConstruction()
{
    std::cout << "Slider : construction initializes value to minVal: ";
    Slider slider(ivec2(0, 0), ivec2(100, 10),
                  vec3(0.2f, 0.2f, 0.2f),
                  vec3(0.0f, 0.6f, 0.0f),
                  vec3(1.0f, 1.0f, 1.0f),
                  0.0f, 1.0f, 8);
    bool passed = (slider.getValue() == 0.0f
                   && slider.getMinVal() == 0.0f
                   && slider.getMaxVal() == 1.0f
                   && slider.getHandleWidth() == 8);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testSliderSetValueInRange()
{
    std::cout << "Slider : setValue stores value in range: ";
    Slider slider(ivec2(0, 0), ivec2(100, 10),
                  vec3(0.2f, 0.2f, 0.2f),
                  vec3(0.0f, 0.6f, 0.0f),
                  vec3(1.0f, 1.0f, 1.0f),
                  0.0f, 100.0f);
    slider.setValue(42.5f);
    bool passed = (slider.getValue() == 42.5f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testSliderSetValueClampLow()
{
    std::cout << "Slider : setValue clamps below minVal: ";
    Slider slider(ivec2(0, 0), ivec2(100, 10),
                  vec3(0.2f, 0.2f, 0.2f),
                  vec3(0.0f, 0.6f, 0.0f),
                  vec3(1.0f, 1.0f, 1.0f),
                  0.0f, 100.0f);
    slider.setValue(-5.0f);
    bool passed = (slider.getValue() == 0.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testSliderSetValueClampHigh()
{
    std::cout << "Slider : setValue clamps above maxVal: ";
    Slider slider(ivec2(0, 0), ivec2(100, 10),
                  vec3(0.2f, 0.2f, 0.2f),
                  vec3(0.0f, 0.6f, 0.0f),
                  vec3(1.0f, 1.0f, 1.0f),
                  0.0f, 100.0f);
    slider.setValue(150.0f);
    bool passed = (slider.getValue() == 100.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testSliderDrawDoesNotCrash()
{
    std::cout << "Slider : draw does not crash: ";
    Screen screen(256, 64);
    Slider slider(ivec2(10, 20), ivec2(110, 30),
                  vec3(0.2f, 0.2f, 0.2f),
                  vec3(0.0f, 0.6f, 0.0f),
                  vec3(1.0f, 1.0f, 1.0f),
                  0.0f, 100.0f);
    slider.setValue(50.0f);
    slider.draw(screen);
    std::cout << "PASS" << '\n';
}

void testSliderDrawZeroRange()
{
    std::cout << "Slider : draw handles zero range: ";
    Screen screen(256, 64);
    Slider slider(ivec2(10, 20), ivec2(110, 30),
                  vec3(0.2f, 0.2f, 0.2f),
                  vec3(0.0f, 0.6f, 0.0f),
                  vec3(1.0f, 1.0f, 1.0f),
                  50.0f, 50.0f);
    slider.draw(screen);
    std::cout << "PASS" << '\n';
}

void testSliderClickSetsValue()
{
    std::cout << "Slider : click in track sets value to mapped x: ";
    Slider slider(ivec2(0, 0), ivec2(100, 10),
                  vec3(0.2f, 0.2f, 0.2f),
                  vec3(0.0f, 0.6f, 0.0f),
                  vec3(1.0f, 1.0f, 1.0f),
                  0.0f, 100.0f);
    ClickEvent click(50, 5);
    bool handled = slider(&click);
    bool passed = (handled && slider.getValue() >= 49.0f && slider.getValue() <= 51.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testSliderClickOutsideIgnored()
{
    std::cout << "Slider : click outside track does not change value: ";
    Slider slider(ivec2(0, 0), ivec2(100, 10),
                  vec3(0.2f, 0.2f, 0.2f),
                  vec3(0.0f, 0.6f, 0.0f),
                  vec3(1.0f, 1.0f, 1.0f),
                  0.0f, 100.0f);
    slider.setValue(25.0f);
    ClickEvent click(500, 500);
    bool handled = slider(&click);
    bool passed = (!handled && slider.getValue() == 25.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testSliderGetters()
{
    std::cout << "Slider : getMinPos/getMaxPos return stored coords: ";
    Slider slider(ivec2(10, 20), ivec2(110, 30),
                  vec3(0.2f, 0.2f, 0.2f),
                  vec3(0.0f, 0.6f, 0.0f),
                  vec3(1.0f, 1.0f, 1.0f),
                  0.0f, 1.0f);
    ivec2 minP = slider.getMinPos();
    ivec2 maxP = slider.getMaxPos();
    bool passed = (minP.x == 10 && minP.y == 20 && maxP.x == 110 && maxP.y == 30);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void sliderTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Slider : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testSliderConstruction();
    testSliderSetValueInRange();
    testSliderSetValueClampLow();
    testSliderSetValueClampHigh();
    testSliderDrawDoesNotCrash();
    testSliderDrawZeroRange();
    testSliderClickSetsValue();
    testSliderClickOutsideIgnored();
    testSliderGetters();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Slider tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
