#ifndef __HELPOVERLAY_HPP__
#define __HELPOVERLAY_HPP__

#include "screen.hpp"

// Renders the modal keyboard-shortcuts panel onto the given screen.
// Caller is responsible for visibility gating; this function paints
// unconditionally. Used by coremetrics.cpp's main render pass when
// g_helpOverlayVisible is true.
//
// First slice of Phase 1.2 from the GUI evolution spec (PR #163):
// the goal is to peel render functions out of coremetrics.cpp into
// their own translation units so the file stops being monolithic.
namespace HelpOverlay
{
    void render(Screen &dest);
}

#endif
