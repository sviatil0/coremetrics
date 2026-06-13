<div align="center">

<img src="assets/logo.png" alt="CoreMetrics logo" width="360">

# CoreMetrics

**Real-time cross-platform system monitor (CPU / RAM / GPU / processes), built on a from-scratch C++23 GUI library over raw SDL3 surfaces.**

[![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/23)
[![SDL3](https://img.shields.io/badge/SDL-3-1a1a1a)](https://www.libsdl.org/)
[![Platforms](https://img.shields.io/badge/platforms-macOS%20%7C%20Linux-informational)](#architecture)
[![License: LGPL-2.1](https://img.shields.io/badge/License-LGPL--2.1-blue.svg)](LICENSE)
[![C/C++ CI](https://github.com/sviatil0/coremetrics/actions/workflows/c-cpp.yml/badge.svg?branch=main)](https://github.com/sviatil0/coremetrics/actions/workflows/c-cpp.yml)
[![Stefan's code](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Fsviatil0%2Fcoremetrics%2Fmain%2F.github%2Fbadges%2Fcontribution.json)](#team-and-my-contribution)
[![Latest release](https://img.shields.io/github/v/release/sviatil0/coremetrics?label=release)](https://github.com/sviatil0/coremetrics/releases/latest)

</div>

| System tab | Processes tab |
|:---:|:---:|
| ![System tab](assets/screenshot-system.png) | ![Processes tab](assets/screenshot-processes.png) |
| CPU / RAM / GPU bars, load-colored (RAM red past 80%), plus rolling sparklines under `--sparklines` | Sortable PID / NAME / CPU% / MEM% table |

> Both frames are rendered by the app itself, headlessly: `coremetrics --screenshot out.png [system|processes]` runs one render pass to an offscreen surface and saves it, no window required. The extension picks the writer (`.png` via `IMG_SavePNG`, anything else via `SDL_SaveBMP`).

New here? [**DOCS.md**](DOCS.md) maps the whole repo; [**API.md**](API.md) is the full public library reference (every class and method).

---

## Why this is interesting

CoreMetrics is two things in one repo: a small **GUI toolkit written directly on SDL3 pixel surfaces** (no Dear ImGui, no Qt, no game framework) and a **real system monitor built on top of it**. A few things worth a look:

- **Three native metrics backends, one header.** `SystemMetrics` reads live data from `/proc` + `/sys` on Linux, mach + IOKit on macOS, and PDH + Toolhelp on Windows, selected at compile time via `#ifdef`.
- **From-scratch UI stack.** Every widget rasterizes itself onto a raw `SDL_Surface` through one `Screen` primitive layer (`drawPixel`, Bresenham `drawLine`, `drawBox`, `drawTriangle`, `blitTo`). No retained-mode GUI library underneath; geometry is hand-rasterized, text and image decode go through SDL's debug renderer and SDL_image/SDL_ttf.
- **Event-driven, no scene rebuilds.** Clicks trickle top-down through the layout tree; tab switches drain as paired show/hide events in a single pass; metrics mutate widgets in place every 500 ms.
- **Modern C++ on purpose.** A `Cloneable<Derived>` CRTP mixin gives every widget a covariant `clone()` for free; ownership flows through `unique_ptr`; the layout tree is a generic `Tree<T>`.
- **Parallel fills.** Wide `drawBox` / `drawTriangle` operations partition pixel rows across a `ThreadPool` and join on `std::future`s per frame (a teammate's work; see the contribution table).
- **214 unit tests across 16 suites** and a Linux + macOS GitHub Actions matrix, plus a non-blocking AddressSanitizer + UndefinedBehaviorSanitizer leg (`make asan`, `make ubsan`).

> This is a 4-person team project, and I was the lead and primary author. The "Stefan's code" badge at the top is computed by a CI job that runs `git blame -w -C -M` across `src/`, `include/`, `bench/`, and `coremetrics.cpp` on every push to `main`, so the percentage is always current and never hand-typed. See [Team and my contribution](#team-and-my-contribution) for the per-file breakdown, and `scripts/compute-contributions.sh` for the exact logic.

## Install (prebuilt)

No build toolchain required.

```bash
# macOS (Homebrew, Apple Silicon)
brew tap sviatil0/coremetrics
brew trust sviatil0/coremetrics   # one-time: Homebrew requires explicit trust for third-party taps
brew install coremetrics

# Debian / Ubuntu
curl -L https://github.com/sviatil0/coremetrics/releases/latest/download/coremetrics_amd64.deb -o /tmp/coremetrics.deb
sudo apt install /tmp/coremetrics.deb

# Any platform (tarball)
curl -LO https://github.com/sviatil0/coremetrics/releases/latest/download/coremetrics-<version>-<platform>.tar.gz
tar xf coremetrics-<version>-<platform>.tar.gz
cd coremetrics-<version>-<platform>
./coremetrics
```

The Homebrew formula pulls SDL3 + SDL3_ttf + SDL3_image as dependencies. The `.deb` declares the equivalent apt dependencies. The tarball assumes SDL3 is already installed.

## Quickstart (from source)

Requires a **C++23 compiler** (g++ 13+ or clang 16+), GNU Make, and **SDL3 + SDL3_ttf + SDL3_image**.

```bash
# 1. clone
git clone https://github.com/sviatil0/coremetrics.git
cd coremetrics

# 2. install the SDL3 dependencies
brew install sdl3 sdl3_ttf sdl3_image          # macOS
# sudo apt install libsdl3-dev libsdl3-ttf-dev libsdl3-image-dev   # Debian/Ubuntu

# 3. build and run the demo
make                 # builds bin/coremetrics and launches it

# 4. (optional) watch the bars spike
./stress.sh          # 30s of CPU + RAM load; bars cross yellow/red thresholds

# (optional) render a frame headlessly, no window needed
./bin/coremetrics --screenshot shot.png             # System tab
./bin/coremetrics --screenshot shot.png processes   # Processes tab
./bin/coremetrics --sparklines                      # adds rolling CPU/RAM/GPU charts
./bin/coremetrics --duration 5                      # auto-quit after 5s (for CI smoke tests)
```

<details>
<summary>Other build targets, Windows notes, and SDL-from-source</summary>

```bash
make coremetrics   # build + run the CoreMetrics demo (same as default)
make test          # build + run the full unit-test suite
make demo          # build + run the Milestone 005 event demo
make clean         # remove obj/ and bin/
```

```bash
./stress.sh 60 8 1024    # custom: duration(sec) cpu-workers ram-MB
```

**Windows:** download the SDL3, SDL3_ttf, and SDL3_image dev libraries from the [SDL release pages](https://github.com/libsdl-org/SDL/releases), extract them, and point `CXXFLAGS` / `LDFLAGS` in the `Makefile` at your include/lib directories.

**SDL3 not packaged yet on your distro?** Build from source:
[SDL](https://github.com/libsdl-org/SDL/releases) ·
[SDL_ttf](https://github.com/libsdl-org/SDL_ttf/releases) ·
[SDL_image](https://github.com/libsdl-org/SDL_image/releases)

Optional GPU-stress tooling: `glmark2` or `stress-ng --gpu` on Linux; on macOS the script drives a WebGL page in the browser (no install). If none are present, CPU and RAM stress still run and GPU stress is skipped with a log line.
</details>

## Architecture

The system is four layers. SDL3 hands us a window and a raw pixel surface; `Screen` turns that surface into drawing primitives; the `GUIElement` / `Layout` tree composes those primitives into a UI; and `EventManager` routes input back down the tree.

```mermaid
flowchart TD
    classDef mine fill:#1b5e20,stroke:#a5d6a7,color:#ffffff
    classDef ext fill:#0d47a1,stroke:#90caf9,color:#ffffff
    classDef note fill:#37474f,stroke:#b0bec5,color:#ffffff

    subgraph APP["APP"]
        Main["coremetrics.cpp<br/>build scene, 500ms poll loop, render+blit each frame"]
    end

    subgraph L3["LAYER 3 - composition"]
        LM["LayoutManager (singleton)<br/>owns Tree of Layout, painter's algorithm"]
        Layout["Layout regions"]
        GUIFile["GUIFile<br/>XML load / save"]
    end

    subgraph L2["LAYER 2 - widgets"]
        GE["GUIElement hierarchy<br/>Bar Row Label Button Image Selection<br/>each draw(Screen&)"]
        Factory["GUIElementFactory + Cloneable CRTP"]
    end

    subgraph L1["LAYER 1 - graphics core"]
        Screen["Screen rasterizer<br/>drawPixel line box triangle text blit"]
        Pool["ThreadPool<br/>parallel pixel rows"]
        Math["Math: Tvec2 Tvec3 Matrix"]
    end

    subgraph L0["LAYER 0 - platform (external)"]
        SDL["SDL3 + SDL3_ttf + SDL3_image<br/>window + SDL_Surface + audio"]
    end

    subgraph EVENTS["EVENTS (cross-cutting)"]
        EM["EventManager (singleton)<br/>Click Show Sound queue"]
        Sound["SoundPlayer"]
    end

    subgraph METRICS["METRICS (cross-cutting data source)"]
        Metrics["SystemMetrics<br/>CPU RAM GPU per OS ifdef"]
    end

    subgraph LEGEND["LEGEND"]
        LK["green = my code"]
        LX["blue = external (SDL3)"]
    end

    %% main render spine
    Main -->|render| LM
    LM --> Layout
    Layout --> GE
    LM -->|painter parent before child, calls draw| GE
    GE -->|draw| Screen
    Screen --> Pool
    Screen -->|owns| SDL
    Main -->|owns + blitTo window| Screen

    %% persistence
    GUIFile -.->|load / save| Layout
    Factory -.->|builds| GE
    Math -.->|underpins| Screen

    %% events cross-cutting
    Main -->|process| EM
    EM -.->|trickle top down| GE
    EM -.->|SoundEvent| Sound
    Sound -->|audio| SDL

    %% metrics cross-cutting
    Main -->|poll 500ms| Metrics
    Metrics -.->|mutates Bars Rows Labels| GE

    class Main,LM,Layout,GUIFile,GE,Factory,Screen,Pool,Math,EM,Sound,Metrics,LK mine
    class SDL,LX ext
    class LEGEND note
```

| Layer | Files | Responsibility |
|---|---|---|
| **Math core** | `vec2.hpp`, `vec3.hpp`, `matrix.hpp`, `linear.hpp` | Templated (int/float) vectors and a 3x3 `Matrix`; dot, cross, magnitude, unit, transpose |
| **Rasterizer** | `screen.hpp` / `screen.cpp`, `ThreadPool.hpp` | Draw primitives onto an `SDL_Surface`; parallelize wide fills |
| **Widgets** | `GUIElement.hpp`, `Cloneable.hpp`, `Bar`, `Row`, `Label`, `Button`, `Image`, `selection` | Self-drawing UI elements behind one polymorphic `draw(Screen&)` interface |
| **Layout** | `Tree.hpp`, `Layout.hpp`, `LayoutManager.hpp`, `GUIFile.hpp` | Relative-coordinate layout tree, painter's-algorithm render, XML load/save |
| **Events** | `Event*.hpp`, `EventManager.hpp`, `SoundPlayer.hpp` | Queue, trickle dispatch, layout show/hide, WAV playback |
| **Metrics** | `SystemMetrics.hpp` + `SystemMetrics_{linux,mac,win}.cpp` | Live CPU / RAM / GPU / process stats per OS |
| **App** | `coremetrics.cpp` | The CoreMetrics system-monitor demo built on the above |

Per-package class diagrams (PlantUML): [Core](assets/core.png) · [GUI](assets/gui.png) · [Layout](assets/layout.png) · [Events](assets/events.png) · [Metrics](assets/metrics.png) · [combined overview](assets/overview.png).

## The demo

`coremetrics.cpp` builds a two-tab system monitor on the GUI library:

- **System tab:** CPU and RAM bars with live numeric readouts. Bars recolor yellow above 60% and red above 80% to flag load at a glance.
- **Processes tab:** a header row plus configurable data rows listing PID / NAME / CPU% / MEM%, sorted by memory usage.

Tab switching is event-driven: each tab button emits a hide event for the other tab and a show event for its own, both drained in one `processEvents` pass so the switch is atomic. Metrics refresh every 500 ms; the main loop walks the layout tree and mutates bars, rows, and labels in place rather than rebuilding the scene.

## Status

**What works**

- GUI library, rasterizer, event system, layout tree, and the CoreMetrics demo build and run.
- Live CPU / RAM and per-process stats on macOS, Linux, and Windows.
- Total GPU usage on Linux (`gpu_busy_percent`), macOS (`IOAccelerator`), and Windows (PDH).
- 175 unit tests across 13 suites; Linux + macOS compile + test matrix in CI (Windows builds locally only).

**Known limitations**

- **Per-process GPU attribution** is not exposed by the cross-platform API yet; only total GPU usage is reported. (NVIDIA NVML on Linux is a backlog item.)
- **Windows GUI is not visually verified end-to-end.** The Windows metrics layer exists, but mouse/click/tab interaction is verified on macOS and Linux only, so CI is gated on Linux + macOS (the Windows SDL3 + pkg-config toolchain on the runner is also flaky). Local cross-platform verification runs via `./run-cross-platform-tests.sh` (macOS native + Ubuntu in Docker).

## API reference

The math core is the most reused surface; the full per-class reference is folded below to keep this page skimmable.

| Type | Selected public API |
|---|---|
| `Tvec2<T>` / `Tvec3<T>` | `dot`, `magnitude`, `unit`, `cross` (vec3), and `==`, `+`, `-`, `*`, `+=`, `-=`, `*=`, `[]`; implicit int↔float conversion. `vec2`/`vec3` = float, `ivec2`/`ivec3` = int |
| `Matrix` | `operator*` (3x3 multiply), `operator==`, `toTranspose` |
| `Screen` | `drawPixel`, `drawLine`, `drawBox`, `drawTriangle`, `drawText`, `blitTo`, `clear` |
| `Tree<T>` | `getData`, `getParent`, `getChildren`, `addChild`, `isRoot`, `isLeaf` |
| `EventManager` | `getInstance`, `pushEvent`, `processEvents` |
| `SystemMetrics` | `readCpuPercent`, `readMemPercent`, `topProcesses(n)` |

<details>
<summary><strong>Full class-by-class reference</strong></summary>

### Math

**`Matrix`**: a 3x3 matrix of floats stored as a 2D array.
- `Matrix operator*(const Matrix&) const`: matrix multiply.
- `bool operator==(const Matrix&) const`: equality.
- `Matrix toTranspose() const`: returns the transpose.

**`Tvec2<T>`**: templated (int or float) 2-component vector. Overloads `==`, `+`, `-`, `*`, `+=`, `-=`, `*=`, `[]`, with implicit int↔float conversion.
- `T dot(const Tvec2<T>&) const`: dot product.
- `T magnitude() const`: Euclidean length for float, L1 (Manhattan) for int.
- `Tvec2<T> unit() const`: unit vector.

**`Tvec3<T>`**: templated 3-component vector with the same operator set as `Tvec2`.
- `T dot`, `T magnitude`, `Tvec3<T> unit`: as above, extended to three components.
- `Tvec3<T> cross(const Tvec3<T>&) const`: cross product.

### Rasterizer

**`Screen`**: renders geometric primitives onto an `SDL_Surface`.
- `void drawPixel(ivec2 pos, vec3 color)`: color a single pixel.
- `void drawLine(ivec2 start, ivec2 end, vec3 color)`: horizontal, vertical, or diagonal line.
- `void drawBox(ivec2 min, ivec2 max, vec3 color)`: filled rectangle.
- `void drawTriangle(ivec2 v1, ivec2 v2, ivec2 v3, vec3 color)`: filled triangle, winding-agnostic.
- `void drawText(ivec2 pos, vec3 color, std::string text)`: text via `SDL_RenderDebugText` (SDL3 built-in debug font). The bundled TTF is used by `Font::drawText`, which `Label` calls.
- `void blitTo(SDL_Surface*)`: copy the internal buffer to a display surface.

**`ThreadPool`**: singleton worker pool sized to `std::thread::hardware_concurrency`. `drawBox` / `drawTriangle` partition pixel rows across it via `submit` and join on returned futures. Copy/assign deleted; destructor signals stop and joins.
- `static ThreadPool& getInstance()`, `size_t threadCount() const`, `template<typename F> std::future<void> submit(F&&)`.

### Widgets

**`GUIElement`**: abstract base for every renderable UI component.
- `virtual void draw(Screen&) = 0`: render against a `Screen` (decoupled from the target).
- `virtual bool operator()(Event*)`: event handler; returns `true` if the event is consumed.
- `virtual GUIElement* clone() const = 0`: deep copy, supplied by `Cloneable`.

**`Cloneable<Derived>`**: CRTP mixin giving subclasses `clone()` for free.
- `GUIElement* clone() const override`: heap copy via the derived copy constructor.
- `Derived* cloneDerived() const`: covariant return, no caller cast.
- `template<typename T> std::unique_ptr<T> cloneUnique(const T&)`: wraps a clone in `unique_ptr`.

**`Point` / `Line` / `Box`**: concrete `GUIElement`s for a pixel, a segment, and a filled rectangle; each `draw` forwards to the matching `Screen` primitive.

**`GUIElementFactory`**: static factory mapping a `GUIElementType` to a concrete element.
- `createPoint`, `createLine`, `createBox`, and a generic `create(type, pos1, pos2, color)` (returns `nullptr` + logs to `std::cerr` on unknown types).

**`Label`**: text component; renders its text via the bundled font (`Font::drawText`, TTF). `draw`, `getText`, `setText`, `setPos`.

**`Selection`**: stateful checkbox (border, background, conditional check mark). `draw`, `toggle`, `isSelected`.

**`Image`**: renders a BMP by manually plotting non-transparent pixels (RGBA8888) rather than using SDL blit helpers. `draw`, `getFilePath`.

**`Button`**: clickable box with optional sound and target-layout. On a hit it pushes a `SoundEvent` and/or `ShowEvent`. `draw`, `checkToggle(x, y)`, `operator()(Event*)`.

**`Bar`**: horizontal progress bar over `[minVal, maxVal]`; recolors yellow above 60% and red above 80%; optional `metricName` tag. `setValue`, `getValue`, `getMetricName`, `draw`.

**`Row`**: horizontal strip of N text cells with caller-supplied column weights; cells truncate to width. Used for the process table. `setCells`, `getCells`, `draw`.

### Layout

**`Tree<T>`**: generic tree; each node owns `unique_ptr` children and holds a non-owning parent pointer. `getData`, `getParent`, `getChildren`, `addChild`, `isRoot`, `isLeaf`.

**`Layout`**: a screen region in relative (0.0 to 1.0) coordinates; holds a flat list of `GUIElement`s. Deep-copies its elements via `Cloneable::clone()`. `resolveAbsStart/End`, `addElement`, `setActive`, `isActive`, `getStart/End`, `getName`, `draw`.

**`LayoutManager`**: singleton owning the root `Tree<Layout>`; renders with the painter's algorithm (depth-first, parent before children). `getInstance`, `getRoot`, `addChild`, `render`.

**`GUIFile`**: loads and saves the GUI layout tree to and from a file via `readFile` / `writeFile`; the parse helpers are private.

### Events

**`Event`**: abstract message carrying an `EventType`. `getType`.
**`ClickEvent`**: carries click pixel coordinates. `getMouseX`, `getMouseY`.
**`ShowEvent`**: targets a named layout to show/hide. `getLayoutName`, `getShow`.
**`SoundEvent`**: requests WAV playback. `getFilePath`.
**`EventManager`**: singleton owning the event queue; click events trickle top-down, show events toggle a named layout, sound events delegate to `SoundPlayer`. `getInstance`, `pushEvent`, `processEvents`.
**`SoundPlayer`**: singleton WAV playback through SDL3 (44.1 kHz, mono, float32). `getInstance`, `play`.

### Metrics

**`SystemMetrics`**: static cross-platform reader; platform-agnostic header, `#ifdef`-guarded impls:
- Linux: `/proc/stat`, `/proc/meminfo`, `/proc/[pid]/{comm,stat,status}`.
- macOS: `host_statistics(64)`, `sysctl(HW_MEMSIZE)`, `proc_listpids` / `proc_pidinfo` / `proc_name`.
- Windows: `GetSystemTimes`, `GlobalMemoryStatusEx`, `CreateToolhelp32Snapshot` + `GetProcessMemoryInfo` / `GetProcessTimes`.
- `static float readCpuPercent()` (first call returns `0.0f`, no baseline), `static float readMemPercent()`, `static std::vector<ProcessInfo> topProcesses(std::size_t n = 20)`.

</details>

<!-- contribution:start -->

## Team and my contribution

A 4-person Notre Dame CSE 40232 software-engineering project (SP26 Team 04), three months, a PR-template + required-review workflow with per-developer branches. I was the lead and primary author: **73.0% of the source by line** (git-blame verified, recomputed on every push to `main`).

_The block below is regenerated by [`scripts/compute-contributions.sh`](scripts/compute-contributions.sh) on every push to `main` via the `Contribution badge` workflow. Don't edit between the markers — your edit will be overwritten._

### Lines of code by author

```mermaid
pie showData
    title Source line share across src/, include/, bench/, coremetrics.cpp
    "Sviatoslav Oleksiienko (me)" : 3614
    "Alicia Melotik" : 863
    "Martin Castelluccio" : 471
```

| Author | Lines | Share |
| --- | ---: | ---: |
| **Sviatoslav Oleksiienko (me)** | 3614 | `███████████████░░░░░` 73.0% |
| Alicia Melotik | 863 | `███░░░░░░░░░░░░░░░░░` 17.4% |
| Martin Castelluccio | 471 | `██░░░░░░░░░░░░░░░░░░` 9.5% |

_Run \`scripts/compute-contributions.sh\` locally to reproduce these numbers from your checkout._

<!-- contribution:end -->

### Who owned what

| Contributor | What they owned |
|---|---|
| **Sviatoslav (me)** | All three `SystemMetrics` platform backends (`/proc`, IOKit, PDH), the event system, the GUI element hierarchy and factory, the `Layout` / `LayoutManager` tree, the Bresenham line rasterizer (`plotLineLow` / `plotLineHigh`) plus `blitTo` / `clear`, the CoreMetrics demo app, the test suites, CI, and tooling. After v0.1.0: `ProcParsers`, `ProcessUtils` helpers, `RingBuffer<T>` + `Sparkline`, `AssetPath` resolver, lifecycle (`--duration`, signal handlers), benchmark, the release pipeline, the Homebrew tap. |
| **Alicia Melotik** | The XML parser and `GUIFile`, the `Cloneable` / CRTP clone path, `drawPixel` / `drawLine` / `drawText` entry points, Windows GPU, layout helpers. |
| **Martin Castelluccio** | The `ThreadPool` and the parallel fills in `drawBox` / `drawTriangle`, plus `Button` event handling. |
| **Daniel Rehberg** (instructor) | Early scaffolding and review. |

`screen.cpp` is shared (line-drawing + blit + Bresenham are mine, the box/triangle parallel fills are Martin's, the pixel/line/text entry points are Alicia's).

Reproduce the live split yourself:

```sh
bash scripts/compute-contributions.sh
cat .github/badges/contribution.json
cat .github/badges/contributions.md
```

Code review caught real bugs before merge (for example a strict-weak-ordering crash in the process sort, and the `utime+stime` off-by-one in the Linux per-process CPU% parser). Released publicly with the team's and instructor's consent under LGPL-2.1.

## Contributing

Coding standards (Allman braces, `#ifndef` guards, no lambdas, no exceptions, camelCase, no magic numbers) and the PR/review process are documented in the collapsible below; fill out [`PULL_REQUEST_TEMPLATE.md`](PULL_REQUEST_TEMPLATE.md) for every PR.

<details>
<summary>Coding standards and team process</summary>

**Design and architecture**
- Keep non-public helpers private; avoid code smells.
- Header guards via `#ifndef`, not `#pragma once`.
- No exceptions or assertions; handle every corner case and log to `std::cerr` instead of crashing.
- No `print` statements in production code; mark file-local functions `static`.
- No lambda functions.

**SDL3**
- Link SDL3 dynamically; do not compile it into source.
- SDL3 sits between Layer 1 (window + GPU pixel output) and Layer 2 (UI layout and interaction).

**Style**

| Category | Convention |
|---|---|
| Variables, free functions | camelCase |
| Class names | PascalCase |
| Vector aliases | `vecX`, `ivecX` |
| Constants | CAPITALIZED_SNAKE_CASE |
| Magic numbers | Not allowed; use `const` / `constexpr` |
| Indentation | Allman style |
| Control one-liners | Not allowed; always full braces |

```cpp
// not allowed
if (x) doSomething();

// required (Allman)
if (x)
{
    doSomething();
}
```

**UML diagrams** are PlantUML, compiled by `compile-uml.sh`: `-` private, `+` public, `name: type` fields, a dividing line between fields and methods.

**Team process**
- Every PR uses the template and gets a written review before merge.
- Trello flow: Backlog -> Active (assigned) -> Review (PR). Address corrections, re-review, then the reviewer merges. The team reviews all changes together at each milestone.
- Run the full test suite locally before pushing.
</details>

## License

[LGPL-2.1](LICENSE). Team project, released with the team's and instructor's consent.