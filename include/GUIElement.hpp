#ifndef __GUIELEMENT_HPP__
#define __GUIELEMENT_HPP__

#include "screen.hpp"
#include "Event.hpp"

class GUIElement
{
public:
    virtual ~GUIElement() {}
    virtual void draw(Screen& screen) = 0;
    virtual bool operator()(Event* event) { return false; }
};

#endif
