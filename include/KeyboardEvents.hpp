#ifndef __KEYBOARDEVENTS_HPP__
#define __KEYBOARDEVENTS_HPP__

#include <SDL3/SDL.h>

// Pulled out of coremetrics.cpp as Phase 1.2 slice 13 of the
// modernization roadmap. handle() owns every keystroke the live UI
// reacts to: help overlay toggle, signal menu picker/confirm flow,
// filter input + commit, process-list navigation (Up/Down/Home/End/
// PgUp/PgDn), and the tree-mode + collapse toggle on the Processes
// tab. The handler reads and mutates the coremetrics.cpp live-UI
// globals directly through `extern` declarations at the top of
// KeyboardEvents.cpp; those globals had to lose their `static`
// keyword to gain external linkage. That coupling is the same
// state-machine the previous in-place block already had; the slice
// just moves the code, not the data ownership. Slices C9 (the wider
// EventHandlers extraction) and beyond will continue tightening
// this surface.
//
// Returns true if the event was consumed (and should not bubble
// further), false otherwise. The current call site does not branch
// on the return value, but the contract is in place for future
// composition.
namespace KeyboardEvents
{
    bool handle(SDL_KeyboardEvent const &event);
}

#endif
