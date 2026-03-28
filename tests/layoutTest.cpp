#include <iostream>
#include <SDL3/SDL.h>
#include "screen.hpp"
#include "Layout.hpp"
#include "selection.hpp"

static bool checkPixel(SDL_Surface* surface, int x, int y, Uint8 expectedR, Uint8 expectedG, Uint8 expectedB)
{
    Uint8 r = 0, g = 0, b = 0, a = 0;
    SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a);
    return r == expectedR && g == expectedG && b == expectedB;
}

static void testSetActiveIsActive()
{
    std::cout << "Layout - setActive/isActive: ";

    Layout layout(ivec2(0, 0), ivec2(100, 100));

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

    Layout layout(ivec2(10, 20), ivec2(50, 60));
    ivec2 offset(5, 5);

    ivec2 absStart = layout.resolveAbsStart(offset);
    ivec2 absEnd = layout.resolveAbsEnd(offset);

    bool passed = (absStart == ivec2(15, 25)) && (absEnd == ivec2(55, 65));
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testDrawSkipsWhenInactive()
{
    std::cout << "Layout - draw skips when inactive: ";

    Screen s(50, 50);
    Selection sel(ivec2(0, 0), "Check", true);

    Layout layout(ivec2(0, 0), ivec2(50, 50));
    layout.addElement(&sel);
    layout.setActive(false);
    layout.draw(s);

    SDL_Surface* target = SDL_CreateSurface(50, 50, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);

    bool passed = checkPixel(target, 0, 0, 0, 0, 0);

    SDL_DestroySurface(target);
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

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Layout tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
