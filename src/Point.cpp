#include "Point.hpp"

void Point::draw(Screen &screen)
{
    screen.drawPixel(ivec2(static_cast<int>(pos.x), static_cast<int>(pos.y)), color);
}
