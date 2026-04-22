#include "Row.hpp"

constexpr int CHAR_WIDTH = 8;
constexpr int CHAR_HEIGHT = 12;
constexpr int CHAR_GAP = 2;

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

    size_t numCols = cells.size();
    if (columnWeights.size() < numCols)
    {
        numCols = columnWeights.size();
    }

    for (size_t col = 0; col < numCols; ++col)
    {
        int colWidth = static_cast<int>((columnWeights[col] / totalWeight) * static_cast<float>(rowWidth));
        const std::string &text = cells[col];

        int maxChars = 0;
        if (CHAR_WIDTH > 0)
        {
            maxChars = colWidth / CHAR_WIDTH;
        }
        size_t charsToDraw = text.length();
        if (static_cast<size_t>(maxChars) < charsToDraw)
        {
            charsToDraw = static_cast<size_t>(maxChars);
        }

        for (size_t i = 0; i < charsToDraw; ++i)
        {
            ivec2 charMin(currentX + static_cast<int>(i) * CHAR_WIDTH, minPos.y);
            ivec2 charMax(charMin.x + CHAR_WIDTH - CHAR_GAP, minPos.y + CHAR_HEIGHT);
            screen.drawBox(charMin, charMax, textColor);
        }

        currentX += colWidth;
    }
}
