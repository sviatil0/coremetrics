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

static void testSetAndGetPoint()
{
    std::cout << "GUIFile setPoint / getPoints: ";
    GUIFile f;
    Point p(vec2(10.0f, 20.0f), vec3(255.0f, 0.0f, 0.0f));
    f.setPoint(p);
    
    auto points = f.getPoints();
    bool passed = (points.size() == 1 && points[0].pos.x == 10.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testSetAndGetLine()
{
    std::cout << "GUIFile setLine / getLines: ";
    GUIFile f;
    Line l(vec2(0.0f, 0.0f), vec2(50.0f, 50.0f), vec3(0.0f, 255.0f, 0.0f));
    f.setLine(l);
    
    auto lines = f.getLines();
    bool passed = (lines.size() == 1 && lines[0].end.y == 50.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testSetAndGetBox()
{
    std::cout << "GUIFile setBox / getBoxes: ";
    GUIFile f;
    Box b(vec2(5.0f, 5.0f), vec2(15.0f, 15.0f), vec3(0.0f, 0.0f, 255.0f));
    f.setBox(b);
    
    auto boxes = f.getBoxes();
    bool passed = (boxes.size() == 1 && boxes[0].maxPos.x == 15.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testGetterDataProtection()
{
    std::cout << "GUIFile Getters (Data Protection): ";
    GUIFile f;
    f.setPoint(Point(vec2(1.0f, 1.0f), vec3(255.0f, 255.0f, 255.0f)));
    
    std::vector<Point> pts = f.getPoints();
    pts.clear(); 
    
    bool passed = (f.getPoints().size() == 1);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

static void testWriteFile()
{
    std::cout << "GUIFile writeFile: ";
    GUIFile f;
    std::string testFile = "unit_test_output.xml";
    
    f.setPoint(Point(vec2(480.0f, 270.0f), vec3(67.0f, 200.0f, 142.0f)));
    f.writeFile(testFile);
    
    bool hasLayout = fileContains(testFile, "<layout>");
    bool hasPoint = fileContains(testFile, "  <point>");
    bool hasValueOnOneLine = fileContains(testFile, "<x>480</x>");
    
    bool passed = hasLayout && hasPoint && hasValueOnOneLine;
    std::cout << (passed ? "PASS" : "FAIL") << '\n';

    if (std::filesystem::exists(testFile)) 
    {
        std::filesystem::remove(testFile);
    }
}

static void testFileRead()
{
    std::cout << "GUIFile readFile: ";
    GUIFile g;
    g.readFile("tests/ex1.xml");

    LayoutManager &manager = LayoutManager::getInstance();
    std::vector<std::unique_ptr<Tree<Layout>>>& children = manager.getRoot().getChildren();
    size_t totalChildren = children.size();
    for (auto& child : children)
    {
        if (!(child->isLeaf())) totalChildren++;
    }
    size_t numLine = 0, numBox = 0, numPoint = 0;
    bool layoutPassed = false;
    if (!children.empty()) {
        const Layout& layout = children[0]->getData();
        bool layoutPos = (layout.getStart().x == 0.25) && (layout.getStart().y == 0.25) && 
                        (layout.getEnd().x == 0.75) && (layout.getEnd().y == 0.75);
        bool layoutAttr = (layout.isActive()) && (layout.getName() == "outer");
        layoutPassed = layoutPos && layoutAttr;

        for (const auto& elemPtr : layout.elements) {
            if (dynamic_cast<Line*>(elemPtr.get())) {
                numLine++;
            } else if (dynamic_cast<Box*>(elemPtr.get())) {
                numBox++;
            } else if (dynamic_cast<Point*>(elemPtr.get())) {
                numPoint++;
            }
        }
    }
    bool passed = ((totalChildren == 2) && (numLine == 1) && (numBox == 1) && (numPoint == 1) && layoutPassed);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void guiFileTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  GUIFile - Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testSetAndGetPoint();
    testSetAndGetLine();
    testSetAndGetBox();
    testGetterDataProtection();
    testWriteFile();
    testFileRead();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  GUIFile tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}