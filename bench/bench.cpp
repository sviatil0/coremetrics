#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <SDL3/SDL.h>
#include "Bar.hpp"
#include "font.hpp"
#include "Label.hpp"
#include "screen.hpp"
#include "Sparkline.hpp"

// Author: Sviatoslav Oleksiienko <soleksiienko1@gmail.com>
//
// Tiny micro-benchmarks for the project's own software rasterizer paths.
// First leg is the Bresenham line core in src/screen.cpp (random-direction
// lines into an offscreen surface). The next two legs measure the new
// visual polish on top of that rasterizer: the Sparkline two-pass renderer
// (triangle fill plus thickened 3px stroke) from src/Sparkline.cpp, and
// the Bar gradient/depth overlay from src/Bar.cpp. They report draws/sec
// so a regression in the polish code shows up as a frame-budget cost
// number rather than a vibe.
//
// The last three legs measure the text + pixel + label pipeline that the
// new visual polish leans on. drawPixel is the rasterizer's per-cell rate
// (clipping, color clamp, MapSurfaceRGBA, one 32-bit store) and acts as
// the floor every higher-level draw rides on top of. drawText hits the
// font.cpp text-surface cache: the warmup pre-fills the cache and the
// timed loop measures the cached lookup + blit path (the actual
// TTF_RenderText_Blended cost only happens once). Label::draw with bold
// true exercises the fake-bold double-blit so any regression in the
// percent-readout path shows up as a draws/sec drop here.

namespace
{
    constexpr int CANVAS_W = 960;
    constexpr int CANVAS_H = 540;
    constexpr std::size_t LINE_COUNT = 200000;
    constexpr int WARMUP_LINES = 1000;

    constexpr std::size_t SPARKLINE_CAPACITY = 64;
    constexpr int SPARKLINE_MIN_X = 22;
    constexpr int SPARKLINE_MIN_Y = 60;
    constexpr int SPARKLINE_MAX_X = 938;
    constexpr int SPARKLINE_MAX_Y = 88;
    constexpr std::size_t SPARKLINE_DRAWS = 800;
    constexpr std::size_t SPARKLINE_WARMUP = 16;

    constexpr int BAR_MIN_X = 84;
    constexpr int BAR_MIN_Y = 92;
    constexpr int BAR_MAX_X = 820;
    constexpr int BAR_MAX_Y = 116;
    constexpr std::size_t BAR_DRAWS = 5000;
    constexpr std::size_t BAR_WARMUP = 100;

    // drawPixel rate. PIXEL_COUNT scattered writes into the canvas, sized
    // to keep the run inside ~1s on a release build while still pinning
    // the loop above noise. A precomputed coord+color array means the
    // timed section is pure rasterizer cost (clip + clamp + store), not
    // RNG churn.
    constexpr std::size_t PIXEL_COUNT = 5000000;
    constexpr std::size_t PIXEL_WARMUP = 1000;

    // drawText rate via Screen::drawText. The string is intentionally 30
    // chars to mirror the longest readouts the app draws (e.g. process
    // command tails); WARMUP just runs the same call once so the
    // (text, color) entry lands in font.cpp's text-surface cache, then
    // the timed loop measures the cached lookup + blit path.
    constexpr char DRAW_TEXT_STRING[] = "CPU 100% RAM 100% GPU 100% OK!";
    constexpr std::size_t DRAW_TEXT_DRAWS = 100000;
    constexpr std::size_t DRAW_TEXT_WARMUP = 4;
    constexpr int DRAW_TEXT_X = 16;
    constexpr int DRAW_TEXT_Y = 16;

    // Label::draw rate with bold true so the fake-bold double-blit path
    // is the one being measured. Same caching story as drawText: the
    // glyph surface is rendered once during warmup and every timed
    // iteration is two cached blits + the Label dispatch overhead.
    constexpr char LABEL_STRING[] = "CPU 100% RAM 100% GPU 100% OK!";
    constexpr std::size_t LABEL_DRAWS = 100000;
    constexpr std::size_t LABEL_WARMUP = 4;
    constexpr int LABEL_X = 16;
    constexpr int LABEL_Y = 40;

