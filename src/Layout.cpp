#include "Layout.hpp"

Layout::Layout(ivec2 start, ivec2 end) : m_start(start), m_end(end), m_active(true)
{
}

void Layout::addElement(GUIElement* element)
{
    m_elements.push_back(element);
}

void Layout::setActive(bool active)
{
    m_active = active;
}

bool Layout::isActive() const
{
    return m_active;
}

ivec2 Layout::getStart() const
{
    return m_start;
}

ivec2 Layout::getEnd() const
{
    return m_end;
}

ivec2 Layout::resolveAbsStart(ivec2 offset) const
{
    return m_start + offset;
}

ivec2 Layout::resolveAbsEnd(ivec2 offset) const
{
    return m_end + offset;
}

void Layout::draw(Screen& screen)
{
    if (!m_active)
    {
        return;
    }

    for (GUIElement* element : m_elements)
    {
        element->draw(screen);
    }
}
