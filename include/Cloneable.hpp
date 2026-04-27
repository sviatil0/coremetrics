#ifndef __CLONEABLE_HPP__
#define __CLONEABLE_HPP__

#include "GUIElement.hpp"
#include <memory>

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

template <typename T>
std::unique_ptr<T> cloneUnique(const T& obj)
{
    return std::unique_ptr<T>(static_cast<T*>(obj.clone()));
}

#endif