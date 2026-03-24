#include <iostream>
#include "Tree.hpp"
#include "TreeTest.hpp"

void testRootIsRoot()
{
    std::cout << "Tree : root isRoot(): ";
    Tree<int> root(42);
    bool passed = root.isRoot();
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testRootIsLeafInitially()
{
    std::cout << "Tree : root isLeaf() before addChild: ";
    Tree<int> root(1);
    bool passed = root.isLeaf();
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testAddChildNotLeaf()
{
    std::cout << "Tree : isLeaf() false after addChild: ";
    Tree<int> root(1);
    root.addChild(2);
    bool passed = !root.isLeaf();
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testAddChildGetData()
{
    std::cout << "Tree : addChild data stored correctly: ";
    Tree<int> root(1);
    Tree<int> *child = root.addChild(99);
    bool passed = (child->getData() == 99);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testAddChildParentPointer()
{
    std::cout << "Tree : child getParent() points to root: ";
    Tree<int> root(1);
    Tree<int> *child = root.addChild(2);
    bool passed = (child->getParent() == &root);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testChildIsNotRoot()
{
    std::cout << "Tree : child isRoot() is false: ";
    Tree<int> root(1);
    Tree<int> *child = root.addChild(2);
    bool passed = !child->isRoot();
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testChildrenCount()
{
    std::cout << "Tree : getChildren() size matches addChild calls: ";
    Tree<int> root(0);
    root.addChild(1);
    root.addChild(2);
    root.addChild(3);
    bool passed = (root.getChildren().size() == 3);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testMultiLevelNesting()
{
    std::cout << "Tree : grandchild parent chain correct: ";
    Tree<int> root(1);
    Tree<int> *child = root.addChild(2);
    Tree<int> *grandchild = child->addChild(3);
    bool passed = (grandchild->getParent() == child) && (grandchild->getParent()->getParent() == &root);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testRootParentIsNull()
{
    std::cout << "Tree : root getParent() is nullptr: ";
    Tree<int> root(5);
    bool passed = (root.getParent() == nullptr);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void treeTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Tree : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testRootIsRoot();
    testRootIsLeafInitially();
    testAddChildNotLeaf();
    testAddChildGetData();
    testAddChildParentPointer();
    testChildIsNotRoot();
    testChildrenCount();
    testMultiLevelNesting();
    testRootParentIsNull();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Tree tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
