#include <iostream>
#include <memory>
#include <SDL3/SDL.h>
#include "screen.hpp"
#include "Layout.hpp"
#include "Point.hpp"
#include "Bar.hpp"

static bool checkPixel(SDL_Surface* surface, int x, int y, Uint8 expectedR, Uint8 expectedG, Uint8 expectedB)
{
    Uint8 r = 0, g = 0, b = 0, a = 0;
    SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a);
    return r == expectedR && g == expectedG && b == expectedB;
}

static void testSetActiveIsActive()
{
    std::cout << "Layout - setActive/isActive: ";

    Layout layout(vec2(0.0f, 0.0f), vec2(1.0f, 1.0f));

    bool defaultActive = layout.isActive();
    layout.setActive(false);
    bool afterDeactivate = layout.isActive();
    layout.setActive(true);
    bool afterReactivate = layout.isActive();

    bool passed = (defaultActive && !afterDeactivate && afterReactivate);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testResolveAbsPositions()
{
    std::cout << "Layout - resolveAbsStart/resolveAbsEnd: ";

    Layout layout(vec2(0.25f, 0.25f), vec2(0.75f, 0.75f));
    ivec2 parentStart(0, 0);
    ivec2 parentEnd(100, 100);

    ivec2 absStart = layout.resolveAbsStart(parentStart, parentEnd);
    ivec2 absEnd = layout.resolveAbsEnd(parentStart, parentEnd);

    bool passed = (absStart == ivec2(25, 25)) && (absEnd == ivec2(75, 75));
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testDrawSkipsWhenInactive()
{
    std::cout << "Layout - draw skips when inactive: ";

    Screen s(50, 50);
    Layout layout(vec2(0.0f, 0.0f), vec2(1.0f, 1.0f));
    layout.addElement(std::make_unique<Point>(vec2(5.0f, 5.0f), vec3(1.0f, 0.0f, 0.0f)));
    layout.setActive(false);
    layout.draw(s, ivec2(0, 0), ivec2(50, 50));

    SDL_Surface* target = SDL_CreateSurface(50, 50, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);

    bool passed = checkPixel(target, 5, 5, 0, 0, 0);

    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testCopyConstructorDeepClonesElements()
{
    std::cout << "Layout - copy ctor deep-clones polymorphic children via CRTP clone(): ";

    Layout original(vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), true, "src");
    auto bar = std::make_unique<Bar>(ivec2(0, 0), ivec2(50, 10), vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 0.0f, 100.0f, "cpu");
    bar->setValue(42.0f);
    original.addElement(std::move(bar));

    Layout copy = original;

    bool sizeMatches = (copy.elements.size() == 1);
    Bar *originalBar = dynamic_cast<Bar *>(original.elements.front().get());
    Bar *copyBar = dynamic_cast<Bar *>(copy.elements.front().get());
    bool independentPointers = (originalBar != nullptr) && (copyBar != nullptr) && (originalBar != copyBar);
    bool valueCopied = independentPointers && (copyBar->getValue() == 42.0f);

    if (originalBar != nullptr)
    {
        originalBar->setValue(7.0f);
    }
    bool independentState = independentPointers && (copyBar->getValue() == 42.0f);

    bool passed = sizeMatches && independentPointers && valueCopied && independentState;
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testAssignmentOperatorDeepClones()
{
    std::cout << "Layout - operator= deep-clones children: ";

    Layout source(vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), true, "src");
    source.addElement(std::make_unique<Point>(vec2(5.0f, 5.0f), vec3(1.0f, 0.0f, 0.0f)));

    Layout target(vec2(0.0f, 0.0f), vec2(0.5f, 0.5f), false, "old");
    target.addElement(std::make_unique<Point>(vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)));
    target.addElement(std::make_unique<Point>(vec2(1.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f)));

    target = source;

    bool sizeReplaced = (target.elements.size() == 1);
    bool nameCopied = (target.getName() == "src");
    bool independent = (target.elements.front().get() != source.elements.front().get());

    bool passed = sizeReplaced && nameCopied && independent;
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void layoutTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Layout - Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testSetActiveIsActive();
    testResolveAbsPositions();
    testDrawSkipsWhenInactive();
    testCopyConstructorDeepClonesElements();
    testAssignmentOperatorDeepClones();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Layout tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
