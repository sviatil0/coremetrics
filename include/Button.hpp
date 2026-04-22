#ifndef __BUTTON_HPP__
#define __BUTTON_HPP__

#include <string>
#include "GUIElement.hpp"
#include "linear.hpp"

class Button : public GUIElement
{
private:
    ivec2 minPos;
    ivec2 maxPos;
    vec3 color;
    std::string soundFile;
    std::string targetLayout;
    std::string targetLayoutHide;

public:
    Button(ivec2 minP, ivec2 maxP, vec3 col,
           std::string soundFile = "",
           std::string targetLayout = "",
           std::string targetLayoutHide = "");

    ~Button() {}

    void draw(Screen& screen) override;

    bool checkToggle(int mouseX, int mouseY);

    bool operator()(Event* event) override;
};

#endif