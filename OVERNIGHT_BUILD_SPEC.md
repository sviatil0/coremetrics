# CoreMetrics: overnight build spec (Gastown agents)

> Goal: deepen CoreMetrics so it maximizes a Neo Scholars admission. Neo selects on
> extraordinary technical ability and runs a live Deep-Dive where the author must defend
> every line. So the prime directive is: **make the parts that are 100% Stefan's deeper,
> more impressive, and more obviously his.** Do NOT extend teammates' code as if it were
> his. Two tracks run in parallel: a SAFE track (ships by morning, merges to main via PR)
> and a MOONSHOT track (ambitious, on its own branch, behind CI, may stay half-done).

## Ground rules (every agent, every PR)

1. **Branch flow:** branch off `origin/dev`. PR targets `dev`, never `main`. One PR per unit of work. Never push to main directly. `main` and `dev` are currently in sync.
2. **You MAY build on top of any code in the repo, including teammates' (ThreadPool, Cloneable, the box/triangle fills).** Stefan approved building on top. The freedom is about WHAT to build, not about CLAIMING it: the contribution accounting must stay truthful. So:
   - New solo-authored code is Stefan's and can be described as his.
   - New code that builds on or modifies a teammate's component: describe it as "built on [teammate]'s X", never as if the underlying component is his.
   - Do not rewrite the README contribution table to inflate his share; if his real share grows because he (the agents on his behalf) added substantial new code, update the numbers from a fresh `git blame`, never by hand.
   - No "from-scratch" claim for anything wrapping SDL. Every doc claim must survive `git blame` and a Deep-Dive "walk me through this".
3. **No em-dashes** anywhere (code, docs, commit messages). Conventional Commits (`feat(...)`, `fix(...)`, `test(...)`), imperative, no `Co-Authored-By`.
4. **Every PR must:** build clean with `-Wall -Wextra` on macOS and Linux (CI matrix), keep all 175 existing tests green, and add tests for new code. A red PR is not done.
5. **C++23, no new third-party deps** beyond SDL3/SDL3_ttf/SDL3_image. Match the existing style (Allman braces, `#ifndef` guards, no lambdas per the course spec the repo documents, camelCase, named constants).
6. **Persisted working dir:** `~/Documents/code/coremetrics` (NOT /tmp). Remote: `github.com/sviatil0/coremetrics`. Author commits as Stefan: `git -c user.name="Sviatoslav Oleksiienko" -c user.email="soleksiienko1@gmail.com"`.
7. **Git author identity matters for the contribution story.** All commits MUST be authored as Sviatoslav (above) so blame attributes the new work to Stefan. This is the whole point of the overnight build.

## Authorship map (for honest attribution, not a build restriction)

Build wherever the feature needs to live. This map exists so the agents describe the
result truthfully, and so the highest-value work targets where Stefan's NEW lines land.

**100% Stefan's today (new code here is unambiguously his):**
- `src/SystemMetrics_linux.cpp` (308 lines), `src/SystemMetrics_mac.cpp` (234) — the metrics backends, **his strongest moat**.
- `src/font.cpp` (the real TTF text path via SDL3_ttf), `src/SoundPlayer.cpp`
- `src/Bar.cpp`, `src/Row.cpp` (the monitor widgets), `src/matrix.cpp`
- `src/GUIElementFactory.cpp`, `include/Tree.hpp`, `include/Bar.hpp`, `include/Row.hpp`, `include/EventManager.hpp`
- The Bresenham line rasterizer (`plotLineLow`/`plotLineHigh` in screen.cpp) is his.

