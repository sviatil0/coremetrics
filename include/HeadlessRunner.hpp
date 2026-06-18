#ifndef __HEADLESSRUNNER_HPP__
#define __HEADLESSRUNNER_HPP__

#include <string>

#include "TopProcessesPrinter.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 16 of the
// modernization roadmap, mirroring the KeyboardEvents (slice 13),
// MouseEvents (slice 14), and MetricsPoller (slice 15) extractions
// that landed just before. main() previously housed every headless
// dispatch path next to the live UI loop: --top / --watch, the CSV +
// JSON exporters, and the --screenshot frame renderer. Together the
// three blocks were ~280 lines of CLI-driven logic that never paint
// inside the SDL event loop, so they belong outside the main TU.
//
// Each runner reads its own parameters (paths, sort + colorize, tab
// selector) and returns an int exit code. The caller invokes them in
// priority order at the top of main() and returns that exit code
// immediately when any of them was selected. A return value of -1 is
// reserved for "this mode was not requested" so the caller can chain
// the runners without a flag-per-mode check; in practice main()
// already gates each call by the same flag the runner inspects so
// only runTopMode() needs the early-return contract.
//
// runScreenshotMode() owns its own SDL_Init(SDL_INIT_VIDEO) +
// SDL_Quit() so the live UI path can keep its own SDL_Init with audio
// enabled. The --screenshot path deliberately skips SDL_INIT_AUDIO
// because the AudioQueue background thread races the screenshot
// teardown on macOS and produces a sporadic EXC_BAD_ACCESS in
// pthread_mutex_lock during static destructors after the file has
// already been written.
namespace HeadlessRunner
{
    // Returns the exit code if the corresponding mode is active, or
    // -1 if the mode was not requested (caller continues main). The
    // current caller already gates each invocation by the same flag
    // so the -1 sentinel is reserved for future composition.
    int runTopMode(int topCount,
                   TopSortKey topSortKey,
                   bool colorize,
                   bool watch,
                   unsigned long long pollIntervalMs);
    int runExportMode(const std::string &csvPath,
                      const std::string &jsonPath);
    int runScreenshotMode(const std::string &screenshotPath,
                          const std::string &screenshotTab);
}

#endif
