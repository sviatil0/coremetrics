#include <iostream>
#include <memory>
#include <type_traits>
#include "CloneableTest.hpp"
#include "Cloneable.hpp"
#include "GUIElement.hpp"
#include "Bar.hpp"
#include "Label.hpp"

void testCloneableProducesIndependentCopy()
{
    std::cout << "Cloneable : clone() through GUIElement* makes deep copy: ";
    Bar original(ivec2(0, 0), ivec2(100, 20), vec3(0.0f, 1.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 0.0f, 100.0f, "cpu");
    original.setValue(42.0f);

    GUIElement *base = &original;
    std::unique_ptr<GUIElement> copy(base->clone());

    Bar *copyAsBar = dynamic_cast<Bar *>(copy.get());
    bool passed = copyAsBar != nullptr
        && copyAsBar != &original
        && copyAsBar->getValue() == 42.0f
        && copyAsBar->getMetricName() == "cpu";

    original.setValue(7.0f);
    passed = passed && copyAsBar->getValue() == 42.0f;

    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testCloneableCovariantReturn()
{
    std::cout << "Cloneable : cloneDerived() returns Derived* (covariant): ";
    Label original("hello", ivec2(5, 5), vec3(1.0f, 1.0f, 1.0f));
    Label *clone = original.cloneDerived();

    static_assert(std::is_same_v<decltype(original.cloneDerived()), Label *>,
                  "cloneDerived must return the derived pointer type");

    bool passed = clone != nullptr
        && clone != &original
        && clone->getText() == "hello";

    delete clone;
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testCloneableUniqueHelper()
{
    std::cout << "Cloneable : cloneUnique<T>() yields unique_ptr<T>: ";
    Bar original(ivec2(0, 0), ivec2(50, 10), vec3(1.0f, 0.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 0.0f, 1.0f);
    original.setValue(0.5f);

    std::unique_ptr<Bar> owned = cloneUnique(original);
    bool passed = owned != nullptr
        && owned.get() != &original
        && owned->getValue() == 0.5f;

    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void cloneableTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Cloneable : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testCloneableProducesIndependentCopy();
    testCloneableCovariantReturn();
    testCloneableUniqueHelper();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Cloneable tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
