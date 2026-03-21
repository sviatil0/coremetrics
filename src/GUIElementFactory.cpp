#include <iostream>
#include "GUIElementFactory.hpp"

std::unique_ptr<GUIElement> GUIElementFactory::createPoint(vec2 pos, vec3 color)
{
    return std::make_unique<Point>(pos, color);
}

std::unique_ptr<GUIElement> GUIElementFactory::createLine(vec2 start, vec2 end, vec3 color)
{
    return std::make_unique<Line>(start, end, color);
}

std::unique_ptr<GUIElement> GUIElementFactory::createBox(vec2 minPos, vec2 maxPos, vec3 color)
{
    return std::make_unique<Box>(minPos, maxPos, color);
}

std::unique_ptr<GUIElement> GUIElementFactory::create(GUIElementType type, vec2 pos1, vec2 pos2, vec3 color)
{
    switch (type)
    {
    case GUIElementType::POINT:
        return createPoint(pos1, color);
    case GUIElementType::LINE:
        return createLine(pos1, pos2, color);
    case GUIElementType::BOX:
        return createBox(pos1, pos2, color);
    }
    std::cerr << "GUIElementFactory: unknown element type\n";
    return nullptr;
}
