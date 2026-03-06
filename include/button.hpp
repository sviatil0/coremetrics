#ifndef __BUTTON_HPP__
#define __BUTTON_HPP__

#include "GUIElement.hpp"
#include "linear.hpp"

class Button : public GUIElement
{
private:
    ivec2 minPos;
    ivec2 maxPos;
    vec3 color;

public:
    Button(ivec2 minP, ivec2 maxP, vec3 col);

    ~Button() {}

    void draw(Screen& screen) override;

    bool checkToggle(int mouseX, int mouseY);
};

#endif