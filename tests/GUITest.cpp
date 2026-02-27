#include <iostream>
#include "../include/GUIFile.hpp"

static void testFileRead()
{
    std::cout << "File read: ";
    GUIFile g;
    g.readFile("ex1.xml");
    bool passed = g.getPoints().size() == 1
               && g.getLines().size() == 1
               && g.getBoxes().size() == 1;
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void GUIFileTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  GUIFile - Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testFileRead();
    

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  GUIFile tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}