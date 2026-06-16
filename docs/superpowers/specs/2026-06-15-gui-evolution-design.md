# CoreMetrics: GUI library evolution + repo cleanup

Date: 2026-06-15
Status: Draft, autonomous execution authorized by user

## Tagline

Evolve the from-scratch C++23 widget toolkit so new widget types are easy to add, raw-pointer ownership is replaced with `unique_ptr`, and the monolithic `coremetrics.cpp` is split at natural seams. In parallel, prune unused docs and group source by domain so the repo skim path is shorter.

## Why now

- 30+ feature PRs shipped this week made `coremetrics.cpp` push past 1700 lines (rule of thumb §19 in oop-best-practices: hard look at 500 lines).
- Every new widget requires a fresh `g_xyzBar = new Bar(...)` in `buildScene()` + a manual `delete` in teardown. `unique_ptr` removes the leak surface.
- Reviewer-visible variety is currently three widgets (Bar, Sparkline, Label). htop comparison (`docs/COMPARISON.md`) flags this as a small surface vs. the field.
- A handful of `*.md` files in the repo root no longer pull weight; some duplicate `docs/`.
- `src/` is flat — 30+ `.cpp` files in one directory makes the project look more disorganized than it is.

## Constraints

- C++23, raw SDL3 + SDL3_ttf + SDL3_image only.
- Allman braces, camelCase, `#ifndef __X_HPP__` header guards, no lambdas in app code except `ThreadPool::submit`.
- Author `Sviatoslav Oleksiienko <soleksiienko1@gmail.com>` on every commit.
- Branch flow: every feature branch off `dev`, PR to `dev`. Only `dev -> main` for tag cuts.
- No new third-party deps.
- Cumulative behavior change must be invisible to a user looking at the current dashboard until Phase 3 integration wires new widgets in.

## Phases

### Phase 0 — Repo cleanup (low risk, do first)

**Goal**: shorter README skim path, cleaner repo root.

**Markdown audit**:
- `ADVANTAGES.md` — fold into `docs/ARCHITECTURE.md`. Delete.
- `README.md` — keep.
- `DOCS.md` — keep.
- `API.md` — keep, move to `docs/API.md` (root currently lists 4 separate docs).
- `CONTRIBUTING.md` — keep at root (GitHub convention).
- `CONTRIBUTORS.md` — keep at root.
- `SECURITY.md` — keep at root.
- `CITATION.cff` — keep at root.
- `CHANGELOG.md` — keep at root.
- `CODE_OF_CONDUCT.md` — already lives under `.github/`. Confirm not duplicated at root.
- `OVERNIGHT_BUILD_SPEC.md` — already in `.gitignore`, never tracked. Verify.

**Source structure**:
```
src/
  widgets/        Bar, Button, Label, Row, Sparkline, Box, Image, Line, Point, Selection
  render/         screen, font
  system/         SystemMetrics, SystemMetrics_{mac,linux,win}, ProcParsers, ProcessUtils,
                  HwInfo, Battery, CpuTemp, Notification
  app/            Settings, Exporter, KillLog, TraceLog, Bookmark, SignalUtils, AssetPath
  layout/         Layout, LayoutManager, LayoutUtils, EventManager, Event, ClickEvent,
                  ShowEvent, SoundEvent, GUIElementFactory, GUIElements, GUIFile
  audio/          SoundPlayer
  core/           ThreadPool, matrix, image, selection (if not the Selection widget)
```
Mirror layout under `include/`. `Makefile` already uses `wildcard $(SRCDIR)/*.cpp` so it needs one tweak to recurse: `wildcard $(SRCDIR)/**/*.cpp` plus a fallback for the flat layout during transition.

