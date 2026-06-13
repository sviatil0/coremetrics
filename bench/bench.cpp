#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>
#include <SDL3/SDL.h>
#include "screen.hpp"

// Tiny micro-benchmark for the Bresenham line rasterizer in src/screen.cpp.
// Reports lines/sec for random-direction lines drawn into an offscreen
// software surface, so the number is a real measurement of the rasterizer +
// drawPixel path without any windowing or vsync in the way. The aggregate
// pixel rate is also useful for sanity-checking the box / triangle fills
// since they share the same drawPixel core.

namespace
{
    constexpr int CANVAS_W = 960;
    constexpr int CANVAS_H = 540;
    constexpr std::size_t LINE_COUNT = 200000;
    constexpr int WARMUP_LINES = 1000;

    struct Result
    {
        double seconds;
        double linesPerSec;
        double pixelsPerSec;
    };

    Result benchBresenham()
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

        Result r;
        r.seconds = seconds;
        r.linesPerSec = static_cast<double>(LINE_COUNT) / seconds;
        r.pixelsPerSec = static_cast<double>(pixelEstimate) / seconds;
        return r;
    }

    void printRow(const char *label, const Result &r)
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
}

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 1;
    }

    std::cout << "==================================================================\n";
    std::cout << "  CoreMetrics bench: Bresenham line rasterizer\n";
    std::cout << "==================================================================\n";
    std::cout << "  Canvas:        " << CANVAS_W << "x" << CANVAS_H << '\n';
    std::cout << "  Lines:         " << LINE_COUNT << '\n';
    std::cout << "  Warmup lines:  " << WARMUP_LINES << '\n';
    std::cout << '\n';

    Result r = benchBresenham();
    printRow("drawLine (random)", r);

    SDL_Quit();
    return 0;
}
