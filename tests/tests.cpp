#include <iostream>
#include <SDL3/SDL.h>
#include "linearTest.hpp"
#include "screenTest.hpp"
#include "guiFileTest.hpp"
#include "GUIElementTest.hpp"
#include "TreeTest.hpp"
#include "LayoutManagerTest.hpp"
#include "EventManagerTest.hpp"
#include "layoutTest.hpp"
#include "BarTest.hpp"
#include "RowTest.hpp"
#include "SystemMetricsTest.hpp"

int main(int argc, char **argv)
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		std::cerr << "Failed to init SDL3: " << SDL_GetError() << '\n';
		return -1;
	}

	linearTestSuite();
	screenTestSuite();
	guiFileTestSuite();
	GUIElementTestSuite();
	treeTestSuite();
	layoutManagerTestSuite();
	eventManagerTestSuite();
	layoutTestSuite();
	barTestSuite();
	rowTestSuite();
	systemMetricsTestSuite();

	SDL_Quit();

	std::cout << "=============================================" << '\n';
	std::cout << "  All tests completed!" << '\n';
	std::cout << "=============================================" << '\n';
	return 0;
}