This is a high-conflict move. Defer until current PR queue (#152-162) is merged.

### Phase 1 — GUI library foundation refactor

**Goal**: cleaner ownership and smaller TUs without changing behavior.

**1.1 `unique_ptr` widget ownership**
- Convert `g_cpuBar`, `g_ramBar`, `g_gpuBar`, `g_cpuReadout`, `g_ramReadout`, `g_gpuReadout`, `g_pollLabel`, `g_muteLabel`, `g_headerRow` from raw `T*` to `std::unique_ptr<T>`.
- `cacheElementPointers()` keeps raw `Bar*` / `Label*` views, populated from the tree, NOT owners.
- `buildScene()` constructs via `std::make_unique`.
- `destroySparklines()` and the existing teardown helpers stop needing manual `delete`.

**1.2 Widget interface split**
- Today `GUIElement` declares both `draw()` and `operator()(Event*)`.
- Extract `IDrawable { virtual void draw(Screen&) = 0; }` and `IInteractive { virtual bool operator()(Event*) = 0; }`.
- `GUIElement` becomes `GUIElement : IDrawable, IInteractive`.
- Widgets that only draw (Bar, Sparkline, Label) gain a `NoOpInteractive` mixin so `GUIElement* -> operator()` keeps compiling.
- Goal: future widgets that are purely visual (e.g., Gauge, Heatmap) can implement just `IDrawable`.

**1.3 `coremetrics.cpp` split**
Natural seams:
- `src/app/SystemTabRender.cpp` — `renderUptimeAndLoad`, `renderDiskUsage`, `renderAggregateDiskIo`, `renderNetIo`, `renderMemBreakdownStrip`, `renderPerCoreStrip`, `renderSparklineLabels`.
- `src/app/ProcessesTabRender.cpp` — `renderProcessesSummary`, the selected-row highlight, the sort glyph helper, the filter input strip.
- `src/app/ChromeRender.cpp` — `renderFooterLiveStats`, `renderHelpOverlay`, status flash.
- `src/app/EventHandlers.cpp` — the SDL_EVENT_KEY_DOWN big switch, the mouse handler, signal-menu key bindings.
- `src/app/MainLoop.cpp` — `pollMetrics`, the tick loop body.
- `coremetrics.cpp` keeps `int main`, argv parsing, SDL_Init, screenshot fast path, and signal handlers (`< 400` lines target).

### Phase 2 — New widgets

Each widget = one PR. New widgets only; no integration yet (Phase 3 wires them).

| Widget | One-line | Files | Reviewer impact |
|---|---|---|---|
| ProgressRing | Circular percent display | include/widgets/ProgressRing.hpp, src/widgets/ProgressRing.cpp | high |
| Gauge | Semicircle dial 0..100 | include/widgets/Gauge.hpp, src/widgets/Gauge.cpp | high |
| Toggle | On/off switch | include/widgets/Toggle.hpp, src/widgets/Toggle.cpp | medium |
| Slider | Drag handle on a track | include/widgets/Slider.hpp, src/widgets/Slider.cpp | medium |
| Dropdown | Click expands list of options | include/widgets/Dropdown.hpp, src/widgets/Dropdown.cpp | medium |
| TabStrip | First-class tab widget (replaces base.xml tabs in Phase 3) | include/widgets/TabStrip.hpp, src/widgets/TabStrip.cpp | high |
| TreeView | Generic indented tree of rows | include/widgets/TreeView.hpp, src/widgets/TreeView.cpp | medium |
| Tooltip | Hover-triggered floating label | include/widgets/Tooltip.hpp, src/widgets/Tooltip.cpp | medium |
| Modal | Centered dialog with overlay | include/widgets/Modal.hpp, src/widgets/Modal.cpp | medium |
| Heatmap | 2D grid of colored cells | include/widgets/Heatmap.hpp, src/widgets/Heatmap.cpp | high |
| Donut | Circular percent with center label | include/widgets/Donut.hpp, src/widgets/Donut.cpp | medium |
| StackedBar | Bar with N colored segments | include/widgets/StackedBar.hpp, src/widgets/StackedBar.cpp | medium |
| TextField | Single-line input (read filter is the precedent) | include/widgets/TextField.hpp, src/widgets/TextField.cpp | medium |
| IconButton | Button with an SVG glyph instead of text | include/widgets/IconButton.hpp, src/widgets/IconButton.cpp | low |
| SparklineCursor | Newest-sample marker + hover-to-show value | extends Sparkline + new include/widgets/SparklineCursor.hpp | medium |

Each agent's hard scope is its own two files. Each widget extends `Cloneable<T>` like the rest, implements `draw(Screen&)`, and (if interactive) `operator()(Event*)`.

Each agent ships a matching test suite under `tests/widgets/<Name>Test.cpp`.

### Phase 3 — Integration

Wire selected new widgets into the dashboard. Each is a separate PR.

1. Replace the base.xml tab buttons with the new `TabStrip` widget. Removes hand-coded tab geometry from the XML.
2. Add a CLI flag `--gauges` that swaps the System-tab Bars for `Gauge` semicircles.
3. Add a `Heatmap` view for the per-core CPU strip behind `--heatmap` (current `--sparklines` style flag).
4. Add a `Tooltip` triggered on Processes-tab row hover showing the full process name + PID + cwd.
5. Add a `Modal` confirmation for `--quit` to replace the current EXIT button's direct-kill.

## Failure modes + mitigations

| Mode | Mitigation |
|---|---|
| Phase 1 split breaks the build mid-PR | One PR per file moved; small atomic diffs; CI gate on all three OSes. |
| Conflict storm from parallel agents | Each new widget is two new files. Zero shared TU. Phase 1 is mostly mine sequentially. |
| Behavior change accidentally lands in Phase 1 | Pre/post screenshot diff (already wired by CI smoke); pre/post `./bin/tests` parity. |
| Repo cleanup loses real history | Only delete files we wrote ourselves; `git mv` for source reshuffles (preserves blame). |
| Restructure breaks IDE indexers | Compile-commands JSON regenerates from Make; check after the move. |

## Testing strategy

- Each new widget gets a `tests/widgets/<Name>Test.cpp` covering construction, the draw smoke path against a 100x100 `Screen`, and the interactive path if applicable.
- Phase 1 (refactor) gates on: existing `./bin/tests` is byte-identical green; headless `--screenshot system.png` PNG sha256 is byte-identical to the pre-refactor PNG for the same `--duration` seed; pre/post `./bin/bench` numbers within 5%.
- Phase 3 (integration) widgets gain a screenshot in the CI smoke set so the artifact bundle shows the new widget rendered.

## Out of scope

- Window resize support (already on `docs/ROADMAP.md`).
- Light theme palette (`Theme::set("light")`, also on roadmap).
- Non-mouse input methods beyond what `--filter`/`--tree` already do.
- Network monitoring beyond rx/tx aggregate.

## Rollout

1. Land Phase 0 (md cleanup + maybe structure) as a single PR after current queue (#152-162) clears.
2. Phase 1 refactor: 3-4 sequential PRs (own).
3. Phase 2: 15 parallel agent PRs (worktrees, disjoint files).
4. Phase 3: 5 sequential PRs (own).
5. Tag `v0.3.0` once all three phases land — minor bump signals the new widget set.

## Decision log

- Picked unique_ptr over shared_ptr: widgets have a single owner (the LayoutManager tree or a global slot), not shared lifetime.
- Kept singletons (LayoutManager, EventManager) for now; conversion to DI is a v0.4 concern. Today every widget touches them and DI plumbing is larger than the benefit at this stage.
- Did NOT propose flexbox-like layout: the hand-coordinate model in base.xml is the project's signature; moving away from it removes a Neo Scholars reviewer talking point.
