#ifndef __LAYOUT_HPP__
#define __LAYOUT_HPP__

#include "GUIElement.hpp"
#include "linear.hpp"
#include <vector>

class Layout : public GUIElement
{
private:
    ivec2 m_start;
    ivec2 m_end;
    bool m_active;
    std::vector<GUIElement*> m_elements;

public:
    Layout(ivec2 start, ivec2 end);

    virtual ~Layout() = default;

    virtual void draw(Screen& screen) override;

    void addElement(GUIElement* element);
    void setActive(bool active);
    bool isActive() const;
    ivec2 getStart() const;
    ivec2 getEnd() const;
    ivec2 resolveAbsStart(ivec2 offset) const;
    ivec2 resolveAbsEnd(ivec2 offset) const;
};

#endif
