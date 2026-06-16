#include <iostream>
#include "TabStripTest.hpp"
#include "TabStrip.hpp"
#include "ClickEvent.hpp"
#include "screen.hpp"

static TabStrip makeThreeTabStrip()
{
    TabStrip strip(ivec2(0, 0), ivec2(300, 24),
                   vec3(0.15f, 0.15f, 0.15f),
                   vec3(0.30f, 0.55f, 0.85f),
                   vec3(1.0f, 1.0f, 1.0f),
                   vec3(0.05f, 0.05f, 0.05f));
    strip.addTab("CPU");
    strip.addTab("RAM");
    strip.addTab("GPU");
    return strip;
}

void testTabStripConstructionDefaults()
{
    std::cout << "TabStrip : default selectedIndex is 0 with no tabs: ";
    TabStrip strip(ivec2(0, 0), ivec2(100, 20),
                   vec3(0.1f, 0.1f, 0.1f),
                   vec3(0.2f, 0.4f, 0.8f),
                   vec3(1.0f, 1.0f, 1.0f),
                   vec3(0.0f, 0.0f, 0.0f));
    bool passed = (strip.getSelected() == 0u && strip.getTabCount() == 0u);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTabStripAddTabAndSetSelected()
{
    std::cout << "TabStrip : addTab + setSelected updates index: ";
    TabStrip strip = makeThreeTabStrip();
    strip.setSelected(1);
    bool sel1 = (strip.getSelected() == 1u);
    strip.setSelected(2);
    bool sel2 = (strip.getSelected() == 2u);
    bool count = (strip.getTabCount() == 3u);
    std::cout << ((sel1 && sel2 && count) ? "PASS" : "FAIL") << '\n';
}

void testTabStripSetSelectedOutOfRangeIgnored()
{
    std::cout << "TabStrip : setSelected out of range is ignored: ";
    TabStrip strip = makeThreeTabStrip();
    strip.setSelected(1);
    strip.setSelected(99);
    bool passed = (strip.getSelected() == 1u);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTabStripClickSelectsTabTwo()
{
    std::cout << "TabStrip : click on tab 2 sets selectedIndex to 2: ";
    TabStrip strip = makeThreeTabStrip();
    // Strip spans x in [0, 300] over 3 tabs => tab width 100, so tab 2
    // covers x in [200, 300). x = 250, y = 10 lands squarely inside.
    ClickEvent click(250, 10);
    bool consumed = strip(&click);
    bool selected = (strip.getSelected() == 2u);
    std::cout << ((consumed && selected) ? "PASS" : "FAIL") << '\n';
}

void testTabStripClickOutsideIgnored()
{
    std::cout << "TabStrip : click outside the strip is ignored: ";
    TabStrip strip = makeThreeTabStrip();
    strip.setSelected(1);
    ClickEvent click(500, 500);
    bool consumed = strip(&click);
    bool unchanged = (strip.getSelected() == 1u);
    std::cout << ((!consumed && unchanged) ? "PASS" : "FAIL") << '\n';
}

void testTabStripDrawSmoke()
{
    std::cout << "TabStrip : draw does not crash: ";
    Screen screen(320, 64);
    TabStrip strip = makeThreeTabStrip();
    strip.setSelected(1);
    strip.draw(screen);
    std::cout << "PASS" << '\n';
}

void testTabStripDrawEmptyIsSafe()
{
    std::cout << "TabStrip : draw with no tabs is safe: ";
    Screen screen(320, 64);
    TabStrip strip(ivec2(0, 0), ivec2(300, 24),
                   vec3(0.1f, 0.1f, 0.1f),
                   vec3(0.2f, 0.4f, 0.8f),
                   vec3(1.0f, 1.0f, 1.0f),
                   vec3(0.0f, 0.0f, 0.0f));
    strip.draw(screen);
    std::cout << "PASS" << '\n';
}

void tabStripTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  TabStrip : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testTabStripConstructionDefaults();
    testTabStripAddTabAndSetSelected();
    testTabStripSetSelectedOutOfRangeIgnored();
    testTabStripClickSelectsTabTwo();
    testTabStripClickOutsideIgnored();
    testTabStripDrawSmoke();
    testTabStripDrawEmptyIsSafe();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  TabStrip tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
