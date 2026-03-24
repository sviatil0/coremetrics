#include <iostream>
#include "LayoutManager.hpp"
#include "screen.hpp"
#include "LayoutManagerTest.hpp"

void testSingletonIdentity()
{
    std::cout << "LayoutManager : getInstance() returns same instance: ";
    LayoutManager &first = LayoutManager::getInstance();
    LayoutManager &second = LayoutManager::getInstance();
    bool passed = (&first == &second);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testGetRootIsRoot()
{
    std::cout << "LayoutManager : getRoot() is tree root: ";
    LayoutManager &manager = LayoutManager::getInstance();
    bool passed = manager.getRoot().isRoot();
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testAddChildReturnsNonNull()
{
    std::cout << "LayoutManager : addChild() returns non-null: ";
    LayoutManager &manager = LayoutManager::getInstance();
    Tree<Layout> *child = manager.addChild(
        &manager.getRoot(),
        Layout(vec2(0.0f, 0.0f), vec2(0.5f, 0.5f)));
    bool passed = (child != nullptr);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testAddChildIncreasesChildCount()
{
    std::cout << "LayoutManager : addChild() increases children count: ";
    LayoutManager &manager = LayoutManager::getInstance();
    std::size_t before = manager.getRoot().getChildren().size();
    manager.addChild(
        &manager.getRoot(),
        Layout(vec2(0.5f, 0.5f), vec2(1.0f, 1.0f)));
    std::size_t after = manager.getRoot().getChildren().size();
    bool passed = (after == before + 1);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testAddChildParentIsRoot()
{
    std::cout << "LayoutManager : addChild() parent pointer is root: ";
    LayoutManager &manager = LayoutManager::getInstance();
    Tree<Layout> *child = manager.addChild(
        &manager.getRoot(),
        Layout(vec2(0.0f, 0.0f), vec2(1.0f, 1.0f)));
    bool passed = (child->getParent() == &manager.getRoot());
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testRenderDoesNotCrash()
{
    std::cout << "LayoutManager : render() runs without crash: ";
    LayoutManager &manager = LayoutManager::getInstance();
    Screen screen(64, 64);
    manager.render(screen, ivec2(0, 0), ivec2(63, 63));
    std::cout << "PASS" << '\n';
}

void layoutManagerTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  LayoutManager : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testSingletonIdentity();
    testGetRootIsRoot();
    testAddChildReturnsNonNull();
    testAddChildIncreasesChildCount();
    testAddChildParentIsRoot();
    testRenderDoesNotCrash();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  LayoutManager tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
