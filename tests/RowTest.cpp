#include <iostream>
#include "RowTest.hpp"
#include "Row.hpp"
#include "screen.hpp"

void testRowConstruction()
{
    std::cout << "Row : construction stores cells and weights: ";
    std::vector<std::string> cells = {"PID", "NAME", "CPU"};
    std::vector<float> weights = {0.2f, 0.6f, 0.2f};
    Row row(ivec2(0, 0), ivec2(300, 20), cells, weights, vec3(1.0f, 1.0f, 1.0f));
    bool passed = (row.getCells().size() == 3 && row.getColumnWeights().size() == 3);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testRowSetCells()
{
    std::cout << "Row : setCells replaces contents: ";
    std::vector<std::string> initial = {"a", "b"};
    std::vector<float> weights = {0.5f, 0.5f};
    Row row(ivec2(0, 0), ivec2(100, 20), initial, weights, vec3(1.0f, 1.0f, 1.0f));
    std::vector<std::string> updated = {"x", "y"};
    row.setCells(updated);
    bool passed = (row.getCells()[0] == "x" && row.getCells()[1] == "y");
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testRowGetters()
{
    std::cout << "Row : getMinPos/getMaxPos return stored coords: ";
    Row row(ivec2(10, 20), ivec2(310, 40), {"a"}, {1.0f}, vec3(1.0f, 1.0f, 1.0f));
    ivec2 minP = row.getMinPos();
    ivec2 maxP = row.getMaxPos();
    bool passed = (minP.x == 10 && minP.y == 20 && maxP.x == 310 && maxP.y == 40);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testRowDrawDoesNotCrash()
{
    std::cout << "Row : draw does not crash: ";
    Screen screen(256, 64);
    Row row(ivec2(0, 0), ivec2(200, 20),
            {"PID", "NAME", "CPU"},
            {0.2f, 0.6f, 0.2f},
            vec3(1.0f, 1.0f, 1.0f));
    row.draw(screen);
    std::cout << "PASS" << '\n';
}

void testRowDrawEmptyCells()
{
    std::cout << "Row : draw handles empty cells: ";
    Screen screen(256, 64);
    Row row(ivec2(0, 0), ivec2(200, 20), {}, {}, vec3(1.0f, 1.0f, 1.0f));
    row.draw(screen);
    std::cout << "PASS" << '\n';
}

void testRowDrawZeroWeights()
{
    std::cout << "Row : draw handles zero total weight: ";
    Screen screen(256, 64);
    Row row(ivec2(0, 0), ivec2(200, 20),
            {"a", "b"}, {0.0f, 0.0f}, vec3(1.0f, 1.0f, 1.0f));
    row.draw(screen);
    std::cout << "PASS" << '\n';
}

void testRowDrawMismatchedSizes()
{
    std::cout << "Row : draw handles more cells than weights: ";
    Screen screen(256, 64);
    Row row(ivec2(0, 0), ivec2(200, 20),
            {"a", "b", "c"}, {1.0f}, vec3(1.0f, 1.0f, 1.0f));
    row.draw(screen);
    std::cout << "PASS" << '\n';
}

void rowTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Row : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testRowConstruction();
    testRowSetCells();
    testRowGetters();
    testRowDrawDoesNotCrash();
    testRowDrawEmptyCells();
    testRowDrawZeroWeights();
    testRowDrawMismatchedSizes();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Row tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
