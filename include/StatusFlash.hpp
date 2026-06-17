#ifndef __STATUSFLASH_HPP__
#define __STATUSFLASH_HPP__

#include <string>

#include "screen.hpp"

// Paints the transient status-flash message (e.g. "config saved",
// "signal sent") at the right end of the footer status strip, just
// before the EXIT button. The helper is the smallest possible
// extraction: one Font::drawText call at a fixed (x, y) anchor with
// the load-success accent green.
//
// The helper is intentionally pure render. It does not check timing,
// does not consult any clock, and does not own any state. The caller
// in coremetrics.cpp gates this on (1) non-empty text AND (2) within
// the expiry window (SDL_GetTicks() < g_statusFlashExpiryMs); the
// caller also clears g_statusFlash once the window elapses. If
// StatusFlash::render() is invoked, the text paints.
//
// Phase 1.2 follow-up of the modernization roadmap. Extracted from
// coremetrics.cpp alongside the final COLOR_ACCENT_GREEN cleanup
// (PRs #207, #216, #218) so every accent-green site flows through
// Theme tokens.
namespace StatusFlash
{
    void render(Screen &dest, const std::string &text);
}

#endif
