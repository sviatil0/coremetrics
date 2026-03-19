#include <iostream>
#include <SDL3/SDL.h>
#include "screen.hpp"
#include "label.hpp"
#include "selection.hpp"
#include "image.hpp"

bool checkPixel(SDL_Surface *surface, int x, int y, Uint8 expectedR, Uint8 expectedG, Uint8 expectedB)
{
    Uint8 r = 0, g = 0, b = 0, a = 0;
    SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a);
    return r == expectedR && g == expectedG && b == expectedB;
}

void testLabelPlacement()
{
    std::cout << "Label - Placeholder Rendering: ";
    Screen s(50, 50);
    Label l("Test", ivec2(20, 20), vec3(0.0f, 0.0f, 1.0f));
    l.draw(s);

    SDL_Surface *target = SDL_CreateSurface(50, 50, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);

    bool passed = checkPixel(target, 20, 20, 0, 0, 255);
    
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testSelectionLogic()
{
    std::cout << "Selection - State Toggling: ";
    Selection sel(ivec2(0, 0), "Check", false);
    
    bool initial = sel.isSelected();
    sel.toggle();
    bool after = sel.isSelected();
    
    bool passed = (!initial && after);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testSelectionCheckedVisual()
{
    std::cout << "Selection - Green Mark Logic: ";
    Screen s(40, 40);
    Selection sel(ivec2(0, 0), "Check", true);
    sel.draw(s);

    SDL_Surface *target = SDL_CreateSurface(40, 40, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);

    bool passed = checkPixel(target, 4, 4, 0, 255, 0);
    
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testImageMissingFile()
{
    std::cout << "Image - Graceful Load Failure: ";
    Screen s(10, 10);

    Image img("missing_ghost.bmp", ivec2(0, 0));
    
    img.draw(s); 
    std::cout << "PASS" << '\n';
}

void GUIElementTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  GUIElement - Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testLabelPlacement();
    testSelectionLogic();
    testSelectionCheckedVisual();
    testImageMissingFile();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  GUIElement tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}