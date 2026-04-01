#ifndef __LAYOUTMANAGER_HPP__
#define __LAYOUTMANAGER_HPP__

#include "Layout.hpp"
#include "Tree.hpp"
#include "screen.hpp"
#include "vec2.hpp"

class LayoutManager
{
private:
    Tree<Layout> root;

    LayoutManager();
    LayoutManager(const LayoutManager&) = delete;
    LayoutManager& operator=(const LayoutManager&) = delete;

    static void renderTree(const Tree<Layout>& node, Screen& screen, ivec2 parentStart, ivec2 parentEnd);

public:
    static LayoutManager& getInstance();

    Tree<Layout>& getRoot();
    Tree<Layout>* addChild(Tree<Layout>* parent, Layout layout);
    void render(Screen& screen, ivec2 screenStart, ivec2 screenEnd) const;

    void clear();
};

#endif
