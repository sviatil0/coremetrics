#include "Line.hpp"

void Line::draw(Screen &screen)
{
    screen.drawLine(
        ivec2(static_cast<int>(start.x), static_cast<int>(start.y)),
        ivec2(static_cast<int>(end.x), static_cast<int>(end.y)),
        color);
}
