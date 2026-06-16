#include <iostream>
#include "ToggleTest.hpp"
#include "Toggle.hpp"
#include "ClickEvent.hpp"
#include "screen.hpp"

void testToggleConstructionDefaults()
{
    std::cout << "Toggle : construction stores initial state and colors: ";
    Toggle toggle(ivec2(0, 0), ivec2(40, 20), false,
                  vec3(0.0f, 1.0f, 0.0f), vec3(0.3f, 0.3f, 0.3f), vec3(1.0f, 1.0f, 1.0f));
    bool passed = (toggle.getState() == false);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testToggleConstructionInitialOn()
{
    std::cout << "Toggle : construction with initialState=true reports on: ";
    Toggle toggle(ivec2(0, 0), ivec2(40, 20), true,
                  vec3(0.0f, 1.0f, 0.0f), vec3(0.3f, 0.3f, 0.3f), vec3(1.0f, 1.0f, 1.0f));
    bool passed = (toggle.getState() == true);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testToggleSetStateRoundTrip()
{
    std::cout << "Toggle : setState round-trip: ";
    Toggle toggle(ivec2(0, 0), ivec2(40, 20), false,
                  vec3(0.0f, 1.0f, 0.0f), vec3(0.3f, 0.3f, 0.3f), vec3(1.0f, 1.0f, 1.0f));
    toggle.setState(true);
    bool firstOn = toggle.getState();
    toggle.setState(false);
    bool thenOff = !toggle.getState();
    bool passed = firstOn && thenOff;
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testToggleClickFlipsState()
{
    std::cout << "Toggle : click inside bounds flips state and is handled: ";
    Toggle toggle(ivec2(10, 10), ivec2(50, 30), false,
                  vec3(0.0f, 1.0f, 0.0f), vec3(0.3f, 0.3f, 0.3f), vec3(1.0f, 1.0f, 1.0f));
    ClickEvent click(20, 20);
    bool handled = toggle(&click);
    bool passed = handled && (toggle.getState() == true);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testToggleClickTwiceReturnsToOff()
{
    std::cout << "Toggle : two clicks return state to off: ";
    Toggle toggle(ivec2(10, 10), ivec2(50, 30), false,
                  vec3(0.0f, 1.0f, 0.0f), vec3(0.3f, 0.3f, 0.3f), vec3(1.0f, 1.0f, 1.0f));
    ClickEvent click(20, 20);
    toggle(&click);
    toggle(&click);
    bool passed = (toggle.getState() == false);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testToggleClickOutsideIgnored()
{
    std::cout << "Toggle : click outside bounds is ignored: ";
    Toggle toggle(ivec2(10, 10), ivec2(50, 30), false,
                  vec3(0.0f, 1.0f, 0.0f), vec3(0.3f, 0.3f, 0.3f), vec3(1.0f, 1.0f, 1.0f));
    ClickEvent click(5, 5);
    bool handled = toggle(&click);
    bool passed = (handled == false) && (toggle.getState() == false);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testToggleDrawDoesNotCrashOff()
{
    std::cout << "Toggle : draw off does not crash: ";
    Screen screen(128, 64);
    Toggle toggle(ivec2(4, 4), ivec2(48, 24), false,
                  vec3(0.0f, 1.0f, 0.0f), vec3(0.3f, 0.3f, 0.3f), vec3(1.0f, 1.0f, 1.0f));
    toggle.draw(screen);
    std::cout << "PASS" << '\n';
}

void testToggleDrawDoesNotCrashOn()
{
    std::cout << "Toggle : draw on does not crash: ";
    Screen screen(128, 64);
    Toggle toggle(ivec2(4, 4), ivec2(48, 24), true,
                  vec3(0.0f, 1.0f, 0.0f), vec3(0.3f, 0.3f, 0.3f), vec3(1.0f, 1.0f, 1.0f));
    toggle.draw(screen);
    std::cout << "PASS" << '\n';
}

void toggleTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Toggle : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testToggleConstructionDefaults();
    testToggleConstructionInitialOn();
    testToggleSetStateRoundTrip();
    testToggleClickFlipsState();
    testToggleClickTwiceReturnsToOff();
    testToggleClickOutsideIgnored();
    testToggleDrawDoesNotCrashOff();
    testToggleDrawDoesNotCrashOn();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Toggle tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
