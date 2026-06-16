#include "Heatmap.hpp"
#include "Thresholds.hpp"

Heatmap::Heatmap(ivec2 minPos, ivec2 maxPos, std::size_t cols, std::size_t rows,
                 vec3 bgColor, int gap)
    : minPos(minPos), maxPos(maxPos), cols(cols), rows(rows),
      values(cols * rows, 0.0f), bgColor(bgColor), gap(gap)
{
}

void Heatmap::setValue(std::size_t row, std::size_t col, float pct)
{
    if (row >= rows || col >= cols)
    {
        return;
    }
    if (pct < 0.0f)
    {
        pct = 0.0f;
    }
    else if (pct > 100.0f)
    {
        pct = 100.0f;
    }
    values[row * cols + col] = pct;
}

float Heatmap::getValue(std::size_t row, std::size_t col) const
{
    if (row >= rows || col >= cols)
    {
        return 0.0f;
    }
    return values[row * cols + col];
}

std::size_t Heatmap::getCols() const
{
    return cols;
}

std::size_t Heatmap::getRows() const
{
    return rows;
}

ivec2 Heatmap::getMinPos() const
{
    return minPos;
}

ivec2 Heatmap::getMaxPos() const
{
    return maxPos;
}

void Heatmap::draw(Screen &screen)
{
    screen.drawBox(minPos, maxPos, bgColor);

    if (cols == 0 || rows == 0)
    {
        return;
    }

    int totalWidth = maxPos.x - minPos.x;
    int totalHeight = maxPos.y - minPos.y;
    int gapsW = static_cast<int>(cols - 1) * gap;
    int gapsH = static_cast<int>(rows - 1) * gap;
    int cellWidth = (totalWidth - gapsW) / static_cast<int>(cols);
    int cellHeight = (totalHeight - gapsH) / static_cast<int>(rows);
    if (cellWidth <= 0 || cellHeight <= 0)
    {
        return;
    }

    for (std::size_t r = 0; r < rows; ++r)
    {
        for (std::size_t c = 0; c < cols; ++c)
        {
            int x0 = minPos.x + static_cast<int>(c) * (cellWidth + gap);
            int y0 = minPos.y + static_cast<int>(r) * (cellHeight + gap);
            ivec2 cellMin(x0, y0);
            ivec2 cellMax(x0 + cellWidth, y0 + cellHeight);
            vec3 cellColor = Thresholds::colorForPct(values[r * cols + c]);
            screen.drawBox(cellMin, cellMax, cellColor);
        }
    }
}
