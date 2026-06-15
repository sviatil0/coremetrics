# CoreMetrics vs htop, btop, and macOS Activity Monitor

CoreMetrics is built from-scratch over raw SDL3 surfaces. Here's how that compares to the established tools.

This document is for Neo Scholars reviewers who want an honest answer to the obvious question: "why write another system monitor?" The short version is that CoreMetrics is primarily a from-scratch C++ GUI toolkit, with a working system monitor on top of it. The three tools below are mature, single-purpose monitors. They do many things better. A few things CoreMetrics does that they don't.

## Feature matrix

| Feature | CoreMetrics | htop | btop | Activity Monitor |
|---|---|---|---|---|
| Per-core CPU strip | yes (per-logical-CPU strip on System tab) | yes | yes | no (aggregate + per-process only) |
| RAM breakdown | yes (active / wired / cached / free segments) | yes (used / buffers / cache) | yes (used / available / cached) | yes (wired / compressed / cached / app) |
| GPU % | yes (Linux gpu_busy_percent, macOS IOAccelerator, Windows PDH; aggregate only) | no | no | partial (GPU history window, separate app) |
| Disk I/O per process | yes (DISK I/O column) | partial (IO_READ_RATE / IO_WRITE_RATE columns, optional) | yes | yes (Disk tab) |
| Network I/O | yes (aggregate rx + tx with sparkline) | no | yes (per-interface, per-process) | yes (per-process Network tab) |
| Sparklines | yes (CPU / RAM / GPU / network, behind `--sparklines`) | no (bars only) | yes (built-in graphs) | partial (CPU history window) |
| Tree view | yes (`t` key, `--tree` flag) | yes (`F5`) | yes | no |
| Process kill / signal menu | yes (TERM / KILL / INT / HUP / STOP / CONT) | yes (full signal list, `F9`) | yes (full signal list) | yes (force quit only) |
| Filter by name | yes (`/` key, case-insensitive substring, `--filter` flag) | yes (`F4` filter, `F3` search) | yes | yes (search box) |
| Sortable columns | yes (click any header) | yes (`F6`) | yes | yes |
| Cross-platform GUI | yes (Linux, macOS, Windows; one binary per OS) | no (Linux + macOS TUI only, no Windows) | yes (TUI on Linux / macOS / Windows via WSL) | no (macOS only) |
| Headless screenshot | yes (`--screenshot out.png [tab]`, no window) | no | no | no |
| CSV / JSON export | yes (`--export-csv` / `--export-json`, one-shot dump) | no | no | no (Activity Monitor sample dumps text only) |
| Dependency footprint | SDL3 + SDL3_ttf + SDL3_image; ~1.3 MB stripped binary | ncurses + libnl (Linux) | ncurses + custom | system framework, bundled with macOS |

The "yes" cells for CoreMetrics are all checkable in the repo today: `coremetrics.cpp` (CLI flags), `src/Exporter.cpp` (CSV / JSON), `src/SignalUtils.cpp` (signal menu), `src/SystemMetrics_{linux,mac,win}.cpp` (per-OS backends).

## What CoreMetrics has and they don't

