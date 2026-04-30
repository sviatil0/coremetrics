#include <iostream>
#include <SDL3/SDL.h>
#include "screen.hpp"
#include "Label.hpp"
#include "selection.hpp"
#include "image.hpp"
#include "Button.hpp"
#include "GUIElements.hpp"
#include "GUIElementFactory.hpp"
#include "ClickEvent.hpp"
#include "ShowEvent.hpp"

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

void testButtonDraw()
{
    std::cout << "Button - Draw White Border: ";
    Screen s(30, 30);
    Button btn(ivec2(5, 5), ivec2(20, 20), vec3(0.0f, 0.0f, 1.0f));
    btn.draw(s);

    SDL_Surface *target = SDL_CreateSurface(30, 30, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);

    bool passed = checkPixel(target, 5, 5, 255, 255, 255);

    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testButtonCheckToggle()
{
    std::cout << "Button - Hit Detection: ";
    Button btn(ivec2(10, 10), ivec2(20, 20), vec3(1.0f, 0.0f, 0.0f));

    bool inside = btn.checkToggle(15, 15);
    bool outside = btn.checkToggle(5, 5);

    bool passed = inside && !outside;
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testPointDraw()
{
    std::cout << "Point - Draw: ";
    Screen s(20, 20);
    Point p(vec2(10.0f, 10.0f), vec3(1.0f, 0.0f, 0.0f));
    p.draw(s);

    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);

    bool passed = checkPixel(target, 10, 10, 255, 0, 0);

    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testLineDraw()
{
    std::cout << "Line - Draw: ";
    Screen s(20, 20);
    Line l(vec2(0.0f, 0.0f), vec2(19.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    l.draw(s);

    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);

    bool passed = checkPixel(target, 0, 0, 0, 255, 0)
               && checkPixel(target, 10, 0, 0, 255, 0)
               && checkPixel(target, 19, 0, 0, 255, 0);

    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testBoxDraw()
{
    std::cout << "Box - Draw: ";
    Screen s(20, 20);
    Box b(vec2(2.0f, 2.0f), vec2(10.0f, 10.0f), vec3(0.0f, 0.0f, 1.0f));
    b.draw(s);

    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);

    bool passed = checkPixel(target, 2, 2, 0, 0, 255)
               && checkPixel(target, 6, 6, 0, 0, 255)
               && checkPixel(target, 10, 10, 0, 0, 255);

    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testFactoryCreatePoint()
{
    std::cout << "GUIElementFactory - createPoint: ";
    std::unique_ptr<GUIElement> element = GUIElementFactory::createPoint(vec2(5.0f, 5.0f), vec3(1.0f, 0.0f, 0.0f));
    Screen s(20, 20);
    if (element)
    {
        element->draw(s);
    }
    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = element != nullptr && checkPixel(target, 5, 5, 255, 0, 0);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testFactoryCreateLine()
{
    std::cout << "GUIElementFactory - createLine: ";
    std::unique_ptr<GUIElement> element = GUIElementFactory::createLine(vec2(0.0f, 5.0f), vec2(19.0f, 5.0f), vec3(1.0f, 1.0f, 0.0f));
    Screen s(20, 20);
    if (element)
    {
        element->draw(s);
    }
    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = element != nullptr && checkPixel(target, 10, 5, 255, 255, 0);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testFactoryCreateBox()
{
    std::cout << "GUIElementFactory - createBox: ";
    std::unique_ptr<GUIElement> element = GUIElementFactory::createBox(vec2(1.0f, 1.0f), vec2(8.0f, 8.0f), vec3(1.0f, 0.0f, 1.0f));
    Screen s(20, 20);
    if (element)
    {
        element->draw(s);
    }
    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = element != nullptr && checkPixel(target, 4, 4, 255, 0, 255);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testFactoryCreate()
{
    std::cout << "GUIElementFactory - create (generic): ";
    std::unique_ptr<GUIElement> point = GUIElementFactory::create(GUIElementType::POINT, vec2(3.0f, 3.0f), vec2(), vec3(1.0f, 0.0f, 0.0f));
    std::unique_ptr<GUIElement> line = GUIElementFactory::create(GUIElementType::LINE, vec2(0.0f, 8.0f), vec2(19.0f, 8.0f), vec3(0.0f, 1.0f, 0.0f));
    std::unique_ptr<GUIElement> box = GUIElementFactory::create(GUIElementType::BOX, vec2(12.0f, 12.0f), vec2(18.0f, 18.0f), vec3(0.0f, 0.0f, 1.0f));
    Screen s(20, 20);
    if (point)
    {
        point->draw(s);
    }
    if (line)
    {
        line->draw(s);
    }
    if (box)
    {
        box->draw(s);
    }
    SDL_Surface *target = SDL_CreateSurface(20, 20, SDL_PIXELFORMAT_RGBA8888);
    s.blitTo(target);
    bool passed = point && line && box
               && checkPixel(target, 3, 3, 255, 0, 0)
               && checkPixel(target, 10, 8, 0, 255, 0)
               && checkPixel(target, 15, 15, 0, 0, 255);
    SDL_DestroySurface(target);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testButtonOperatorHit()
{
    std::cout << "Button - operator() hit returns true: ";
    Button btn(ivec2(10, 10), ivec2(20, 20), vec3(1.0f, 0.0f, 0.0f));
    ClickEvent event(15, 15);
    bool passed = btn(&event);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testButtonOperatorMiss()
{
    std::cout << "Button - operator() miss returns false: ";
    Button btn(ivec2(10, 10), ivec2(20, 20), vec3(1.0f, 0.0f, 0.0f));
    ClickEvent event(5, 5);
    bool passed = !btn(&event);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testButtonOperatorWrongType()
{
    std::cout << "Button - operator() wrong event type returns false: ";
    Button btn(ivec2(10, 10), ivec2(20, 20), vec3(1.0f, 0.0f, 0.0f));
    ShowEvent event("someLayout", true);
    bool passed = !btn(&event);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
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
    testButtonDraw();
    testButtonCheckToggle();
    testButtonOperatorHit();
    testButtonOperatorMiss();
    testButtonOperatorWrongType();
    testPointDraw();
    testLineDraw();
    testBoxDraw();
    testFactoryCreatePoint();
    testFactoryCreateLine();
    testFactoryCreateBox();
    testFactoryCreate();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  GUIElement tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
