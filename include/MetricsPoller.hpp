#ifndef __METRICSPOLLER_HPP__
#define __METRICSPOLLER_HPP__

// Pulled out of coremetrics.cpp as Phase 1.2 slice 15 of the
// modernization roadmap, mirroring the KeyboardEvents (slice 13)
// and MouseEvents (slice 14) extractions that landed just before.
// poll() owns every per-tick metrics refresh the live UI depends on:
// CPU/RAM/GPU sampling, threshold-based bar recolor, sparkline
// pushes, per-core CPU, memory breakdown, uptime + load averages,
// root-disk usage, network rx/tx, alarm flash bookkeeping, and the
// full process-table refresh (fetch, optional tree-mode flatten,
// filter, sort, scroll-window slice, row-cell rewrite, tree-mode
// glyph overlay state, selected-pid re-anchor).
//
// The function reads and mutates a long list of file-scope globals.
// Globals whose primary writer is the poller (live metric samples,
// alarm-active flags, glyph overlay parallel arrays, last-poll
// summary counters) moved to MetricsPoller.cpp so the new TU owns
// the data it produces. Globals the poller only consumes (the scene
// widget pointers, the keyboard/mouse handler state machine) stay
// in their existing TUs and are reached via extern. The wider C9
// EventHandlers / state-machine slices will continue tightening the
// surface.
namespace MetricsPoller
{
    void poll();
}

#endif
