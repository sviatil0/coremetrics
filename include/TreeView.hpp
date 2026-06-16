#ifndef __TREE_VIEW_HPP__
#define __TREE_VIEW_HPP__

#include <cstddef>
#include <string>
#include <vector>
#include "Cloneable.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Generic indented row list widget. Each row carries a depth and a label,
// rendered as `"  " * depth + "|- " + label`. Rows whose hasChildren flag
// is set get an extra `+ ` (collapsed) or `- ` (expanded) glyph in front
// of the connector so a caller can build a fold/unfold view without
// having to hand-format every line. Rows nested under a collapsed parent
// are skipped at draw time. Row height is a fixed 18px; anything past
// maxPos.y is clipped silently so the widget never overflows its box.
class TreeView : public Cloneable<TreeView>
{
public:
    struct Node
    {
        std::string label;
        int depth;
        bool hasChildren;
        bool collapsed;
    };

    ivec2 minPos;
    ivec2 maxPos;
    std::vector<Node> nodes;
    vec3 textColor;
    vec3 connectorColor;
    vec3 glyphColor;

    TreeView();
    TreeView(ivec2 minPos, ivec2 maxPos, vec3 textColor,
             vec3 connectorColor, vec3 glyphColor);

    void addNode(std::string label, int depth, bool hasChildren);
    void clear();
    void setCollapsed(std::size_t index, bool collapsed);

    void draw(Screen &screen) override;
};

#endif
