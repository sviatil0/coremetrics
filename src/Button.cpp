#include "Button.hpp"

Button::Button(ivec2 minP, ivec2 maxP, vec3 col)
    : minPos(minP), maxPos(maxP), color(col)
{
}

void Button::draw(Screen& screen)
{
    //like selection element, draw white border around button element
    vec3 borderColor = { 1.0f, 1.0f, 1.0f }; // White border
    screen.drawBox(minPos, maxPos, borderColor);
    
    ivec2 innerMin = { minPos.x + 1, minPos.y + 1 };
    ivec2 innerMax = { maxPos.x - 1, maxPos.y - 1 };
    screen.drawBox(innerMin, innerMax, color);

    //probably add label implementation here later
}

bool Button::checkToggle(int mouseX, int mouseY)
{
    if (((mouseX >= minPos.x) && (mouseX <= maxPos.x))
        && ((mouseY >= minPos.y) && (mouseY <= maxPos.y)))
    {
        return true;
    }
    return false;
}
