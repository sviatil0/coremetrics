#ifndef __FILTERSTRIP_HPP__
#define __FILTERSTRIP_HPP__

#include <string>

#include "screen.hpp"

// Paints the Processes-tab filter input strip that sits above the
// header row. Two-tone presentation: an accent-green "filter>" /
// "filter:" prefix at x=24, the live query in primary text at x=120,
// and a dim "Esc clears" hint at x=800 when the input is inactive but
// the query is non-empty. The blinking underscore cursor is appended
// every other 400 ms slot while input is active. All inputs are
// passed explicitly so the helper holds no static state of its own.
//
// The caller in coremetrics.cpp still gates this on
// `processesTabActive() && (g_filterInputActive || !g_filterText.empty())`
// so the strip never paints outside the Processes tab and never paints
// for an empty inactive query.
//
// Phase 1.2 slice 12 of the modernization roadmap. Extracted from
// coremetrics.cpp; mirrors the FooterLiveStats / PerCoreStrip shape.
namespace FilterStrip
{
    void render(Screen &dest,
                bool inputActive,
                const std::string &filterText,
                unsigned long long tickMs);
}

#endif