- **Headless screenshot mode.** `coremetrics --screenshot out.png [system|processes]` runs one render pass to an offscreen `SDL_Surface` and writes a PNG (or BMP via extension), no window required. CI uses this to verify the UI on macOS, Linux, and Windows without a display.
- **Hand-rasterized UI.** Every widget rasterizes itself onto a raw `SDL_Surface` through one `Screen` primitive layer (`drawPixel`, Bresenham `drawLine`, `drawBox`, `drawTriangle`, `blitTo`). No Dear ImGui, no Qt, no GTK, no Electron, no game engine. htop and btop are TUIs over ncurses; Activity Monitor is Cocoa.
- **CSV / JSON export, one-shot.** `--export-csv <path>` and `--export-json <path>` dump the current aggregate metrics plus a top-N process snapshot and exit. CSV is RFC 4180 quoted and formula-injection guarded (`'=cmd|...'` is prefixed with a single quote). The other three have no equivalent built in.
- **Sub-1.5 MB binary.** The stripped release binary is around 1.3 MB on macOS arm64. SDL3 + SDL3_ttf + SDL3_image are dynamically linked from the system.
- **No Qt, GTK, or Electron.** The entire widget toolkit (`Bar`, `Row`, `Label`, `Button`, `Image`, `Selection`) is in this repo, behind one `GUIElement::draw(Screen&)` interface, with a `Cloneable<Derived>` CRTP mixin for free deep-copy.
- **Three native metrics backends from one header.** `SystemMetrics.hpp` has one public surface; `SystemMetrics_linux.cpp` reads `/proc` + `/sys`, `SystemMetrics_mac.cpp` calls mach + IOKit, `SystemMetrics_win.cpp` calls PDH + Toolhelp. Selected at compile time by `#ifdef`. htop is POSIX-only; btop has a Windows port in progress but is primarily a TUI; Activity Monitor is macOS-only.

## What they have and CoreMetrics doesn't

- **Full signal set.** htop and btop expose every signal in the platform's `signal.h`. CoreMetrics's `k` menu is curated to six entries (TERM, KILL, INT, HUP, STOP, CONT). Adding more is a one-line change in `SignalUtils.cpp`, but it isn't done yet. Activity Monitor only exposes "Quit" and "Force Quit".
- **Historical samples log.** btop keeps a long rolling history per metric and can export it; htop's `--tree` and meters persist across sessions via `~/.config/htop/htoprc`. CoreMetrics keeps a fixed-size in-memory `RingBuffer<T>` for sparklines (the buffer length is set at construction) and forgets everything when the process exits. There is no on-disk log file.
- **Customizable color themes.** htop has named color schemes (Default, Monochrome, Black on White, Light Terminal, MC, Broken Gray); btop ships dozens of themes and supports user themes. CoreMetrics has one theme: the load-coloring thresholds (yellow above 60%, red above 80%) are hard-coded in `Thresholds.hpp`. A theme system would be a feature, not a fix.
- **Gauge-style charts.** btop has rich circular and gauge meters; Activity Monitor's per-core CPU window is gauge-style. CoreMetrics has horizontal `Bar` widgets and `Sparkline` line charts. Adding a gauge widget would mean a new `GUIElement` subclass.
- **Command-line invocation patterns.** htop has `htop -u <user>`, `htop -p <pid>`, `htop --sort-key <col>`, `htop --no-color`, and a dozen more. CoreMetrics's CLI is intentionally small: `--screenshot`, `--duration`, `--sparklines`, `--tree`, `--filter`, `--poll-ms`, `--export-csv`, `--export-json`. No user filter, no PID filter, no sort flag, no color flag.
- **Maturity.** htop has been shipping since 2004 and has been audited, packaged, and battle-tested on every Unix. btop is a few years old but has a large user base. Activity Monitor is the system tool. CoreMetrics is a three-month student project at v0.2.x.

## Verdict

CoreMetrics is not trying to replace htop. CoreMetrics is a from-scratch C++23 GUI toolkit (rasterizer, widget hierarchy, event system, layout tree, three native metrics backends) with a working system monitor demo built on top of it. The monitor is the proof-of-life for the toolkit. The toolkit is the reason the repo exists.

htop is the industry-standard interactive process viewer. If your goal is "I need to see what's eating my CPU right now on a Linux server I just SSHed into," install htop. If your goal is "I want to read a small, modern C++ codebase that builds a real GUI app on top of raw pixel surfaces and three OS-native metrics backends, with three months of CI history, 17 test suites, and a contribution-percentage badge wired to `git blame -w -C -M`," read CoreMetrics.

Different goals, different tools. Both should exist.