    struct LineResult
    {
        double seconds;
        double linesPerSec;
        double pixelsPerSec;
    };

    struct OpsResult
    {
        double seconds;
        double opsPerSec;
    };

    LineResult benchBresenham()
    {
        Screen screen(CANVAS_W, CANVAS_H);

        std::mt19937 rng(42);
        std::uniform_int_distribution<int> xDist(0, CANVAS_W - 1);
        std::uniform_int_distribution<int> yDist(0, CANVAS_H - 1);

        std::vector<std::pair<ivec2, ivec2>> lines;
        lines.reserve(LINE_COUNT);
        for (std::size_t i = 0; i < LINE_COUNT; ++i)
        {
            lines.emplace_back(ivec2(xDist(rng), yDist(rng)),
                               ivec2(xDist(rng), yDist(rng)));
        }
        const vec3 color(1.0f, 1.0f, 1.0f);

        for (int i = 0; i < WARMUP_LINES; ++i)
        {
            screen.drawLine(lines[i].first, lines[i].second, color);
        }
        screen.clear();

        std::uint64_t pixelEstimate = 0;
        auto t0 = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < LINE_COUNT; ++i)
        {
            screen.drawLine(lines[i].first, lines[i].second, color);
            int dx = lines[i].second.x - lines[i].first.x;
            int dy = lines[i].second.y - lines[i].first.y;
            int adx = dx < 0 ? -dx : dx;
            int ady = dy < 0 ? -dy : dy;
            pixelEstimate += static_cast<std::uint64_t>(adx > ady ? adx : ady) + 1;
        }
        auto t1 = std::chrono::steady_clock::now();
        double seconds = std::chrono::duration<double>(t1 - t0).count();

