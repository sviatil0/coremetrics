#include <iostream>
#include <SDL3/SDL.h>
#include "screen.hpp"

static bool checkPixel(SDL_Surface *surface, int x, int y, Uint8 expectedR, Uint8 expectedG, Uint8 expectedB)
{
    Uint8 r = 0, g = 0, b = 0, a = 0;
    SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a);
    return r == expectedR && g == expectedG && b == expectedB;
}

static void testConstruction()
{
    std::cout << "Screen construction: ";
    Screen s(10, 10);
    SDL_Surface *target = SDL_CreateSurface(10, 10, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    SDL_DestroySurface(target);
    std::cout << "PASS" << '\n';
}

static void testDrawPixelFloat()
{
    std::cout << "Screen drawPixel (float color): ";
    Screen s(10, 10);
    s.drawPixel(ivec2(5, 5), vec3(1.0f, 0.0f, 0.0f));
    SDL_Surface *target = SDL_CreateSurface(10, 10, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = checkPixel(target, 5, 5, 255, 0, 0);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testDrawPixelInt()
{
    std::cout << "Screen drawPixel (int color): ";
    Screen s(10, 10);
    s.drawPixel(ivec2(3, 3), ivec3(0, 255, 0));
    SDL_Surface *target = SDL_CreateSurface(10, 10, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = checkPixel(target, 3, 3, 0, 255, 0);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testDrawPixelOutOfBounds()
{
    std::cout << "Screen drawPixel out of bounds: ";
    Screen s(10, 10);
    s.drawPixel(ivec2(-1, -1), vec3(1.0f, 0.0f, 0.0f));
    s.drawPixel(ivec2(10, 10), vec3(1.0f, 0.0f, 0.0f));
    s.drawPixel(ivec2(100, 100), vec3(1.0f, 0.0f, 0.0f));
    std::cout << "PASS" << '\n';
}

static void testDrawLineHorizontal()
{
    std::cout << "Screen drawLine horizontal: ";
    Screen s(20, 20);
    s.drawLine(ivec2(0, 10), ivec2(19, 10), vec3(0.0f, 0.0f, 1.0f));
    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = checkPixel(target, 0, 10, 0, 0, 255)
               && checkPixel(target, 10, 10, 0, 0, 255)
               && checkPixel(target, 19, 10, 0, 0, 255);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testDrawLineReversed()
{
    std::cout << "Screen drawLine reversed endpoints: ";
    Screen s(20, 20);
    s.drawLine(ivec2(19, 10), ivec2(0, 10), vec3(0.0f, 0.0f, 1.0f));
    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = checkPixel(target, 0, 10, 0, 0, 255)
               && checkPixel(target, 10, 10, 0, 0, 255)
               && checkPixel(target, 19, 10, 0, 0, 255);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testDrawLineVertical()
{
    std::cout << "Screen drawLine vertical: ";
    Screen s(20, 20);
    s.drawLine(ivec2(10, 0), ivec2(10, 19), vec3(0.0f, 1.0f, 0.0f));
    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = checkPixel(target, 10, 0, 0, 255, 0)
               && checkPixel(target, 10, 10, 0, 255, 0)
               && checkPixel(target, 10, 19, 0, 255, 0);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testDrawBoxNormal()
{
    std::cout << "Screen drawBox: ";
    Screen s(20, 20);
    s.drawBox(ivec2(5, 5), ivec2(10, 10), vec3(1.0f, 0.0f, 1.0f));
    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = checkPixel(target, 5, 5, 255, 0, 255)
               && checkPixel(target, 7, 7, 255, 0, 255)
               && checkPixel(target, 10, 10, 255, 0, 255);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testDrawBoxSwapped()
{
    std::cout << "Screen drawBox swapped corners: ";
    Screen s(20, 20);
    s.drawBox(ivec2(10, 10), ivec2(5, 5), vec3(1.0f, 0.0f, 1.0f));
    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = checkPixel(target, 5, 5, 255, 0, 255)
               && checkPixel(target, 7, 7, 255, 0, 255)
               && checkPixel(target, 10, 10, 255, 0, 255);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testDrawLineDiagonalDownRight()
{
    std::cout << "Screen drawLine diagonal (top-left to bottom-right): ";
    Screen s(20, 20);
    s.drawLine(ivec2(0, 0), ivec2(19, 19), vec3(1.0f, 0.0f, 0.0f));
    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = checkPixel(target, 0, 0, 255, 0, 0)
               && checkPixel(target, 10, 10, 255, 0, 0)
               && checkPixel(target, 19, 19, 255, 0, 0);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testDrawLineDiagonalUpRight()
{
    std::cout << "Screen drawLine diagonal (bottom-left to top-right): ";
    Screen s(20, 20);
    s.drawLine(ivec2(0, 19), ivec2(19, 0), vec3(0.0f, 1.0f, 0.0f));
    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = checkPixel(target, 0, 19, 0, 255, 0)
               && checkPixel(target, 10, 9, 0, 255, 0)
               && checkPixel(target, 19, 0, 0, 255, 0);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testDrawLineSteepDiagonal()
{
    std::cout << "Screen drawLine steep diagonal: ";
    Screen s(20, 20);
    s.drawLine(ivec2(5, 0), ivec2(10, 19), vec3(1.0f, 1.0f, 0.0f));
    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = checkPixel(target, 5, 0, 255, 255, 0)
               && checkPixel(target, 10, 19, 255, 255, 0);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testBlitTo()
{
    std::cout << "Screen blitTo: ";
    Screen s(10, 10);
    s.drawPixel(ivec2(2, 2), vec3(1.0f, 1.0f, 1.0f));
    SDL_Surface *target = SDL_CreateSurface(10, 10, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = checkPixel(target, 2, 2, 255, 255, 255);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void screenTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Screen - Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testConstruction();
    testDrawPixelFloat();
    testDrawPixelInt();
    testDrawPixelOutOfBounds();
    testDrawLineHorizontal();
    testDrawLineReversed();
    testDrawLineVertical();
    testDrawLineDiagonalDownRight();
    testDrawLineDiagonalUpRight();
    testDrawLineSteepDiagonal();
    testDrawBoxNormal();
    testDrawBoxSwapped();
    testBlitTo();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Screen tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}