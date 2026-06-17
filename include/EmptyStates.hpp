#ifndef __EMPTYSTATES_HPP__
#define __EMPTYSTATES_HPP__

#include "screen.hpp"

// Hint painted over an empty table region when there is no data to
// show. Replaces the blank-rows-with-no-explanation slate the
// Processes tab used to render during the boot moment before the first
// pollMetrics() returns. Does not yet cover the filter-empty case
// (filter excludes every visible row while the underlying list is
// non-empty); see the comment on the call-site gate in coremetrics.cpp
// for the follow-up.
//
// Pillar A6 of docs/superpowers/specs/2026-06-16-modernization-roadmap.md.
namespace EmptyStates
{
    // Caller is responsible for gating on processesTabActive() and the
    // empty condition; this helper only knows how to paint the hint.
    void renderProcessesEmpty(Screen &dest);
}

#endif
