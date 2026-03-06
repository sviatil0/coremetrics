#include <iostream>
#include <SDL3/SDL.h>
#include "screen.hpp"

constexpr int RESX = 960;
constexpr int RESY = 540;

int main(int argc, char** argv)
{
	SDL_Window* window = nullptr;

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		std::cerr << "Failed to init SDL: " << SDL_GetError() << '\n';
		return -1;
	}

	window = SDL_CreateWindow("Milestone 002 Demo", RESX, RESY, 0);

	if (!window)
	{
		std::cerr << "Failed to create a window: " << SDL_GetError() << '\n';
		return -2;
	}

	Screen screen(RESX, RESY);

	ivec2 center(RESX / 2, RESY / 2);
	ivec2 topLeft(0, 0);
	ivec2 topRight(RESX - 1, 0);
	ivec2 bottomLeft(0, RESY - 1);
	ivec2 bottomRight(RESX - 1, RESY - 1);

	constexpr int BOX_HALF_WIDTH = RESX / 4;
	constexpr int BOX_HALF_HEIGHT = RESY / 4;
	ivec2 boxMin(center.x - BOX_HALF_WIDTH, center.y - BOX_HALF_HEIGHT);
	ivec2 boxMax(center.x + BOX_HALF_WIDTH, center.y + BOX_HALF_HEIGHT);

	vec3 boxColor(0.0f, 0.3f, 0.6f);
	screen.drawBox(boxMin, boxMax, boxColor);

	vec3 lineColor(1.0f, 1.0f, 1.0f);
	screen.drawLine(center, topLeft, lineColor);
	screen.drawLine(center, topRight, lineColor);
	screen.drawLine(center, bottomLeft, lineColor);
	screen.drawLine(center, bottomRight, lineColor);

	vec3 triangleColor(0.8f, 0.2f, 0.3f);
	vec2 triV1(center.x - BOX_HALF_WIDTH, center.y - BOX_HALF_HEIGHT);
	vec2 triV3(center.x, center.y);
	vec2 triV2(center.x - BOX_HALF_WIDTH, center.y + BOX_HALF_HEIGHT);
	screen.drawTriangle(triV1, triV2, triV3, triangleColor);

	SDL_Event event;
	bool end = false;
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
			}
		}
		screen.blitTo(SDL_GetWindowSurface(window));
		SDL_UpdateWindowSurface(window);
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}