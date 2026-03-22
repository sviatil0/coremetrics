#include "Box.hpp"

void Box::draw(Screen &screen)
{
    screen.drawBox(
        ivec2(static_cast<int>(minPos.x), static_cast<int>(minPos.y)),
        ivec2(static_cast<int>(maxPos.x), static_cast<int>(maxPos.y)),
        color);
}
