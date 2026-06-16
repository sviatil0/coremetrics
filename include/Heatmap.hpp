#ifndef __HEATMAP_HPP__
#define __HEATMAP_HPP__

#include <cstddef>
#include <vector>
#include "Cloneable.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Heatmap: a 2D grid of color-coded cells. Each cell holds a 0..100
// percentage and is filled with Thresholds::colorForPct(value) at draw
// time, so the green/yellow/red palette stays consistent with Bar,
// Sparkline, and the per-core strip. Useful for per-core CPU
// visualization (cols = core count, rows = 1) or richer 2D layouts
// like a GPU SM occupancy grid.
class Heatmap : public Cloneable<Heatmap>
{
private:
    ivec2 minPos;
    ivec2 maxPos;
    std::size_t cols;
    std::size_t rows;
    std::vector<float> values;
    vec3 bgColor;
    int gap;

public:
    Heatmap(ivec2 minPos, ivec2 maxPos, std::size_t cols, std::size_t rows,
            vec3 bgColor, int gap = 1);

    void setValue(std::size_t row, std::size_t col, float pct);
    float getValue(std::size_t row, std::size_t col) const;

    std::size_t getCols() const;
    std::size_t getRows() const;
    ivec2 getMinPos() const;
    ivec2 getMaxPos() const;

    void draw(Screen &screen) override;
};

#endif