        LineResult r;
        r.seconds = seconds;
        r.linesPerSec = static_cast<double>(LINE_COUNT) / seconds;
        r.pixelsPerSec = static_cast<double>(pixelEstimate) / seconds;
        return r;
    }

    // Sparkline frame rate. One Sparkline (capacity 64), filled with 64
    // random samples in [0, 100], then redrawn SPARKLINE_DRAWS times.
    // draw() is const so the buffer state stays put across iterations and
    // each iteration does the same triangle-fill + 3px stroke work; the
    // canvas is intentionally left dirty between draws (same convention as
    // the Bresenham leg above) so we don't double-count Screen::clear.
    OpsResult benchSparkline()
    {
        Screen screen(CANVAS_W, CANVAS_H);

        vec3 lineColor(0.4f, 0.85f, 0.95f);
        Sparkline spark(ivec2(SPARKLINE_MIN_X, SPARKLINE_MIN_Y),
                        ivec2(SPARKLINE_MAX_X, SPARKLINE_MAX_Y),
                        lineColor, 0.0f, 100.0f, SPARKLINE_CAPACITY);

        std::mt19937 rng(1337);
        std::uniform_real_distribution<float> vDist(0.0f, 100.0f);
        for (std::size_t i = 0; i < SPARKLINE_CAPACITY; ++i)
        {
            spark.push(vDist(rng));
        }

        for (std::size_t i = 0; i < SPARKLINE_WARMUP; ++i)
        {
            spark.draw(screen);
        }
        screen.clear();

        auto t0 = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < SPARKLINE_DRAWS; ++i)
        {
            spark.draw(screen);
        }
        auto t1 = std::chrono::steady_clock::now();
        double seconds = std::chrono::duration<double>(t1 - t0).count();

        OpsResult r;
        r.seconds = seconds;
        r.opsPerSec = static_cast<double>(SPARKLINE_DRAWS) / seconds;
        return r;
    }

    // Bar draw rate. One Bar at (84,92)..(820,116) with the new gradient
    // / depth overlay path. The value cycles 0..100 modulo so each
    // iteration crosses both the YELLOW and RED threshold colors at least
    // once across the run, which keeps the threshold-recolor branch in
    // src/Bar.cpp warm instead of stuck on a single fill color.
    OpsResult benchBar()
    {
        Screen screen(CANVAS_W, CANVAS_H);

        vec3 fillColor(0.30f, 0.75f, 0.55f);
        vec3 bgColor(0.10f, 0.12f, 0.14f);
        Bar bar(ivec2(BAR_MIN_X, BAR_MIN_Y), ivec2(BAR_MAX_X, BAR_MAX_Y),
                fillColor, bgColor, 0.0f, 100.0f, "bench");

        for (std::size_t i = 0; i < BAR_WARMUP; ++i)
        {
            bar.setValue(static_cast<float>(i % 101));
            bar.draw(screen);
        }
        screen.clear();

        auto t0 = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < BAR_DRAWS; ++i)
        {
            bar.setValue(static_cast<float>(i % 101));
            bar.draw(screen);
        }
        auto t1 = std::chrono::steady_clock::now();
        double seconds = std::chrono::duration<double>(t1 - t0).count();

        OpsResult r;
        r.seconds = seconds;
        r.opsPerSec = static_cast<double>(BAR_DRAWS) / seconds;
        return r;
    }

    // Single-pixel rasterizer rate. Coordinates are precomputed into a
    // vector so the timed loop is pure Screen::drawPixel work: bounds
    // check, color clamp, MapSurfaceRGBA, one 32-bit store. Color is held
    // constant so the clamp can hoist; the only varying input is the
    // pixel coordinate, which is what every higher-level draw ends up
    // multiplying.
    OpsResult benchDrawPixel()
    {
        Screen screen(CANVAS_W, CANVAS_H);

        std::mt19937 rng(2024);
        std::uniform_int_distribution<int> xDist(0, CANVAS_W - 1);
        std::uniform_int_distribution<int> yDist(0, CANVAS_H - 1);

        std::vector<ivec2> points;
        points.reserve(PIXEL_COUNT);
        for (std::size_t i = 0; i < PIXEL_COUNT; ++i)
        {
            points.emplace_back(xDist(rng), yDist(rng));
        }
        const vec3 color(0.85f, 0.85f, 0.95f);

        for (std::size_t i = 0; i < PIXEL_WARMUP; ++i)
        {
            screen.drawPixel(points[i], color);
        }
        screen.clear();

        auto t0 = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < PIXEL_COUNT; ++i)
        {
            screen.drawPixel(points[i], color);
        }
        auto t1 = std::chrono::steady_clock::now();
        double seconds = std::chrono::duration<double>(t1 - t0).count();

        OpsResult r;
        r.seconds = seconds;
        r.opsPerSec = static_cast<double>(PIXEL_COUNT) / seconds;
        return r;
    }

    // Screen::drawText rate on the cached path. font.cpp keys glyph
    // surfaces on (text, RGB), so the first call renders + caches and
    // every subsequent call with the same string + color is a hash
    // lookup plus a blit. The warmup phase pre-populates that entry so
    // the timed loop measures only the lookup + blit cost, which is the
    // realistic per-frame number once the UI is settled.
    OpsResult benchDrawText()
    {
        Screen screen(CANVAS_W, CANVAS_H);

        const vec3 color(0.90f, 0.92f, 0.96f);
        std::string text(DRAW_TEXT_STRING);
        ivec2 pos(DRAW_TEXT_X, DRAW_TEXT_Y);

        for (std::size_t i = 0; i < DRAW_TEXT_WARMUP; ++i)
        {
            screen.drawText(pos, color, text);
        }
        screen.clear();

        auto t0 = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < DRAW_TEXT_DRAWS; ++i)
        {
            screen.drawText(pos, color, text);
        }
        auto t1 = std::chrono::steady_clock::now();
        double seconds = std::chrono::duration<double>(t1 - t0).count();

        OpsResult r;
        r.seconds = seconds;
        r.opsPerSec = static_cast<double>(DRAW_TEXT_DRAWS) / seconds;
        return r;
    }

    // Label::draw rate with bold true. Routes through Font::drawTextBold
    // which blits the cached glyph surface twice (once at pos, once at
    // pos + 1px on x) to fake a bold weight. Same warmup story so the
    // timed loop hits the cache; what's being measured is the Label
    // dispatch overhead plus the double-blit cost on top of one cache
    // lookup per call.
    OpsResult benchLabelDraw()
    {
        Screen screen(CANVAS_W, CANVAS_H);

        Label label(LABEL_STRING, ivec2(LABEL_X, LABEL_Y),
                    vec3(0.95f, 0.95f, 0.95f));
        label.setBold(true);

        for (std::size_t i = 0; i < LABEL_WARMUP; ++i)
        {
            label.draw(screen);
        }
        screen.clear();

        auto t0 = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < LABEL_DRAWS; ++i)
        {
            label.draw(screen);
        }
        auto t1 = std::chrono::steady_clock::now();
        double seconds = std::chrono::duration<double>(t1 - t0).count();

        OpsResult r;
        r.seconds = seconds;
        r.opsPerSec = static_cast<double>(LABEL_DRAWS) / seconds;
        return r;
    }

    void printLineRow(const char *label, const LineResult &r)
    {
        std::cout << "  "
                  << std::left << std::setw(28) << label
                  << "  "
                  << std::right << std::setw(12) << std::fixed << std::setprecision(3) << r.seconds << " s"
                  << "  "
                  << std::setw(14) << std::scientific << std::setprecision(3) << r.linesPerSec << " lines/s"
                  << "  "
                  << std::setw(14) << r.pixelsPerSec << " px/s"
                  << '\n';
    }

    void printOpsRow(const char *label, const OpsResult &r)
    {
        std::cout << "  "
                  << std::left << std::setw(28) << label
                  << "  "
                  << std::right << std::setw(12) << std::fixed << std::setprecision(3) << r.seconds << " s"
                  << "  "
                  << std::setw(14) << std::scientific << std::setprecision(3) << r.opsPerSec << " ops/sec"
                  << '\n';
    }
}

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 1;
    }

    std::cout << "==================================================================\n";
    std::cout << "  CoreMetrics bench: rasterizer + visual polish\n";
    std::cout << "==================================================================\n";
    std::cout << "  Canvas:           " << CANVAS_W << "x" << CANVAS_H << '\n';
    std::cout << "  Bresenham lines:  " << LINE_COUNT << " (warmup " << WARMUP_LINES << ")\n";
    std::cout << "  Sparkline draws:  " << SPARKLINE_DRAWS
              << " (capacity " << SPARKLINE_CAPACITY
              << ", rect " << (SPARKLINE_MAX_X - SPARKLINE_MIN_X) << "x"
              << (SPARKLINE_MAX_Y - SPARKLINE_MIN_Y) << ")\n";
    std::cout << "  Bar draws:        " << BAR_DRAWS
              << " (rect " << (BAR_MAX_X - BAR_MIN_X) << "x"
              << (BAR_MAX_Y - BAR_MIN_Y) << ")\n";
    std::cout << "  drawPixel writes: " << PIXEL_COUNT
              << " (warmup " << PIXEL_WARMUP << ")\n";
    std::cout << "  drawText draws:   " << DRAW_TEXT_DRAWS
              << " (string len " << (sizeof(DRAW_TEXT_STRING) - 1) << ")\n";
    std::cout << "  Label draws:      " << LABEL_DRAWS
              << " (bold, string len " << (sizeof(LABEL_STRING) - 1) << ")\n";
    std::cout << '\n';

    LineResult lineR = benchBresenham();
    printLineRow("drawLine (random)", lineR);

    OpsResult sparkR = benchSparkline();
    printOpsRow("Sparkline::draw (64 samples)", sparkR);

    OpsResult barR = benchBar();
    printOpsRow("Bar::draw (gradient + depth)", barR);

    OpsResult pixelR = benchDrawPixel();
    printOpsRow("Screen::drawPixel", pixelR);

    OpsResult textR = benchDrawText();
    printOpsRow("Screen::drawText (cached)", textR);

    OpsResult labelR = benchLabelDraw();
    printOpsRow("Label::draw (bold)", labelR);

    Font::shutdown();
    SDL_Quit();
    return 0;
}
