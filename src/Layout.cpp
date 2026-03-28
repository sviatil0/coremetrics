#include "Layout.hpp"

Layout::Layout(vec2 start, vec2 end, bool active) : start(start), end(end), active(active)
{
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
