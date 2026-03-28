#ifndef __LAYOUT_HPP__
#define __LAYOUT_HPP__

#include <vector>
#include <memory>
#include "GUIElement.hpp"
#include "screen.hpp"
#include "vec2.hpp"

class Layout
{
private:
    vec2 start;
    vec2 end;
    bool active;
    

public:
    Layout(vec2 start, vec2 end, bool active = true);

    void addElement(std::unique_ptr<GUIElement> element);
    void setActive(bool active);
    bool isActive() const;
    vec2 getStart() const;
    vec2 getEnd() const;

    std::vector<std::unique_ptr<GUIElement>> elements; // MADE THIS PUBLIC FOR TESTING PURPOSES, SHOULD ADD getElements METHOD

    ivec2 resolveAbsStart(ivec2 parentStart, ivec2 parentEnd) const;
    ivec2 resolveAbsEnd(ivec2 parentStart, ivec2 parentEnd) const;

    void draw(Screen& screen, ivec2 parentStart, ivec2 parentEnd) const;
};

#endif
