# CoreMetrics Architecture

A C++23 system monitor built on raw SDL3, where every layer between
the pixel buffer and the kernel counter is in this repo. Nothing is
hidden behind a widget framework, a charting library, or a retained
scene graph.

## Layer cake

```
+----------------------------------------------+
|  coremetrics.cpp (main loop, event handler)  |
+----------------------------------------------+
|  LayoutManager / EventManager (tree + routing) |
|  Widgets: Bar, Sparkline, Label, Row, Button   |
+----------------------------------------------+
|  Screen (drawPixel, drawLine, drawBox, ...)    |
+----------------------------------------------+
|  SystemMetrics (linux/mac/win backends)        |
+----------------------------------------------+
|  SDL3 + SDL3_ttf + SDL3_image                  |
+----------------------------------------------+
```

Each box is a single subsystem. Calls only flow top-down: the main
loop drives the layout tree, which renders into a `Screen`, which in
turn calls SDL3 primitives. Metric reads are a parallel branch on the
same layer as widgets: `pollMetrics()` consults `SystemMetrics` and
pushes the values into widget state before render.

## Main loop

`coremetrics.cpp` owns startup, argv parsing, signal hooks, and the
poll/render loop. The default refresh cadence is 500 ms, overridable
via `--poll-ms <N>` and clamped to `[100, 10000]` so the UI cannot be
told to repaint faster than the OS can serve samples. Each tick runs
`pollMetrics()`, then walks `EventManager::processEvents`, then
`LayoutManager::render` into the `Screen`, then paints overlays
(uptime, per-core strip, sparkline labels, signal menu, help panel)
on top of the tree's output, then blits to the SDL window surface.

`--screenshot path.bmp [tab]` is a parallel one-frame path that
constructs an offscreen `Screen`, primes the polling pipeline (3 s
warmup, or 64 sparkline ticks at 50 ms when `--sparklines` is on),
renders one frame, and writes a BMP. No window opens. `--duration N`
arms a wall-clock timer that exits the live loop cleanly so CI smoke
tests are bounded. `--export-csv` / `--export-json` write one
aggregate sample plus the top-N process list to disk and exit before
SDL even initializes.

## Layout and events

The widget scene is a `Tree<Layout>` rooted in a static
`LayoutManager` singleton. `Tree<T>` is the project's own templated
node: a value plus `unique_ptr<Tree<T>>` children plus a back pointer
to its parent. `Layout` carries a bounding rect, a name, an active
flag, and an `elements` vector of polymorphic `GUIElement` pointers
(Bar, Label, Row, Sparkline, Button, Image). `base.xml` declares the
initial tree; `GUIFile::readFile` parses it into the singleton.

`EventManager` is a second singleton that owns an event queue and a
`trickleEvent` walker. Events trickle from root to leaf, top-down,
and each layout decides whether to consume the event or pass it down.
Tab switching is implemented as a pair of `ShowEvent`s: one to
deactivate the outgoing tab, one to activate the incoming tab. Clicks
become `ClickEvent` routed by hit-test rect. Alarm thresholds emit
`SoundEvent`s that the loop drains during `processEvents`.

## Widgets

Every widget is a `Cloneable<Derived>` subclass: a CRTP base that
gives each leaf type a `clone()` override returning `new Derived(*this)`
and a covariant `cloneDerived()`. The base class `GUIElement` exposes
`draw(Screen&)`, so the layout tree can render polymorphically without
knowing the concrete type. `Bar` takes a normalized value, a min/max
range, and a metric name; its fill color is recolored by
`Thresholds::colorForPct` so CPU/RAM/GPU all redden together at 60%
yellow, 80% red. `Label` blits a TTF surface through `Font`. `Row`
draws a 5-column table line and lets the main loop swap cell strings
each tick. `Sparkline` owns a `RingBuffer<float>` (a fixed-capacity
circular buffer), pushes samples in `O(1)`, and on `draw` walks the
ring as Bresenham segments. With threshold mode on, each segment
gradients across the same palette `Bar` uses.

## Screen

