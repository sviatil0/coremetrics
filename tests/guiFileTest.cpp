#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include "GUIFile.hpp"
#include "guiFileTest.hpp"
#include "GUIElements.hpp"
#include "GUIElementFactory.hpp"
#include "Tree.hpp"
#include "Layout.hpp"
#include "LayoutManager.hpp"

static bool fileContains(const std::string& fileName, const std::string& search)
{
    std::ifstream inFile(fileName);
    std::string line;

    if (inFile.is_open())
    {
        while (std::getline(inFile, line))
        {
            if (line.find(search) != std::string::npos)
            {
                return true;
            }
        }
    }
    return false;
}

static void testWriteFile()
{
    std::cout << "GUIFile writeFile: ";
    GUIFile f;
    std::string testFile = "unit_test_output.xml";

    LayoutManager& manager = LayoutManager::getInstance();
    manager.clear();

    Layout layout(vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), true);
    layout.addElement(GUIElementFactory::createPoint(vec2(480.0f, 270.0f), vec3(67.0f, 200.0f, 142.0f)));
    manager.addChild(&manager.getRoot(), std::move(layout));

    f.writeFile(testFile, manager);

    bool hasLayout = fileContains(testFile, "<layout>");
    bool hasPoint = fileContains(testFile, "<point>");
    bool hasValueOnOneLine = fileContains(testFile, "<x>480</x>");

    bool passed = hasLayout && hasPoint && hasValueOnOneLine;
    std::cout << (passed ? "PASS" : "FAIL") << '\n';

    if (std::filesystem::exists(testFile))
    {
        std::filesystem::remove(testFile);
    }

    manager.clear();
}

static void testFileRead()
{
    std::cout << "GUIFile readFile: ";
    GUIFile g;
    LayoutManager &manager = LayoutManager::getInstance();
    g.readFile("tests/ex1.xml", manager);

    std::vector<std::unique_ptr<Tree<Layout>>>& children = manager.getRoot().getChildren();
    size_t totalChildren = children.size();
    for (auto& child : children)
    {
        if (!(child->isLeaf())) totalChildren++;
    }
    size_t numLine = 0, numBox = 0, numPoint = 0, numLabel = 0, numButton = 0;
    bool layoutPassed = false;
    if (!children.empty()) {
        const Layout& layout = children[0]->getData();
        bool layoutPos = (layout.getStart().x == 0.25) && (layout.getStart().y == 0.25) && 
                        (layout.getEnd().x == 0.75) && (layout.getEnd().y == 0.75);
        bool layoutAttr = (layout.isActive()) && (layout.getName() == "outer");
        layoutPassed = layoutPos && layoutAttr;

        for (const auto& elemPtr : layout.elements) {
            if (dynamic_cast<Line*>(elemPtr.get())) 
            {
                numLine++;
            } 
            else if (dynamic_cast<Box*>(elemPtr.get())) 
            {
                numBox++;
            } 
            else if (dynamic_cast<Point*>(elemPtr.get())) 
            {
                numPoint++;
            } 
            else if (dynamic_cast<Label*>(elemPtr.get())) 
            {
                numLabel++;
            }
            else if (dynamic_cast<Button*>(elemPtr.get())) 
            {
                numButton++;
            }
        }
    }
    bool passed = ((totalChildren == 2) && (numLine == 1) && (numBox == 1) && (numPoint == 1) 
                    && (numLabel == 1) && (numButton == 1) && layoutPassed);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void guiFileTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  GUIFile - Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testWriteFile();
    testFileRead();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  GUIFile tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}