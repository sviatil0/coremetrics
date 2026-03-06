#include "button.hpp"

Button::Button(ivec2 minPos, ivec2 maxPos, vec3 color, bool clicked = false)
    : minPos(minPos), maxPos(maxPos), color(color), clicked(clicked)
{
}

Button::~Button() 
{
}

void draw(Screen& screen) override
{
    //like selection element, draw white border around button element
    vec3 borderColor = { 1.0f, 1.0f, 1.0f }; // White border
    screen.drawBox(minPos, maxPos, borderColor);
    
    ivec2 innerMin = { minPos.x + 1, minPos.y + 1 };
    ivec2 innerMax = { maxPos.x - 1, maxPos.y - 1 };
    screen.drawBox(innerMin, innerMax, color);

    //probably add label implementation here later
}

bool checkToggle(int mouseX, int mouseY)
{
    if (((mouseX >= minPos.x) && (mouseX <= maxPos.x))
        && ((mouseY >= minPos.y) && (mouseY <= maxPos.y)))
    {
        return true;
    }
    else
    {
        return false;
    }
}