**Teammates' today (you MAY build on these; describe new work as "built on X"):**
- `ThreadPool.*` (Martin's), `Cloneable.hpp` (Alicia's), `drawBox`/`drawTriangle`/`drawPixel`/`drawText`/`drawLine` bodies (Martin's + Alicia's), `GUIFile.cpp` (Alicia's).

**Highest leverage:** features whose new lines land in his files or in brand-new files he
(via these agents) authors. That grows his real, defensible share AND deepens his moat.

---

## SAFE TRACK (must ship + merge by morning)

Each is a separate PR to `dev`. Ordered by value.

### S1. Real per-process CPU% on macOS + verify Linux/Windows (his backends)
The Processes tab CPU% is the single most visible weak spot (screenshots show 0.0). The Linux+Windows deltas were added; macOS `topProcesses` likely still reports a non-delta value.
- In `SystemMetrics_mac.cpp::topProcesses`, track previous per-pid CPU time (`proc_pidinfo` / ` pti_total_user + pti_total_system`) and the previous wall-clock or system-total, and compute a real percentage between samples. Mirror the Linux approach already in the repo.
- Add a deterministic unit test for the delta math (feed two synthetic samples to a small pure helper, assert the percentage). Extract the delta computation into a tiny testable free function so it can be unit-tested without a live OS.
- Update the README/API to state CPU% is delta-based on all three platforms.

### S2. Sortable-by-any-column process table, click the header to sort
Right now the table sorts by memory only. Make it match the README claim "sortable by PID / NAME / CPU% / MEM%."
- This lives in `coremetrics.cpp` (Stefan's app) + `Row` (his). Wire the header `Row` cells to emit a sort event; re-sort `topProcesses` output by the chosen column; show a sort arrow. Keep all sorting logic in his files.
- Test the comparator for each column (pure function, easy unit test).

### S3. Extract and unit-test the metrics parsers (his backends, real coverage)
The `/proc` parsing (`readProcStatTotals`, `readProcCpuTicks`, `readProcMemKb`) is his and currently has no direct unit tests.
- Refactor each parser to take a string (the file contents) instead of reading the file, so it is unit-testable with fixtures. Keep a thin file-reading wrapper.
- Add a `tests/SystemMetricsParseTest.cpp` with fixtures: well-formed `/proc/stat`, a malformed line, a process with a space/paren in its name (the `rfind(')')` case), empty input. This is real, defensible test coverage on his hardest-to-verify code, and it kills the "no metrics tests" gap.

### S4. Make `drawText` real on his side: a font-rendered HUD label path
`Screen::drawText` uses SDL's 8x8 debug font (not his, and ugly in the screenshots). `Font::drawText` (his, real TTF) exists but is only used by `Label`.
- Route the monitor's numeric readouts and headers through `Font::drawText` so the app renders crisp TTF text (his code path), and the hero screenshots look professional.
- Regenerate the two screenshots (System + Processes) via the existing `--screenshot` flag AFTER S1 (so CPU% shows real non-zero numbers) and AFTER this (so text is crisp). Commit the new PNGs. This directly fixes the audit's top presentation finding.

### S5. Repo hygiene + on-ramp
- Add a `CONTRIBUTING.md` (build, test, the PR/branch flow) and a short `CMakeLists.txt` alongside the Makefile (many reviewers expect CMake; keep the Makefile too).
- Add a `make asan` / `make ubsan` target that builds the test suite with `-fsanitize=address,undefined` and runs it; wire a CI leg for it. Fix anything it flags in his files.
- Replace the bundled `assets/font.ttf` with a clearly-licensed font (e.g. a SIL-OFL font) and note the license, OR document the current font's license. (Audit flagged an unlicensed bundled font.)
- Set GitHub repo topics (`cpp`, `cpp23`, `sdl3`, `system-monitor`, `gui-library`, `cross-platform`) via `gh repo edit --add-topic`.

### S6. A real benchmark for his code
- Add `bench/` with a micro-benchmark of the Bresenham line rasterizer (his) and the `/proc` parse throughput (his): lines/sec, parses/sec. Print a small table. This gives the README a real, honest performance number on code that is unambiguously his (no ThreadPool claim).

---

## MOONSHOT TRACK (separate branch `moonshot/*`, behind CI, may stay half-done)

Pick ONE. Each is a genuinely hard, 100%-Stefan feature that would make the project stand out. All are additive and isolated so a half-finished version never breaks main.

### M1. A from-scratch font atlas + glyph rasterizer (replace the debug font, fully his)
Build a real text renderer that does NOT lean on SDL's debug font: load the TTF glyph outlines via SDL3_ttf into a packed texture atlas once, then draw text by blitting glyph quads from the atlas through his own `Screen`/`blitSurface` path. This makes the entire text pipeline his, is a real graphics-systems feature, and is the single best Deep-Dive story (kerning, atlas packing, the bitmap cache). Gate it behind a flag; fall back to the current path if it is unfinished.

### M2. Historical sparklines: ring-buffer time series per metric, drawn by his rasterizer
Add a fixed-size ring buffer of the last N samples for CPU/RAM/GPU and each top process, and render a live sparkline (a polyline through his Bresenham `drawLine`) next to each bar. This is real data-structure + rendering work entirely in his files (a new `Sparkline` widget + a `RingBuffer<T>` in the style of his `Tree<T>`), turns a static monitor into a real-time one, and is genuinely impressive in a screenshot.

### M3. A lock-free single-producer/single-consumer sample queue (concurrency he owns)
The audit noted the parallel fills are a teammate's, so Stefan has no concurrency story he can defend. Give him one: a header-only `SpscRingBuffer<T>` (his) that decouples a metrics-sampling thread from the render thread, so sampling never blocks the UI. Real lock-free systems work (atomics, memory ordering, the ABA-free SPSC design), fully his, and the exact kind of thing Neo's bar rewards. Heavily tested (a stress test with a producer + consumer thread, ASan/TSan clean).

### M4. eBPF / perf-event per-process CPU on Linux (deep systems, his backend)
Go past `/proc`: read per-process scheduler stats via `perf_event_open` for accurate, low-overhead CPU attribution on Linux. Deep kernel-interface systems work in his metrics backend. High risk (needs privileges, Linux-only), so strictly moonshot-on-a-branch.

**Recommended moonshot: M2 (sparklines) for visible payoff + M1 (font atlas) as the stretch.** M3 if you want a pure-systems concurrency story to counter the "parallel fills aren't his" gap. M2 is the safest moonshot that still looks impressive in a screenshot.

---

## Morning deliverable

- All SAFE-track PRs merged to `dev`, CI green, ready for a `dev -> main` release PR.
- Moonshot on its own branch with a clear status note (what works, what's stubbed).
- New screenshots committed (real CPU%, crisp text).
- A one-paragraph summary in this file's CHANGELOG section of what shipped, so Stefan can update the README contribution table honestly (every new line is his; state it).

## CHANGELOG (agents append here)
- (empty)
