#include <iostream>
#include <memory>
#include <SDL3/SDL.h>
#include "screen.hpp"
#include "LayoutManager.hpp"
#include "EventManager.hpp"
#include "ClickEvent.hpp"
#include "Box.hpp"
#include "Line.hpp"
#include "Point.hpp"
#include "button.hpp"

constexpr int RESX = 960;
constexpr int RESY = 540;
constexpr int HALF_W = RESX / 2;
constexpr int HALF_H = RESY / 2;
constexpr int QUARTER_W = RESX / 4;
constexpr int QUARTER_H = RESY / 4;

int crossEdge(ivec2 a, ivec2 b, ivec2 c)
{
    return ((b.y - a.y) * (c.x - a.x)) - ((b.x - a.x) * (c.y - a.y));
}

bool pointInTriangle(ivec2 p, ivec2 v1, ivec2 v2, ivec2 v3)
{
    int w0 = crossEdge(v2, v3, p);
    int w1 = crossEdge(v3, v1, p);
    int w2 = crossEdge(v1, v2, p);
    bool clockwise = (w0 >= 0 && w1 >= 0 && w2 >= 0);
    bool counterClockwise = (w0 <= 0 && w1 <= 0 && w2 <= 0);
    return clockwise || counterClockwise;
}

int main(int argc, char **argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::cerr << "Failed to init SDL: " << SDL_GetError() << '\n';
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("Milestone 005 Demo", RESX, RESY, 0);
    if (!window)
    {
        std::cerr << "Failed to create a window: " << SDL_GetError() << '\n';
        return -2;
    }

    Screen screen(RESX, RESY);

    ivec2 triV1(HALF_W - QUARTER_W, HALF_H - QUARTER_H);
    ivec2 triV2(HALF_W - QUARTER_W, HALF_H + QUARTER_H);
    ivec2 triV3(HALF_W, HALF_H);

    LayoutManager &manager = LayoutManager::getInstance();

    manager.getRoot().getData().addElement(std::make_unique<Box>(
        vec2(static_cast<float>(HALF_W - QUARTER_W), static_cast<float>(HALF_H - QUARTER_H)),
        vec2(static_cast<float>(HALF_W + QUARTER_W), static_cast<float>(HALF_H + QUARTER_H)),
        vec3(0.0f, 0.3f, 0.6f)));
    manager.getRoot().getData().addElement(std::make_unique<Line>(
        vec2(static_cast<float>(HALF_W), static_cast<float>(HALF_H)),
        vec2(0.0f, 0.0f),
        vec3(1.0f, 1.0f, 1.0f)));
    manager.getRoot().getData().addElement(std::make_unique<Line>(
        vec2(static_cast<float>(HALF_W), static_cast<float>(HALF_H)),
        vec2(static_cast<float>(RESX - 1), 0.0f),
        vec3(1.0f, 1.0f, 1.0f)));
    manager.getRoot().getData().addElement(std::make_unique<Line>(
        vec2(static_cast<float>(HALF_W), static_cast<float>(HALF_H)),
        vec2(0.0f, static_cast<float>(RESY - 1)),
        vec3(1.0f, 1.0f, 1.0f)));
    manager.getRoot().getData().addElement(std::make_unique<Line>(
        vec2(static_cast<float>(HALF_W), static_cast<float>(HALF_H)),
        vec2(static_cast<float>(RESX - 1), static_cast<float>(RESY - 1)),
        vec3(1.0f, 1.0f, 1.0f)));

    Tree<Layout> *overlay = manager.addChild(&manager.getRoot(),
                                             Layout(vec2(0.5f, 0.0f), vec2(1.0f, 1.0f), false, "overlay"));
    overlay->getData().addElement(std::make_unique<Box>(
        vec2(static_cast<float>(HALF_W), 0.0f),
        vec2(static_cast<float>(RESX - 1), static_cast<float>(RESY - 1)),
        vec3(0.6f, 0.1f, 0.1f)));
    overlay->getData().addElement(std::make_unique<Point>(
        vec2(static_cast<float>(HALF_W + QUARTER_W), static_cast<float>(HALF_H)),
        vec3(1.0f, 1.0f, 0.0f)));

    constexpr int BTN_MIN_X = HALF_W + QUARTER_W + 20;
    constexpr int BTN_MIN_Y = HALF_H - 20;
    constexpr int BTN_MAX_X = HALF_W + QUARTER_W + 100;
    constexpr int BTN_MAX_Y = HALF_H + 20;

    manager.getRoot().getData().addElement(std::make_unique<Button>(
        ivec2(BTN_MIN_X, BTN_MIN_Y),
        ivec2(BTN_MAX_X, BTN_MAX_Y),
        vec3(0.2f, 0.8f, 0.2f),
        "assets/click.wav",
        "overlay"));

    vec3 triangleColor(0.8f, 0.2f, 0.3f);

    SDL_Event event;
    bool end = false;
    float mouseX = 0.0f;
    float mouseY = 0.0f;

    while (!end)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
            {
                end = true;
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                float clickX = 0.0f;
                float clickY = 0.0f;
                SDL_GetMouseState(&clickX, &clickY);
                EventManager::getInstance().pushEvent(
                    std::make_unique<ClickEvent>(static_cast<int>(clickX), static_cast<int>(clickY)));
                break;
            }
            }
        }

        EventManager::getInstance().processEvents(ivec2(0, 0), ivec2(RESX - 1, RESY - 1));

        screen.clear();
        manager.render(screen, ivec2(0, 0), ivec2(RESX - 1, RESY - 1));
        screen.drawTriangle(triV1, triV2, triV3, triangleColor);
        screen.blitTo(SDL_GetWindowSurface(window));
        SDL_UpdateWindowSurface(window);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
