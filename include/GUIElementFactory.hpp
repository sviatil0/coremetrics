#ifndef __GUIELEMENTFACTORY_HPP__
#define __GUIELEMENTFACTORY_HPP__

#include <memory>
#include <string>
#include <vector>
#include "GUIElements.hpp"

class GUIElementFactory
{
public:
    static std::unique_ptr<GUIElement> createPoint(vec2 pos, vec3 color);
    static std::unique_ptr<GUIElement> createLine(vec2 start, vec2 end, vec3 color);
    static std::unique_ptr<GUIElement> createBox(vec2 minPos, vec2 maxPos, vec3 color);
    static std::unique_ptr<GUIElement> createBar(ivec2 minPos, ivec2 maxPos,
                                                 vec3 fillColor, vec3 bgColor,
                                                 float minVal, float maxVal,
                                                 std::string metricName);
    static std::unique_ptr<GUIElement> createRow(ivec2 minPos, ivec2 maxPos,
                                                 std::vector<std::string> cells,
                                                 std::vector<float> columnWeights,
                                                 vec3 textColor);
    static std::unique_ptr<GUIElement> createLabel(std::string text, ivec2 pos, vec3 color);
    static std::unique_ptr<GUIElement> createButton(ivec2 minPos, ivec2 maxPos, vec3 color,
                                                    std::string soundFile,
                                                    std::string targetLayout,
                                                    std::string targetLayoutHide);
    static std::unique_ptr<GUIElement> create(GUIElementType type, vec2 pos1, vec2 pos2, vec3 color);
};

#endif
