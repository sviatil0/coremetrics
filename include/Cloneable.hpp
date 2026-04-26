#ifndef __CLONEABLE_HPP__
#define __CLONEABLE_HPP__

#include "GUIElement.hpp"

template <typename Derived> //CRTP
class Cloneable : public GUIElement {
public:
    GUIElement* clone() const override
    {
        return new Derived(static_cast<const Derived&>(*this));
    }

    Derived* cloneDerived() const //demonstration of covariant return
    { 
        return static_cast<Derived*>(clone());
    }
};

#endif