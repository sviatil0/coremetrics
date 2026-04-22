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

std::unique_ptr<GUIElement> GUIElementFactory::createBar(ivec2 minPos, ivec2 maxPos,
                                                         vec3 fillColor, vec3 bgColor,
                                                         float minVal, float maxVal,
                                                         std::string metricName)
{
    return std::make_unique<Bar>(minPos, maxPos, fillColor, bgColor, minVal, maxVal, std::move(metricName));
}

std::unique_ptr<GUIElement> GUIElementFactory::createRow(ivec2 minPos, ivec2 maxPos,
                                                         std::vector<std::string> cells,
                                                         std::vector<float> columnWeights,
                                                         vec3 textColor)
{
    return std::make_unique<Row>(minPos, maxPos, std::move(cells), std::move(columnWeights), textColor);
}

std::unique_ptr<GUIElement> GUIElementFactory::createLabel(std::string text, ivec2 pos, vec3 color)
{
    return std::make_unique<Label>(std::move(text), pos, color);
}

std::unique_ptr<GUIElement> GUIElementFactory::createButton(ivec2 minPos, ivec2 maxPos, vec3 color,
                                                            std::string soundFile,
                                                            std::string targetLayout,
                                                            std::string targetLayoutHide)
{
    return std::make_unique<Button>(minPos, maxPos, color,
                                    std::move(soundFile),
                                    std::move(targetLayout),
                                    std::move(targetLayoutHide));
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
    case GUIElementType::BAR:
    case GUIElementType::ROW:
    case GUIElementType::LABEL:
    case GUIElementType::BUTTON:
        std::cerr << "GUIElementFactory: type requires specialized overload\n";
        return nullptr;
    }
    std::cerr << "GUIElementFactory: unknown element type\n";
    return nullptr;
}
