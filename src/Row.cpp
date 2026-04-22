#include "Row.hpp"
#include "font.hpp"

Row::Row(ivec2 minPos, ivec2 maxPos,
         std::vector<std::string> cells,
         std::vector<float> columnWeights,
         vec3 textColor)
    : minPos(minPos), maxPos(maxPos),
      cells(std::move(cells)),
      columnWeights(std::move(columnWeights)),
      textColor(textColor)
{
}

void Row::setCells(std::vector<std::string> newCells)
{
    cells = std::move(newCells);
}

const std::vector<std::string>& Row::getCells() const
{
    return cells;
}

const std::vector<float>& Row::getColumnWeights() const
{
    return columnWeights;
}

ivec2 Row::getMinPos() const
{
    return minPos;
}

ivec2 Row::getMaxPos() const
{
    return maxPos;
}

void Row::draw(Screen &screen)
{
    if (cells.empty() || columnWeights.empty())
    {
        return;
    }

    float totalWeight = 0.0f;
    for (float w : columnWeights)
    {
        totalWeight += w;
    }
    if (totalWeight <= 0.0f)
    {
        return;
    }

    int rowWidth = maxPos.x - minPos.x;
    int currentX = minPos.x;

    std::size_t numCols = cells.size();
    if (columnWeights.size() < numCols)
    {
        numCols = columnWeights.size();
    }

    for (std::size_t col = 0; col < numCols; ++col)
    {
        int colWidth = static_cast<int>((columnWeights[col] / totalWeight) * static_cast<float>(rowWidth));
        Font::drawText(screen, cells[col], ivec2(currentX + 4, minPos.y), textColor);
        currentX += colWidth;
    }
}
