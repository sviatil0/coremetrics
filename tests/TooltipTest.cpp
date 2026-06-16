#include <iostream>
#include "TooltipTest.hpp"
#include "Tooltip.hpp"
#include "screen.hpp"

void testTooltipConstructionDefaults()
{
    std::cout << "Tooltip : default construction has empty text and hidden: ";
    Tooltip tip;
    bool passed = (tip.text.empty() && tip.isVisible() == false && tip.padding == 4);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTooltipConstructionWithArgs()
{
    std::cout << "Tooltip : ctor with pos+text stores both: ";
    Tooltip tip(ivec2(10, 20), "hello");
    bool passed = (tip.text == "hello" && tip.pos.x == 10 && tip.pos.y == 20 && tip.isVisible() == false);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTooltipSetTextRoundTrip()
{
    std::cout << "Tooltip : setText round-trip: ";
    Tooltip tip;
    tip.setText("hover me");
    bool passed = (tip.text == "hover me");
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTooltipSetVisibleRoundTrip()
{
    std::cout << "Tooltip : setVisible round-trip: ";
    Tooltip tip;
    tip.setVisible(true);
    bool afterShow = tip.isVisible();
    tip.setVisible(false);
    bool afterHide = tip.isVisible();
    bool passed = (afterShow == true && afterHide == false);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTooltipSetPosRoundTrip()
{
    std::cout << "Tooltip : setPos round-trip: ";
    Tooltip tip;
    tip.setPos(ivec2(42, 17));
    bool passed = (tip.pos.x == 42 && tip.pos.y == 17);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTooltipDrawWhenHiddenIsNoCrash()
{
    std::cout << "Tooltip : draw while hidden does not crash: ";
    Screen screen(256, 128);
    Tooltip tip(ivec2(10, 10), "hidden");
    tip.setVisible(false);
    tip.draw(screen);
    std::cout << "PASS" << '\n';
}

void testTooltipDrawWhenVisibleIsNoCrash()
{
    std::cout << "Tooltip : draw while visible does not crash: ";
    Screen screen(256, 128);
    Tooltip tip(ivec2(10, 10), "shown");
    tip.setVisible(true);
    tip.draw(screen);
    std::cout << "PASS" << '\n';
}

void testTooltipDrawEmptyTextVisibleIsNoCrash()
{
    std::cout << "Tooltip : draw with empty text + visible does not crash: ";
    Screen screen(256, 128);
    Tooltip tip;
    tip.setPos(ivec2(5, 5));
    tip.setVisible(true);
    tip.draw(screen);
    std::cout << "PASS" << '\n';
}

void tooltipTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Tooltip : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testTooltipConstructionDefaults();
    testTooltipConstructionWithArgs();
    testTooltipSetTextRoundTrip();
    testTooltipSetVisibleRoundTrip();
    testTooltipSetPosRoundTrip();
    testTooltipDrawWhenHiddenIsNoCrash();
    testTooltipDrawWhenVisibleIsNoCrash();
    testTooltipDrawEmptyTextVisibleIsNoCrash();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Tooltip tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
