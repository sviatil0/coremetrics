#ifndef __SIGNALMENUOVERLAY_HPP__
#define __SIGNALMENUOVERLAY_HPP__

#include "screen.hpp"

// Paints the centered "Send signal to pid N" modal that appears when
// the user presses `k` over a selected row on the Processes tab. The
// overlay has two states driven by pickedIndex:
//   pickedIndex <  0 -> picker mode  ("1 TERM  2 KILL  3 INT ...")
//   pickedIndex >= 0 -> confirm mode ("Send TERM to pid N? Y/N")
//
// The gate (g_signalMenuVisible) stays in the caller so this helper
// has no opinion on visibility; if render() is called, the modal
// paints. Palette routes through Theme tokens already migrated to the
// Tokyo Night scheme by PR #216 (panelBg, accent, textPrimary,
// textDim) plus accentGreen() for the hotkey-list line so it stays
// distinguishable from the prompt text.
//
// Phase 1.2 slice 11 of the GUI evolution spec. Extracted from
// coremetrics.cpp; mirrors the file shape of ProcessesSummary.
namespace SignalMenuOverlay
{
    void render(Screen &dest, int selectedPid, int pickedIndex);
}

#endif
