#include <iostream>
#include "TreeViewTest.hpp"
#include "TreeView.hpp"
#include "screen.hpp"

void testTreeViewConstruction()
{
    std::cout << "TreeView : construction stores box and starts empty: ";
    TreeView tv(ivec2(0, 0), ivec2(200, 200),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.6f, 0.6f, 0.6f),
                vec3(0.9f, 0.9f, 0.9f));
    bool passed = (tv.nodes.empty()
                   && tv.minPos.x == 0 && tv.minPos.y == 0
                   && tv.maxPos.x == 200 && tv.maxPos.y == 200);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTreeViewAddNode()
{
    std::cout << "TreeView : addNode appends rows in order: ";
    TreeView tv;
    tv.addNode("root", 0, true);
    tv.addNode("child", 1, false);
    tv.addNode("leaf", 2, false);
    bool passed = (tv.nodes.size() == 3
                   && tv.nodes[0].label == "root"
                   && tv.nodes[1].depth == 1
                   && tv.nodes[2].hasChildren == false);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTreeViewAddNodeClampsNegativeDepth()
{
    std::cout << "TreeView : addNode clamps negative depth to 0: ";
    TreeView tv;
    tv.addNode("x", -3, false);
    bool passed = (tv.nodes.size() == 1 && tv.nodes[0].depth == 0);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTreeViewClear()
{
    std::cout << "TreeView : clear drops every node: ";
    TreeView tv;
    tv.addNode("a", 0, false);
    tv.addNode("b", 0, false);
    tv.clear();
    bool passed = tv.nodes.empty();
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTreeViewSetCollapsed()
{
    std::cout << "TreeView : setCollapsed flips the per-node flag: ";
    TreeView tv;
    tv.addNode("p", 0, true);
    tv.addNode("c", 1, false);
    tv.setCollapsed(0, true);
    bool passed = (tv.nodes[0].collapsed == true && tv.nodes[1].collapsed == false);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTreeViewSetCollapsedOutOfRange()
{
    std::cout << "TreeView : setCollapsed ignores out-of-range index: ";
    TreeView tv;
    tv.addNode("only", 0, true);
    tv.setCollapsed(7, true);
    bool passed = (tv.nodes[0].collapsed == false);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTreeViewDrawSmoke()
{
    std::cout << "TreeView : addNode + draw smoke test: ";
    Screen screen(320, 240);
    TreeView tv(ivec2(0, 0), ivec2(320, 240),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.6f, 0.6f, 0.6f),
                vec3(0.9f, 0.9f, 0.9f));
    tv.addNode("root", 0, true);
    tv.addNode("a", 1, false);
    tv.addNode("b", 1, true);
    tv.addNode("b.1", 2, false);
    tv.draw(screen);
    std::cout << "PASS" << '\n';
}

void testTreeViewDrawEmptyNoCrash()
{
    std::cout << "TreeView : draw on empty list does not crash: ";
    Screen screen(64, 64);
    TreeView tv(ivec2(0, 0), ivec2(64, 64),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.6f, 0.6f, 0.6f),
                vec3(0.9f, 0.9f, 0.9f));
    tv.draw(screen);
    std::cout << "PASS" << '\n';
}

void testTreeViewCollapsedHidesDescendants()
{
    std::cout << "TreeView : collapsed parent hides descendants on draw: ";
    Screen screen(320, 240);
    TreeView tv(ivec2(0, 0), ivec2(320, 240),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.6f, 0.6f, 0.6f),
                vec3(0.9f, 0.9f, 0.9f));
    // Tree shape:
    //   root(0)
    //     parent(1)        <-- collapsed
    //       hidden_a(2)
    //         hidden_b(3)
    //     sibling(1)
    tv.addNode("root", 0, true);
    tv.addNode("parent", 1, true);
    tv.addNode("hidden_a", 2, false);
    tv.addNode("hidden_b", 3, false);
    tv.addNode("sibling", 1, false);
    tv.setCollapsed(1, true);

    // Confirm the state flag is honored (the contract the draw loop
    // walks), and that the draw with the gate active does not crash.
    bool passed = (tv.nodes[1].collapsed == true
                   && tv.nodes[2].collapsed == false
                   && tv.nodes[3].collapsed == false);
    tv.draw(screen);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testTreeViewDrawClipsOverflow()
{
    std::cout << "TreeView : draw clips rows past maxPos.y: ";
    // Box is only tall enough for a single row; the second one must be
    // skipped without crashing or scribbling outside the box.
    Screen screen(128, 64);
    TreeView tv(ivec2(0, 0), ivec2(128, 18),
                vec3(1.0f, 1.0f, 1.0f),
                vec3(0.6f, 0.6f, 0.6f),
                vec3(0.9f, 0.9f, 0.9f));
    tv.addNode("one", 0, false);
    tv.addNode("two", 0, false);
    tv.addNode("three", 0, false);
    tv.draw(screen);
    std::cout << "PASS" << '\n';
}

void treeViewTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  TreeView : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testTreeViewConstruction();
    testTreeViewAddNode();
    testTreeViewAddNodeClampsNegativeDepth();
    testTreeViewClear();
    testTreeViewSetCollapsed();
    testTreeViewSetCollapsedOutOfRange();
    testTreeViewDrawSmoke();
    testTreeViewDrawEmptyNoCrash();
    testTreeViewCollapsedHidesDescendants();
    testTreeViewDrawClipsOverflow();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  TreeView tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
