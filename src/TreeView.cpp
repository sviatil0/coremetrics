#include "TreeView.hpp"
#include "font.hpp"

namespace
{
    // Fixed line height for the indented row list. Matches the rough cap
    // height of the bundled bitmap font plus 4px of vertical padding so
    // adjacent rows don't visually touch. The Processes-tab tree mode
    // uses the same value, which keeps the two views visually aligned
    // when a caller swaps one for the other.
    constexpr int kRowHeight = 18;

    // Horizontal pixel budget per depth level. Two spaces of the bundled
    // bitmap font work out to ~12px, but we under-shoot slightly so
    // deeply nested rows still fit inside narrow boxes (the Processes
    // tree can comfortably hit depth 6 on macOS).
    constexpr int kIndentPx = 12;
}

TreeView::TreeView()
    : minPos(), maxPos(), nodes(),
      textColor(1.0f, 1.0f, 1.0f),
      connectorColor(0.6f, 0.6f, 0.6f),
      glyphColor(1.0f, 1.0f, 1.0f)
{
}

TreeView::TreeView(ivec2 minPos, ivec2 maxPos, vec3 textColor,
                   vec3 connectorColor, vec3 glyphColor)
    : minPos(minPos), maxPos(maxPos), nodes(),
      textColor(textColor),
      connectorColor(connectorColor),
      glyphColor(glyphColor)
{
}

void TreeView::addNode(std::string label, int depth, bool hasChildren)
{
    if (depth < 0)
    {
        depth = 0;
    }
    nodes.push_back(Node{std::move(label), depth, hasChildren, false});
}

void TreeView::clear()
{
    nodes.clear();
}

void TreeView::setCollapsed(std::size_t index, bool collapsed)
{
    if (index >= nodes.size())
    {
        return;
    }
    nodes[index].collapsed = collapsed;
}

void TreeView::draw(Screen &screen)
{
    if (nodes.empty())
    {
        return;
    }

    // hideDepth tracks the depth at which a collapsed ancestor lives.
    // While the current node's depth is strictly greater than hideDepth,
    // it descends from that ancestor and must stay hidden. Once we walk
    // back out (depth <= hideDepth) the gate reopens. Using -1 as the
    // sentinel keeps the comparison branch-free for the common, all-
    // visible case where nothing is collapsed.
    int hideDepth = -1;
    int currentY = minPos.y;

    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        const Node &node = nodes[i];

        if (hideDepth >= 0)
        {
            if (node.depth > hideDepth)
            {
                continue;
            }
            hideDepth = -1;
        }

        if (currentY + kRowHeight > maxPos.y)
        {
            break;
        }

        int indentPx = node.depth * kIndentPx;
        int textX = minPos.x + indentPx;

        // The glyph marker for collapsible rows is drawn in a separate
        // color so callers can dim it relative to the row label. We
        // intentionally use plain ASCII `+`/`-` rather than the unicode
        // triangles so the bundled bitmap font (ASCII-only) renders it.
        if (node.hasChildren)
        {
            std::string glyph = node.collapsed ? "+ " : "- ";
            Font::drawText(screen, glyph, ivec2(textX, currentY), glyphColor);
            textX += 12;
        }

        Font::drawText(screen, "|- ", ivec2(textX, currentY), connectorColor);
        Font::drawText(screen, node.label, ivec2(textX + 18, currentY), textColor);

        if (node.hasChildren && node.collapsed)
        {
            hideDepth = node.depth;
        }

        currentY += kRowHeight;
    }
}
