#ifndef __MOUSEEVENTS_HPP__
#define __MOUSEEVENTS_HPP__

#include <SDL3/SDL.h>

// Pulled out of coremetrics.cpp as Phase 1.2 slice 14 of the
// modernization roadmap, mirroring the KeyboardEvents slice that
// landed just before. handleButtonDown() owns every mouse click the
// live UI reacts to: window-to-viewport letterboxing math, EXIT and
// SOUND button hits, three-tab fan-out, header-row sort toggles,
// process-row selection, and signal-menu dismissal when the click
// misses the panel. handleWheel() drives Processes-tab vertical
// scrolling.
//
// Both handlers read and mutate file-scope state (mute / sort /
// selected-pid / scroll offset / tab geometry rects) through
// extern declarations at the top of MouseEvents.cpp; that state
// moved here from coremetrics.cpp so the new TU owns the data it
// touches, the same shape slice 13 used for the keyboard handler.
//
// Returns true if the event was consumed (and should not bubble
// further), false otherwise. The current call site does not branch
// on the return value, but the contract is in place for future
// composition (e.g. the wider EventHandlers extraction).
namespace MouseEvents
{
    bool handleButtonDown(SDL_MouseButtonEvent const &event);
    bool handleWheel(SDL_MouseWheelEvent const &event);
}

#endif
