#ifndef __GUIELEMENTFACTORY_HPP__
#define __GUIELEMENTFACTORY_HPP__

#include <memory>
#include "GUIElements.hpp"

class GUIElementFactory
{
public:
    static std::unique_ptr<GUIElement> createPoint(vec2 pos, vec3 color);
    static std::unique_ptr<GUIElement> createLine(vec2 start, vec2 end, vec3 color);
    static std::unique_ptr<GUIElement> createBox(vec2 minPos, vec2 maxPos, vec3 color);
    static std::unique_ptr<GUIElement> create(GUIElementType type, vec2 pos1, vec2 pos2, vec3 color);
};

#endif
