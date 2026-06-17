# CoreMetrics Modernization Roadmap

Author: Sviatoslav Oleksiienko
Drafted: 2026-06-16
Status: active, multi-week plan

This document is the single source of truth for the long-running
modernization push. It supersedes ad-hoc TODO lists in chat and is
amended in-tree so future agents pick up exactly where the last one
left off.

The Neo Scholars submission already proves the engineering thesis
(C++23, no framework, custom rasterizer, cross-platform). The next
push is about making the artifact feel as deliberate as the code that
built it. Modern visual language. No clutter. Every pixel intentional.

## Why now

Three signals converge:

1. **User feedback** has been consistent across sessions: "still
   shitty", "cluttered", "poor visibility". Iterative tweaks (stroke
   width, fill alpha) hit a ceiling because the underlying palette is
   ~5% gray on ~5% gray. No micro-fix recovers the contrast budget.
2. **Phase 1.2 file split** is now half done (PRs #200-#206 merged,
   coremetrics.cpp 2425 -> 2183). Continuing the slice cadence is the
   right backbone but it does not visibly change the product. Pairing
   it with a visual track gives every release something the user can
   *see*.
3. **v0.3 is the natural release horizon** for the unintegrated widget
   inventory (TabStrip, Tooltip, Modal, Gauges, Heatmap, StackedBar,
   Toggle, Slider, Dropdown, TreeView). They exist in the codebase
   but no tab actually instantiates them. Cutting v0.3 with a fresh
   palette + actually-wired widgets is a coherent story.

## Pillars

The roadmap has four pillars. Each pillar has its own pipeline. They
run in parallel because they touch disjoint files most of the time.

### Pillar A: visual language

Move CoreMetrics from "developer terminal" aesthetic to "modern
analytics dashboard" aesthetic without giving up the dark/dense feel
that suits a system monitor.

Stages:

- A1. Theme refresh. New palette inspired by Tokyo Night / GitHub
  Dark. Background separates from panels; accents read against both.
  Every render call already routes through Theme::*, so this is a
  single-file change with a global payoff.
- A2. Typography hierarchy. Today everything paints at the same size.
  Define h1/h2/body/caption sizes and apply them. The custom Font
  layer supports it already.
- A3. Spacing grid. Adopt an 8 px increment everywhere. Audit the
  hand-tuned y-coordinates in renderer call sites and snap them to
  the grid.
- A4. Card / panel system. Wrap every metric group in a softly
  rounded panel with a 1 px border in the new "border" token. Gives
  the dashboard structure without busy chrome.
- A5. Subtle motion. 150 ms ease-out on tab switches, hover lifts
  on actionable elements. Single global tween clock; no per-widget
  state machines.
- A6. Empty / loading states. Today empty processes shows nothing.
  Replace with a tasteful "no data yet" hint.

### Pillar B: widget integration (Phase 2 of the GUI spec)

The codebase ships twelve v0.3-era widget classes that nothing renders
to. Wire them into the actual tabs.

Stages:

- B1. TabStrip replaces the four base.xml tab buttons. Removes the
  pixel-fudged y=8..40 hit boxes; the strip computes its own.
- B2. Tooltip on every truncated row cell (Processes tab) so long
  command lines stay discoverable.
- B3. Modal confirm-quit. Q today is destructive; modal asks first.
- B4. Gauge wired into the System tab as the CPU at-a-glance dial.
- B5. Heatmap of per-core CPU history (NxT matrix) under the cores
  strip.
- B6. StackedBar replaces the memory composition strip painted by
  src/MemBreakdownStrip.cpp.
- B7. Toggle drives the SOUND ON/OFF chrome.
- B8. Slider drives the poll-interval CLI flag at runtime.
- B9. Dropdown for the --top-sort cpu/mem/io selection at runtime.
- B10. TreeView for the Processes tab tree mode.

### Pillar C: refactor backbone (Phase 1.2)

Keep peeling render helpers out of coremetrics.cpp until the main TU
is under 1500 lines. Each slice is one PR.

Tracked slices (done):
- #200 HelpOverlay
- #201 SparklineLabels
- #202 UptimeAndLoad
- #203 drop renderAggregateDiskIo no-op
- #204 NetIoFooter
- #205 DiskUsageRow
- #206 MemBreakdownStrip

Remaining slices:
- C1. PerCoreStrip
- C2. FooterLiveStats
- C3. ProcessesSummary
- C4. ProcessesTable (header + rows)
- C5. KillLogTable
- C6. TraceLogTable
- C7. AboutPanel
- C8. ChromeFrame (top + bottom chrome bars + EXIT)
- C9. EventHandlers (mouse/keyboard dispatcher)
- C10. MainLoop (final residual)

### Pillar D: quality & release

Ship-grade work on the periphery so the product moves up release
shelves cleanly.

Stages:

- D1. clang-format the tree under .clang-format so the advisory job
  flips to required. Single large PR; downstream PRs auto-format.
- D2. clang-tidy fixes for actionable findings. Address them one
  category at a time.
- D3. Test coverage push: every widget gets a render-to-screen unit
  test that asserts on the bitmap.
- D4. Sanitizer-clean: investigate any TSan/ASan/UBSan complaints
  from the existing CI legs.
- D5. Win11 ARM live test once the user-side ISO download finishes
  (paused).
- D6. v0.3 release cut. Tag v0.3.0 when A1+A4, B1+B2+B3, and
  C1..C5 land. Brew bump. Screenshot refresh. Docs refresh. UML
  refresh.

## Cadence

One pillar-A or pillar-C PR per cycle, opened against `dev`. Wait
for the three OS legs + sanitizers + coverage before merging.
Advisory clang-format/clang-tidy failures are non-blocking until
D1 lands. Pillar B PRs depend on A1..A4 and so start after A1
merges.

Within a single cycle the order is: theme/UI lift first, refactor
next, then any test/docs/release activity. This keeps every
release notable on its own.

## Done definition

This roadmap is done when:

- coremetrics.cpp is under 1500 lines and split at all C-stages.
- All 12 unwired widgets either ship in a tab or get pulled from
  the inventory.
- A user new to the project would describe the UI as "modern"
  rather than "dense terminal".
- v0.3.0 is tagged, brewed, and the README screenshots match the
  v0.3 binary.
