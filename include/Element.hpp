#ifndef __ELEMENT_HPP__
#define __ELEMENT_HPP__

#include "screen.hpp"

// abstact base class for GUI elements
class Element
{
public:
    virtual ~Element() {}
    virtual void draw(Screen& screen) = 0;
};


#endif