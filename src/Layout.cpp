#include "Layout.hpp"

Layout::Layout(vec2 start, vec2 end, bool active, std::string name) : start(start), end(end), active(active), name(name)
{
}

Layout::Layout(const Layout& other) : start(other.start), end(other.end), active(other.active), name(other.name)
{
    elements.reserve(other.elements.size());
    for (const auto& element : other.elements)
    {
        elements.emplace_back(std::unique_ptr<GUIElement>(element->clone()));
    }
}

Layout& Layout::operator=(const Layout& other)
{
    if (this == &other)
    {
        return *this;
    }
    start = other.start;
    end = other.end;
    active = other.active;
    name = other.name;
    elements.clear();
    elements.reserve(other.elements.size());
    for (const auto& element : other.elements)
    {
        elements.emplace_back(std::unique_ptr<GUIElement>(element->clone()));
    }
    return *this;
}

ivec2 Layout::resolveAbsStart(ivec2 parentStart, ivec2 parentEnd) const
{
    ivec2 cont = parentEnd - parentStart;
    return ivec2(
        parentStart.x + static_cast<int>(start.x * static_cast<float>(cont.x)),
        parentStart.y + static_cast<int>(start.y * static_cast<float>(cont.y))
    );
}

ivec2 Layout::resolveAbsEnd(ivec2 parentStart, ivec2 parentEnd) const
{
    ivec2 cont = parentEnd - parentStart;
    return ivec2(
        parentStart.x + static_cast<int>(end.x * static_cast<float>(cont.x)),
        parentStart.y + static_cast<int>(end.y * static_cast<float>(cont.y))
    );
}

void Layout::draw(Screen& screen, ivec2 parentStart, ivec2 parentEnd) const
{
    if (!active)
    {
        return;
    }
    ivec2 absStart = resolveAbsStart(parentStart, parentEnd);
    ivec2 absEnd = resolveAbsEnd(parentStart, parentEnd);
    for (const auto& element : elements)
    {
        element->draw(screen);
    }
}

void Layout::addElement(std::unique_ptr<GUIElement> element)
{
    elements.push_back(std::move(element));
}

void Layout::setActive(bool active)
{
    this->active = active;
}

bool Layout::isActive() const
{
    return active;
}

vec2 Layout::getStart() const
{
    return start;
}

vec2 Layout::getEnd() const
{
    return end;
}

void Layout::setName(std::string name)
{
    this->name = name;
} 

std::string Layout::getName() const
{
    return name;
}
