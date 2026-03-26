#ifndef __GUIELEMENT_HPP__
#define __GUIELEMENT_HPP__

#include "screen.hpp"

// abstract base class for GUI elements
class GUIElement
{
public:
    virtual ~GUIElement() {}
    virtual void draw(Screen& screen) = 0;
};


#endif