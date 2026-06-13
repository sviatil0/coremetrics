# CoreMetrics: structural advantages

The interesting part of CoreMetrics is not what it shows. It is what it doesn't depend on, and what that buys you.

## The whole stack is C++23 + SDL3

There is no Qt, no GTK, no Electron, no Dear ImGui, no Chromium, no JavaScript runtime, and no scripting layer. The dependency graph from `coremetrics` is:

```
coremetrics
  ├─ SDL3            (window, surface, audio, event loop)
  ├─ SDL3_ttf        (FreeType wrapper for the bundled Roboto)
  ├─ SDL3_image      (PNG load + IMG_SavePNG for hero shots)
  └─ libc / libc++
```

Everything else — the rasterizer, the layout engine, the widget hierarchy, the events, the cross-platform metrics backends, the moonshot sparkline widget, the asset-path resolver — is in this repository, in plain C++23, with no third-party code generation.

Why this matters:

- **Reproducible builds.** A fresh Ubuntu container or a clean Mac with `brew install sdl3 sdl3_ttf sdl3_image` can build the entire binary in under 10 seconds. There is no `node_modules`, no `vendor/`, no submodule tree, no patched fork of a UI framework.
- **One binary per platform.** The macOS release tarball is **1.5 MB**, the Linux release tarball is **1.5 MB**, the Debian package is **1.5 MB**. There is no embedded runtime to ship.
- **Defensible internals.** Every pixel touched at runtime goes through `Screen::drawPixel`, `Screen::plotLineLow` / `plotLineHigh` (Bresenham), `Screen::drawBox`, or `Screen::drawTriangle`. None of those are calls into a library; they are functions in `src/screen.cpp` that can be opened in a code review and walked line by line.
- **No "but the framework does it" answers.** Tabs do not animate because some framework's `<Tabs>` component supports `animation: "ease-in-out"`. They animate because we render the next frame. Crashes, off-by-ones, and missed signals all live in code we can read.

## One UI codebase, three native metrics backends

`include/SystemMetrics.hpp` declares four static methods:

```cpp
class SystemMetrics
{
public:
    static float readCpuPercent();
    static float readMemPercent();
    static float readGpuPercent();
    static std::vector<ProcessInfo> topProcesses(std::size_t n = 20);
};
```

The implementation is split into three platform files, each selected at compile time by `#ifdef`:

```
src/SystemMetrics_linux.cpp      /proc, /sys              (Linux)
src/SystemMetrics_mac.cpp        Mach + IOKit + libproc   (macOS)
src/SystemMetrics_win.cpp        PDH + Toolhelp           (Windows)
```

There is no abstraction layer between them and the UI. `coremetrics.cpp` calls `SystemMetrics::readCpuPercent()` and gets a `float` — the call site does not care which backend is linked. The platform split is invisible above the header.

That is rare. Most cross-platform system monitors either:

- Use a portable library (`procps`, `sigar`, `system-monitor-rs`) and inherit its abstractions, its assumptions, and its dependencies.
- Ship three separate apps with separate UIs.

CoreMetrics writes the host-specific code itself (`proc_pidinfo` on macOS, `/proc/[pid]/stat` parsing on Linux, `GetProcessTimes` on Windows) and keeps the UI free of platform code. Every backend was written from scratch and is unambiguously the author's, [`git blame` verified](#team-and-my-contribution).

## The custom rasterizer is a feature, not a workaround

The Bresenham line rasterizer in `src/screen.cpp` is small, fast, and provably correct. The benchmark in `bench/bench.cpp` measures **8.5 × 10⁴ lines per second** and **3.1 × 10⁷ pixels per second** on an Apple M-series. Those numbers are pure C++23 against an offscreen `SDL_Surface`.

What that buys:

- **The moonshot sparkline widget is trivial to add.** A new `Sparkline` widget needed exactly two things: a `RingBuffer<float>` of recent samples and a loop that calls `Screen::drawLine` between adjacent samples. Total: 105 lines in `src/Sparkline.cpp`, 64 lines in `include/RingBuffer.hpp`. No new dependencies, no theme integration, no styling system; the chart inherits the application's color palette because the application owns the renderer.
- **The same rasterizer powers everything.** Bars, the moonshot sparkline polylines, dividers under the tab bar and the Processes header, the green accent stripe in the footer — all of them are `drawBox` and `drawLine` calls. There is no separate "charts" layer that needs to be aligned with the "widgets" layer.
- **Performance is predictable.** When the Processes tab refreshes, the work is `O(rows × columns × glyph blits)`. There is no virtual DOM diff, no style cascade, no layout pass that walks an opaque object graph. The 500 ms poll loop is comfortably within budget on a single thread.

## What this unlocks at submission time

For a Neo Scholars Deep-Dive:

- Every function in the render path can be walked in a code review without leaning on "the library handles that".
- The cross-platform claim is checkable in 30 seconds: open three files, see three different APIs producing one shared shape.
- The hero screenshots are produced by the same `--screenshot` code path users will run; there is no Figma layer between the design and the binary.
- The renderer benchmark gives the README a real, honest performance number on code that is unambiguously the author's.

For the project's future:

- Adding a per-core CPU strip, a memory breakdown bar, or a process tree view does not require a UI framework upgrade. It requires extending `SystemMetrics` and `LayoutManager`, both of which are this repository.
- The moonshot M1 (font atlas + glyph rasterizer) is feasible because every other text path in the app already goes through `Font::drawText`. Swapping the backend is a one-file change, not a framework migration.
- The Homebrew tap, the `.deb`, and the tag-driven release workflow ship the same single binary built from the same source. There is no "web build", "macOS build", "Linux build" divergence to maintain.

The shortest summary: CoreMetrics has fewer moving parts than a comparable Electron or Qt monitor, and the parts it does have are written here, in this repository, by the author. That property is the whole point of the project.
