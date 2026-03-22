#include "GUIElements.hpp"

void Point::draw(Screen &screen)
{
    screen.drawPixel(ivec2(static_cast<int>(pos.x), static_cast<int>(pos.y)), color);
}

void Line::draw(Screen &screen)
{
    screen.drawLine(
        ivec2(static_cast<int>(start.x), static_cast<int>(start.y)),
        ivec2(static_cast<int>(end.x), static_cast<int>(end.y)),
        color);
}

void Box::draw(Screen &screen)
{
    screen.drawBox(
        ivec2(static_cast<int>(minPos.x), static_cast<int>(minPos.y)),
        ivec2(static_cast<int>(maxPos.x), static_cast<int>(maxPos.y)),
        color);
}
