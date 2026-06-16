#include <iostream>
#include "HeatmapTest.hpp"
#include "Heatmap.hpp"
#include "Thresholds.hpp"
#include "screen.hpp"

void testHeatmapConstructionSingleRow()
{
    std::cout << "Heatmap : 8x1 grid initializes all values to 0: ";
    Heatmap hm(ivec2(0, 0), ivec2(160, 20), 8, 1, vec3(0.1f, 0.1f, 0.1f));
    bool passed = true;
    for (std::size_t c = 0; c < 8; ++c)
    {
        if (hm.getValue(0, c) != 0.0f)
        {
            passed = false;
        }
    }
    passed = passed && hm.getCols() == 8 && hm.getRows() == 1;
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testHeatmapConstructionMultiRow()
{
    std::cout << "Heatmap : 4x3 grid reports correct dimensions: ";
    Heatmap hm(ivec2(10, 20), ivec2(110, 80), 4, 3, vec3(0.0f, 0.0f, 0.0f));
    bool passed = (hm.getCols() == 4 && hm.getRows() == 3);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testHeatmapSetGetRoundTrip()
{
    std::cout << "Heatmap : setValue/getValue round-trip: ";
    Heatmap hm(ivec2(0, 0), ivec2(100, 50), 4, 2, vec3(0.0f, 0.0f, 0.0f));
    hm.setValue(0, 0, 12.5f);
    hm.setValue(0, 3, 75.0f);
    hm.setValue(1, 1, 90.0f);
    bool passed = (hm.getValue(0, 0) == 12.5f
                   && hm.getValue(0, 3) == 75.0f
                   && hm.getValue(1, 1) == 90.0f
                   && hm.getValue(1, 0) == 0.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testHeatmapSetValueClampLow()
{
    std::cout << "Heatmap : setValue clamps below 0: ";
    Heatmap hm(ivec2(0, 0), ivec2(100, 50), 2, 2, vec3(0.0f, 0.0f, 0.0f));
    hm.setValue(0, 0, -15.0f);
    bool passed = (hm.getValue(0, 0) == 0.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testHeatmapSetValueClampHigh()
{
    std::cout << "Heatmap : setValue clamps above 100: ";
    Heatmap hm(ivec2(0, 0), ivec2(100, 50), 2, 2, vec3(0.0f, 0.0f, 0.0f));
    hm.setValue(1, 1, 250.0f);
    bool passed = (hm.getValue(1, 1) == 100.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testHeatmapOutOfRangeAccess()
{
    std::cout << "Heatmap : out-of-range get returns 0: ";
    Heatmap hm(ivec2(0, 0), ivec2(100, 50), 2, 2, vec3(0.0f, 0.0f, 0.0f));
    hm.setValue(0, 0, 42.0f);
    bool passed = (hm.getValue(9, 9) == 0.0f && hm.getValue(2, 0) == 0.0f);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testHeatmapDrawDoesNotCrash()
{
    std::cout << "Heatmap : draw does not crash: ";
    Screen screen(256, 128);
    Heatmap hm(ivec2(8, 8), ivec2(200, 32), 8, 1, vec3(0.1f, 0.1f, 0.1f), 2);
    hm.setValue(0, 0, 10.0f);
    hm.setValue(0, 1, 35.0f);
    hm.setValue(0, 2, 65.0f);
    hm.setValue(0, 3, 85.0f);
    hm.setValue(0, 4, 99.0f);
    hm.draw(screen);
    std::cout << "PASS" << '\n';
}

void testHeatmapDrawDegenerateSize()
{
    std::cout << "Heatmap : draw handles degenerate cell size: ";
    Screen screen(64, 64);
    Heatmap hm(ivec2(0, 0), ivec2(2, 2), 8, 8, vec3(0.0f, 0.0f, 0.0f), 2);
    hm.draw(screen);
    std::cout << "PASS" << '\n';
}

void testHeatmapClone()
{
    std::cout << "Heatmap : clone preserves dimensions and values: ";
    Heatmap hm(ivec2(0, 0), ivec2(100, 25), 4, 1, vec3(0.0f, 0.0f, 0.0f));
    hm.setValue(0, 2, 55.0f);
    Heatmap *copy = hm.cloneDerived();
    bool passed = (copy != nullptr
                   && copy->getCols() == 4
                   && copy->getRows() == 1
                   && copy->getValue(0, 2) == 55.0f);
    delete copy;
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void heatmapTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Heatmap : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testHeatmapConstructionSingleRow();
    testHeatmapConstructionMultiRow();
    testHeatmapSetGetRoundTrip();
    testHeatmapSetValueClampLow();
    testHeatmapSetValueClampHigh();
    testHeatmapOutOfRangeAccess();
    testHeatmapDrawDoesNotCrash();
    testHeatmapDrawDegenerateSize();
    testHeatmapClone();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Heatmap tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
