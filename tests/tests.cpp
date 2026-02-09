#include <iostream>
#include <SDL3/SDL.h>

int main(int argc, char** argv)
{
	std::cout << "Hello Testing Environment\n";

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		std::cerr << "Failed to init SDL3 " << SDL_GetError() << '\n';
		return -1;
	}


	SDL_Quit();

	std::cout << "SDL opened and closed successfully\n";

	return 0;
}
