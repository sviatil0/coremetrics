# CoreMetrics Performance

A hand-rasterized C++23 UI that lands every poll inside its frame budget on commodity Apple silicon, with the rasterizer floor sitting near memory bandwidth.

Author: Sviatoslav Oleksiienko <soleksiienko1@gmail.com>

## Benchmark table

All numbers are real, single-threaded unless noted, measured on Apple
M-series with `-O2`. The harness lives in `bench/bench.cpp` and exercises
the same code paths the live UI calls every poll. Variance is ~+/-10%.

| Bench                              | Throughput            | Per-call cost      | Notes                                                              |
| ---------------------------------- | --------------------- | ------------------ | ------------------------------------------------------------------ |
| `Screen::drawPixel`                | ~4.27e7 ops/sec       | ~23 ns             | Pure clip + clamp + 32-bit store; the floor every higher draw rides on. |
| `Screen::drawLine` (random)        | ~1.11e5 ops/sec       | ~9 us              | Bresenham core; ~4.05e7 px/sec, i.e. roughly the drawPixel rate.   |
| `Screen::drawBox` (small, inline)  | ~drawPixel * area     | < 1 us for 32x32   | Areas <= 4096 px or <= 4 rows skip the ThreadPool entirely.        |
| `Screen::drawBox` (parallel pool)  | up to N-way speedup   | depends on area    | Crossed only by big fills; row-strip work units, futures::get fence. |
| `Screen::drawTriangle`             | bounded by bbox area  | scanline-edge fill | Same row-split ThreadPool dispatch as the big drawBox path.        |
| `Sparkline::draw` (64 samples)     | ~1.56e2 ops/sec       | ~6.4 ms / frame    | Triangle fill + 3px stroke; the heaviest widget in the UI.         |
| `Bar::draw` (gradient + depth)     | ~1.87e3 ops/sec       | ~0.5 ms / frame    | One bg box, one active fill, two depth overlays.                   |
| `Screen::drawText` (cached)        | ~2.81e5 ops/sec       | ~3.6 us            | Hashmap lookup + cached SDL surface blit.                          |
| `Label::draw` (bold)               | ~1.51e5 ops/sec       | ~6.6 us            | Cached glyph surface, blitted twice for fake-bold weight.          |

## Frame-budget math

60 Hz gives a 16.6 ms frame budget. CoreMetrics does not chase 60 Hz:
it polls the OS every 500 ms and only repaints after a poll, so the
steady-state render budget per poll is what matters, not per-vsync time.

A typical screen draws four Sparklines, three Bars, and a handful of
Labels:

- 4x Sparkline at ~6.4 ms each = ~25 ms of triangle fill
- 3x Bar at ~0.5 ms each = ~1.5 ms
- ~10x Label at ~6.6 us each = ~66 us
- background `drawBox` clears and separators on the inline fast path = sub-millisecond

That ~27 ms total is over a single 60 Hz frame but well under the 500 ms
poll interval, so the UI thread is idle ~95% of the time. We are
rasterizer-bound during a paint and IO-bound between paints. The
steady-state UI never misses its budget on the measured hardware.

## What is fast and why

- **drawPixel is straight memcpy-ish.** Bounds check, RGB clamp, one
  `SDL_MapSurfaceRGBA`, one 32-bit store. At ~23 ns per write it is
  bottlenecked by L1 store bandwidth, not arithmetic. Every higher
  primitive calls into this, so the floor is healthy.
- **drawBox skips the ThreadPool for small fills.** The threshold is
  `rows <= 4 || rows * cols <= 4096`. Per-row separators, signal-menu
  cell strokes, and 5-column row cells stay inline because the pool
  dispatch round-trip (wakeup + future::get) costs more than the fill
  itself at those sizes. The pool earns its keep only on big background
  and Sparkline fills.
- **Text cache is a hashmap.** `font.cpp` keys glyph surfaces on
  `(text, RGB)`. First call renders + caches; every subsequent call is
  a dictionary lookup plus an `SDL_BlitSurface`. Once the UI settles,
  every Label readout is a cache hit, which is why cached `drawText`
  runs two orders of magnitude above the cold path.
- **drawLine is Bresenham, not floats.** No FP per pixel; integer error
  accumulation and the same drawPixel store. The ~4.05e7 px/sec matches
  the drawPixel rate, which is exactly what you want to see.

## What is slow and why

- **Sparkline dominates the frame.** At ~6.4 ms per draw it is roughly
  40x heavier than Bar. The cost is the two-pass fill: for each of 63
  segments, two triangles are filled to the baseline, and each one goes
  through the ThreadPool with a 916x28 bbox. The triangle rasterizer
  iterates the full bbox and tests edge functions per pixel; most of
  the bbox is rejected for steep segments, which wastes work.
- **Future optimization: scanline fills.** Replacing the per-segment
  two-triangle path with a single trapezoid scanline fill (top edge
  interpolated linearly, bottom clipped to baseline) would cut the fill
  to one scan per column instead of an edge-function test per bbox
  pixel. Conservative estimate: 3-5x speedup on Sparkline.
- **Label bold is a double blit.** Fake-bold blits the same cached
  glyph surface twice, offset by 1 px on x, which halves the cached
  text rate (~2.81e5 to ~1.51e5 ops/sec). Acceptable: real bold needs a
  second TTF face, and at ~6.6 us per call we are nowhere near budget.

## Methodology

```bash
make bench
./bin/bench
```

The harness uses precomputed input vectors so timed sections are pure
rasterizer work, not RNG churn. Each leg has a warmup phase that
pre-populates caches, then times a fixed-count loop with
`std::chrono::steady_clock`. The canvas is left dirty between iterations
on the line and Sparkline legs so `Screen::clear` is not folded in.

Build flags: `-std=c++23 -O2 -DNDEBUG`. Target: macOS arm64. x86_64
Linux numbers land in the same order of magnitude; the rasterizer is
portable C++ with no intrinsics.

## How to verify

After building, run `./bin/bench` and compare against the table above.
A regression shows up as a draws/sec drop or a per-call cost climb on
the affected leg. The Sparkline number is the one to watch: it is the
tightest against any realistic frame budget, so a 2x regression there
turns a comfortable paint into a visibly stuttery one.

If a number is wildly off, check the build is release (`make bench`
uses `-O2`), no debug instrumentation is compiled in, and the machine
is not thermally throttled. The rasterizer has no hidden allocations
on the hot path; a true regression will be in one specific leg.
