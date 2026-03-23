#include "LayoutManager.hpp"

LayoutManager::LayoutManager() : root(Layout(vec2(0.0f, 0.0f), vec2(1.0f, 1.0f)))
{
}

LayoutManager& LayoutManager::getInstance()
{
    static LayoutManager instance;
    return instance;
}

Tree<Layout>& LayoutManager::getRoot()
{
    return root;
}

Tree<Layout>* LayoutManager::addChild(Tree<Layout>* parent, Layout layout)
{
    return parent->addChild(std::move(layout));
}

void LayoutManager::renderTree(const Tree<Layout>& node, Screen& screen, ivec2 parentStart, ivec2 parentEnd)
{
    const Layout& layout = node.getData();
    layout.draw(screen, parentStart, parentEnd);
    ivec2 absStart = layout.resolveAbsStart(parentStart, parentEnd);
    ivec2 absEnd = layout.resolveAbsEnd(parentStart, parentEnd);
    for (const auto& child : node.getChildren())
    {
        renderTree(*child, screen, absStart, absEnd);
    }
}

void LayoutManager::render(Screen& screen, ivec2 screenStart, ivec2 screenEnd) const
{
    renderTree(root, screen, screenStart, screenEnd);
}
