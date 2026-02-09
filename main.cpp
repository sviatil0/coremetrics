#include <iostream>
#include <cstdint>
#include <SDL3/SDL.h>

constexpr int RESX = 960;
constexpr int RESY = 540;

void clearSurface(SDL_Surface*);

int main(int argc, char** argv)
{
	SDL_Window* window = nullptr;

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		std::cerr << "Failed to init SDL: " << SDL_GetError() << '\n';
		return -1;
	}

	window = SDL_CreateWindow("Hello Window", RESX, RESY, 0);

	if (!window)
	{
		std::cerr << "Failed to create a window: " << SDL_GetError() << '\n';
		return -2;
	}

	SDL_Surface* buffer = SDL_CreateSurface(RESX, RESY, SDL_PIXELFORMAT_RGBA8888);
	SDL_Event event;
	bool end = false;
	while (!end)
	{
		while (SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_EVENT_QUIT: end = true; break;
			case SDL_EVENT_KEY_DOWN: std::cout << "Key pressed\n"; break;
			case SDL_EVENT_KEY_UP: std::cout << "Key released\n"; break;
			}
		}
		clearSurface(buffer);
		SDL_BlitSurface(buffer, NULL, SDL_GetWindowSurface(window), NULL);
		SDL_UpdateWindowSurface(window);
	}

	SDL_DestroySurface(buffer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

void clearSurface(SDL_Surface* buf)
{
	std::uint32_t* pixels = static_cast<std::uint32_t*>(buf->pixels);
	size_t curIndex = 0;
	for (int i = 0; i < buf->w; ++i)
	{
		for (int j = 0; j < buf->h; ++j)
		{
			pixels[j * buf->w + i] = SDL_MapSurfaceRGBA(buf, 0xFF, 0x00, 0xFF, 0xFF);
		}
	}
}
