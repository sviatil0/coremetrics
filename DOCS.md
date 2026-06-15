# CoreMetrics: documentation map

A guide to everything in this repo and the order to read it in. Start at the top and
go as deep as you need.

## 1. Start here

| Read | What you get | Time |
|---|---|---|
| [README.md](README.md) | What CoreMetrics is, a screenshot, why it is interesting, install paths (Homebrew + apt + tarball), quickstart, architecture, and the live contribution badge. | 5 min |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Branch flow (`main ← dev ← feature`), Conventional Commits, PR conventions, sanitizers (`make asan` / `make ubsan`), and the test-wiring pattern. | 3 min |
| [Final presentation (PDF)](docs/CoreMetrics-Final-Presentation.pdf) | The architecture, the cross-platform proof, and a demo, in slide form. | 5 min |
| [API.md](API.md) | The full public API of the GUI library: every class and method, by category, with a table of contents. | reference |

## 2. Understand the architecture

The system is four layers: SDL3 gives a window and a raw pixel surface, `Screen` turns
that surface into drawing primitives, the `GUIElement` / `Layout` tree composes those
primitives into a UI, and `EventManager` routes input back down the tree.

| Diagram | Scope |
|---|---|
| [Overview](assets/overview.png) | The whole class graph, grouped by package. |
| [Core](assets/core.png) | Math (`vec2` / `vec3` / `matrix`), `Screen`, `ThreadPool`. |
| [GUI](assets/gui.png) | `GUIElement` hierarchy, `Cloneable`, `GUIElementFactory`. |
| [Layout](assets/layout.png) | `Tree<T>`, `Layout`, `LayoutManager`, `GUIFile`. |
| [Events](assets/events.png) | `Event` hierarchy, `EventManager`, `SoundPlayer`. |
| [Metrics](assets/metrics.png) | The `SystemMetrics` cross-platform backends. |

Diagrams are PlantUML; regenerate with [`compile-uml.sh`](compile-uml.sh) from the
`.puml` sources in [`uml/`](uml/). The full per-class API reference is in the README's
collapsible "API reference" section.

## 3. Read the code, by subsystem

Suggested reading order, simplest to most involved:

1. **Math**: [`include/vec2.hpp`](include/vec2.hpp), [`vec3.hpp`](include/vec3.hpp), [`src/matrix.cpp`](src/matrix.cpp). Templated vectors and a 3x3 matrix.
2. **Rasterizer**: [`src/screen.cpp`](src/screen.cpp). `drawPixel`, Bresenham `drawLine` (`plotLineLow` / `plotLineHigh`), `drawBox`, barycentric `drawTriangle`, `drawText`, `blitTo`. The heart of the graphics stack.
3. **Threading**: [`src/ThreadPool.cpp`](src/ThreadPool.cpp). The worker pool that `drawBox` / `drawTriangle` partition pixel rows across.
4. **Widgets**: [`src/Bar.cpp`](src/Bar.cpp), [`Row.cpp`](src/Row.cpp), [`Label.cpp`](src/Label.cpp), [`Button.cpp`](src/Button.cpp), [`selection.cpp`](src/selection.cpp), and the `Cloneable` / `GUIElementFactory` plumbing.
5. **Layout**: [`src/Layout.cpp`](src/Layout.cpp), [`LayoutManager.cpp`](src/LayoutManager.cpp), [`include/Tree.hpp`](include/Tree.hpp). The render tree and the painter's algorithm.
6. **Events**: [`src/EventManager.cpp`](src/EventManager.cpp), `ClickEvent` / `ShowEvent` / `SoundEvent`, [`SoundPlayer.cpp`](src/SoundPlayer.cpp).
7. **Metrics**: [`src/SystemMetrics_linux.cpp`](src/SystemMetrics_linux.cpp) (`/proc`), [`SystemMetrics_mac.cpp`](src/SystemMetrics_mac.cpp) (IOKit), [`SystemMetrics_win.cpp`](src/SystemMetrics_win.cpp) (PDH), selected by `#ifdef`. Each delegates per-process CPU% to the pure `computeCpuPercentDelta` helper in [`src/ProcessUtils.cpp`](src/ProcessUtils.cpp); the Linux parsers live in [`src/ProcParsers.cpp`](src/ProcParsers.cpp) so they can be fixture-tested without `/proc`.
8. **Sparklines** (optional): [`include/RingBuffer.hpp`](include/RingBuffer.hpp) (header-only template, mirrors `Tree<T>`) and [`src/Sparkline.cpp`](src/Sparkline.cpp) (renders a polyline through `Screen::drawLine`). Activated at runtime with `--sparklines`.
9. **The app**: [`coremetrics.cpp`](coremetrics.cpp). Builds the two-tab scene, the configurable poll loop (default 500 ms, override with `--poll-ms <N>` clamped 100..10000), the `--screenshot` render path (`.png` via `IMG_SavePNG`, `.bmp` via `SDL_SaveBMP`), and the `SIGINT` / `SIGTERM` / `--duration` lifecycle controls.

## 4. Run and verify

```sh
make                                       # build + launch
make test                                  # 17 test suites
make bench                                 # Bresenham line rasterizer throughput
make asan                                  # tests under AddressSanitizer
make ubsan                                 # tests under UndefinedBehaviorSanitizer
./run-cross-platform-tests.sh              # macOS native + Ubuntu (Docker)
./stress.sh                                # drive real CPU/RAM/GPU load so the bars move
./bin/coremetrics --screenshot shot.png    # render one frame headlessly, PNG output
./bin/coremetrics --sparklines             # add rolling CPU/RAM/GPU charts
./bin/coremetrics --duration 5             # auto-quit after 5s (for backgrounded smoke tests)
./bin/coremetrics --poll-ms 250            # custom refresh cadence (clamped 100..10000)
```

Each subsystem has a matching test in [`tests/`](tests/): `screenTest` (the rasterizer,
pixel-level), `ThreadPoolTest`, `SystemMetricsTest`, `SystemMetricsParseTest` (the
fixture-driven `/proc` parsers), `ProcessUtilsTest` (pure CPU% delta and the column
comparator), `RingBufferTest` (the rolling-window storage behind sparklines),
`CloneableTest`, `TreeTest`, `LayoutManagerTest`, `EventManagerTest`, and the widget
tests.

## 5. How the project was built

- [CONTRIBUTING.md](CONTRIBUTING.md) is the day-to-day contributor guide (branch flow,
  Conventional Commits, sanitizers, test wiring, no em-dashes, no `Co-Authored-By`).
- [PULL_REQUEST_TEMPLATE.md](PULL_REQUEST_TEMPLATE.md) and the merge history show the
  PR-per-change, review-before-merge workflow.
- The `dev` branch is the integration branch; `main` is stable.
- Authorship is computed live by `scripts/compute-contributions.sh` (used by the
  `Contribution badge` workflow); the README badge always reflects current `main`.
- Tag-driven releases land binaries on the [Releases page](https://github.com/sviatil0/coremetrics/releases)
  (Linux + macOS tarballs plus a Debian `.deb`); the macOS install path is the
  [`sviatil0/homebrew-coremetrics`](https://github.com/sviatil0/homebrew-coremetrics) tap.

## 6. License

[LGPL-2.1](LICENSE). Team project, released with the team's and instructor's consent.
