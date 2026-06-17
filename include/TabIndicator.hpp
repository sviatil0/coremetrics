#ifndef __TABINDICATOR_HPP__
#define __TABINDICATOR_HPP__

#include "screen.hpp"

// Paints a 2 px accent stripe directly under the currently active tab
// button so the active tab is visually distinguished from inactive
// siblings. Without this strip the four tab buttons in base.xml all
// share Theme::panelBg() and the only cue is which content layout is
// visible. Stripe sits at y=42..44 spanning the active tab button's
// x range.
//
// tabIndex mapping: 0 = System (x=8..402), 1 = Processes (x=410..804).
// Unknown values paint nothing rather than crashing; the helper stays
// inert if a future tab is added before the indicator is taught about it.
//
// Pillar A5 of docs/superpowers/specs/2026-06-16-modernization-roadmap.md.
namespace TabIndicator
{
    void render(Screen &dest, int tabIndex);
}

#endif
