#include <iostream>
#include "DropdownTest.hpp"
#include "Dropdown.hpp"
#include "ClickEvent.hpp"
#include "screen.hpp"

// Unit coverage for the Dropdown widget. The widget owns three pieces of
// observable state: selectedIndex, expanded, and the options list. Each
// test exercises one of those slices in isolation so a regression points
// at a specific behavior. The draw smoke checks are there to catch
// crashes from out-of-range indexing or zero-sized strip rectangles --
// the asserts on pixels are intentionally left out because the goal is
// to keep the test independent of font metrics and palette tweaks.

void testDropdownConstruction()
{
    std::cout << "Dropdown : construction yields collapsed empty widget: ";
    Dropdown dd(ivec2(0, 0), ivec2(120, 20),
                vec3(0.1f, 0.1f, 0.1f),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.3f, 0.3f, 0.6f));
    bool passed = (dd.isExpanded() == false)
                  && (dd.getSelected() == 0)
                  && (dd.optionCount() == 0);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testDropdownAddOptionAndSetSelected()
{
    std::cout << "Dropdown : addOption + setSelected updates index: ";
    Dropdown dd(ivec2(0, 0), ivec2(120, 20),
                vec3(0.1f, 0.1f, 0.1f),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.3f, 0.3f, 0.6f));
    dd.addOption("alpha");
    dd.addOption("beta");
    dd.addOption("gamma");
    dd.setSelected(2);
    bool passed = (dd.optionCount() == 3) && (dd.getSelected() == 2);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testDropdownSetSelectedOutOfRangeIgnored()
{
    std::cout << "Dropdown : setSelected past end is rejected: ";
    Dropdown dd(ivec2(0, 0), ivec2(120, 20),
                vec3(0.1f, 0.1f, 0.1f),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.3f, 0.3f, 0.6f));
    dd.addOption("only");
    dd.setSelected(0);
    dd.setSelected(7);
    bool passed = (dd.getSelected() == 0);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testDropdownClickInBarToggles()
{
    std::cout << "Dropdown : click in collapsed bar expands; second click collapses: ";
    Dropdown dd(ivec2(10, 10), ivec2(110, 30),
                vec3(0.1f, 0.1f, 0.1f),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.3f, 0.3f, 0.6f));
    dd.addOption("a");
    dd.addOption("b");

    ClickEvent open(50, 20);
    bool consumedOpen = dd(&open);
    bool expandedNow = dd.isExpanded();

    ClickEvent closeAgain(50, 20);
    bool consumedClose = dd(&closeAgain);
    bool collapsedNow = !dd.isExpanded();

    bool passed = consumedOpen && expandedNow && consumedClose && collapsedNow;
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testDropdownClickInOptionSelectsAndCollapses()
{
    std::cout << "Dropdown : click on expanded option commits + collapses: ";
    Dropdown dd(ivec2(0, 0), ivec2(100, 20),
                vec3(0.1f, 0.1f, 0.1f),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.3f, 0.3f, 0.6f));
    dd.addOption("zero");
    dd.addOption("one");
    dd.addOption("two");

    ClickEvent expand(50, 10);
    dd(&expand);

    // Strip starts at maxPos.y + 1 = 21. Row height is 22, so row 1 is
    // y in [43, 65). Pick y = 50 to land squarely on row 1.
    ClickEvent pick(50, 50);
    bool consumed = dd(&pick);

    bool passed = consumed
                  && (dd.getSelected() == 1)
                  && (dd.isExpanded() == false);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testDropdownClickOutsideCollapsesWithoutChange()
{
    std::cout << "Dropdown : click outside while expanded collapses without changing selection: ";
    Dropdown dd(ivec2(0, 0), ivec2(100, 20),
                vec3(0.1f, 0.1f, 0.1f),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.3f, 0.3f, 0.6f));
    dd.addOption("zero");
    dd.addOption("one");
    dd.setSelected(0);

    ClickEvent expand(50, 10);
    dd(&expand);

    ClickEvent away(500, 500);
    bool consumed = dd(&away);

    bool passed = consumed
                  && (dd.getSelected() == 0)
                  && (dd.isExpanded() == false);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testDropdownDrawCollapsedDoesNotCrash()
{
    std::cout << "Dropdown : draw in collapsed state does not crash: ";
    Screen screen(256, 128);
    Dropdown dd(ivec2(0, 0), ivec2(100, 20),
                vec3(0.1f, 0.1f, 0.1f),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.3f, 0.3f, 0.6f));
    dd.addOption("alpha");
    dd.draw(screen);
    std::cout << "PASS" << '\n';
}

void testDropdownDrawExpandedDoesNotCrash()
{
    std::cout << "Dropdown : draw in expanded state does not crash: ";
    Screen screen(256, 256);
    Dropdown dd(ivec2(0, 0), ivec2(100, 20),
                vec3(0.1f, 0.1f, 0.1f),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.3f, 0.3f, 0.6f));
    dd.addOption("alpha");
    dd.addOption("beta");
    dd.addOption("gamma");

    ClickEvent expand(50, 10);
    dd(&expand);

    dd.draw(screen);
    std::cout << "PASS" << '\n';
}

void dropdownTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Dropdown : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testDropdownConstruction();
    testDropdownAddOptionAndSetSelected();
    testDropdownSetSelectedOutOfRangeIgnored();
    testDropdownClickInBarToggles();
    testDropdownClickInOptionSelectsAndCollapses();
    testDropdownClickOutsideCollapsesWithoutChange();
    testDropdownDrawCollapsedDoesNotCrash();
    testDropdownDrawExpandedDoesNotCrash();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Dropdown tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