`Screen` is a software framebuffer wrapping a single `SDL_Surface`.
It exposes `drawPixel`, `drawLine` (Bresenham via `plotLineLow` /
`plotLineHigh`), `drawBox`, `drawTriangle` (edge-function rasterizer),
and `blitSurface` for prerendered glyph or icon surfaces. `drawBox`
takes a fast path for small rects: under 4 rows or fewer than 4096
total pixels it stays single-threaded, because thread-pool dispatch
costs more than the inline fill. Above that, it slices the row range
across a `ThreadPool`, submits one lambda per worker, and joins on
`future::get`. `drawTriangle` does the same fan-out for the sparkline
fill triangles. `blitTo(SDL_Surface*)` is the only call that touches
the SDL window surface; everything else stays in the in-house buffer.

## SystemMetrics

`SystemMetrics.hpp` is a single header. Three `.cpp` files
(`SystemMetrics_linux.cpp`, `SystemMetrics_mac.cpp`,
`SystemMetrics_win.cpp`) each wrap their body in `#ifdef __linux__ /
__APPLE__ / _WIN32` so the build picks the right backend without a
toolchain file flag. Linux reads `/proc/stat`, `/proc/meminfo`,
`/proc/uptime`, `/proc/loadavg`, `/proc/net/dev`, `/proc/<pid>/stat`,
`/proc/<pid>/io`, and the AMD `gpu_busy_percent` sysfs node. macOS
goes through `host_statistics`, `host_processor_info`,
`host_statistics64`, `sysctl`, `libproc`, `getifaddrs`, and `IOKit`
for GPU. Windows uses PDH wildcard counters for CPU and GPU,
`Toolhelp32` for the process snapshot, `GetIfTable` for network
counters, and `GetProcessIoCounters` for per-process disk. All three
backends return identical `ProcessInfo`, `NetIo`, `DiskUsage`, and
`MemBreakdown` structs.

## SDL3 surface

SDL3 is only used as a thin platform layer. `SDL_Init` opens VIDEO
plus AUDIO (VIDEO-only in the screenshot path). `SDL_CreateWindow`
plus `SDL_GetWindowSurface` gives the destination buffer; the loop
calls `SDL_UpdateWindowSurface` once per tick. `SDL3_ttf` loads the
bundled font; `SDL3_image` loads the window icon. `SDL_LoadWAV` plus
`SDL_OpenAudioDeviceStream` plus `SDL_PutAudioStreamData` plays the
alarm beep. No retained-mode scene graph, no `SDL_Renderer` GPU path,
no widget framework. SDL3 hands us a surface and an event pump; we
do the rest.

## Data flow per tick

1. `pollMetrics()` calls `SystemMetrics::readCpuPercent` and friends.
2. Each value is pushed into its widget via `setValue` / `setText`
   / `Sparkline::push`. Threshold colors are reapplied here.
3. `EventManager::processEvents` drains the queue: keyboard events
   from the main loop, click hits from the window, `SoundEvent`s
   from the alarm threshold check, `ShowEvent`s from tab switches.
4. `LayoutManager::render` walks the tree top-down; each `Layout`
   asks its children to `draw(Screen&)`.
5. The main loop paints the overlays the tree does not own (per-core
   strip, mem breakdown, sparkline labels, signal menu, help panel).
6. `Screen::blitTo(SDL_GetWindowSurface(window))` copies the buffer.
7. `SDL_UpdateWindowSurface` pushes pixels to the OS compositor.

## Threading

The main thread does everything. The only fan-out is inside
`Screen::drawBox` and `Screen::drawTriangle`: when the rect or the
triangle bounding box exceeds the inline threshold, work is sliced
across the global `ThreadPool` singleton and joined synchronously
before the call returns. There is no async I/O, no background poll
thread, no double-buffered render thread. Audio is the only
exception: `SoundPlayer` hands a WAV buffer to `SDL_OpenAudioDeviceStream`,
which SDL3 drains on its own audio thread. Metric reads block the
main loop, which is fine because the loop sleeps 500 ms anyway.

## Why this matters

- Every layer is in this repo, audited line by line.
- No retained-mode UI library, no charting library, no metrics SDK.
- One header, three platform `.cpp`s: porting is a single file.
- `Screen` is the only software-renderer surface; every pixel is
  ours.
- The polling cadence, the threshold palette, and the layout tree
  are reviewable in one afternoon.
- Threading is opt-in per draw call, not pervasive: the failure
  modes are local.

Author: Sviatoslav Oleksiienko <soleksiienko1@gmail.com>
