#include <iostream>
#include "ModalTest.hpp"
#include "Modal.hpp"
#include "screen.hpp"

static Modal makeDefaultModal()
{
    return Modal(640, 480, 320, 160,
                 "Title", "Body line",
                 vec3(0.05f, 0.05f, 0.05f),
                 vec3(0.15f, 0.15f, 0.18f),
                 vec3(0.8f, 0.8f, 0.85f),
                 vec3(1.0f, 1.0f, 1.0f),
                 vec3(0.85f, 0.85f, 0.9f));
}

void testModalConstructionDefaultsHidden()
{
    std::cout << "Modal : construction defaults to hidden: ";
    Modal modal = makeDefaultModal();
    bool passed = (modal.isVisible() == false);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testModalSetVisibleRoundTrip()
{
    std::cout << "Modal : setVisible round-trip: ";
    Modal modal = makeDefaultModal();
    modal.setVisible(true);
    bool first = modal.isVisible();
    modal.setVisible(false);
    bool second = modal.isVisible();
    bool passed = (first == true && second == false);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testModalSetTitleAndBodyDoNotCrash()
{
    std::cout << "Modal : setTitle/setBody mutate without crash: ";
    Modal modal = makeDefaultModal();
    modal.setTitle("Confirm exit");
    modal.setBody("Quit CoreMetrics?");
    std::cout << "PASS" << '\n';
}

void testModalDrawVisible()
{
    std::cout << "Modal : draw when visible does not crash: ";
    Screen screen(640, 480);
    Modal modal = makeDefaultModal();
    modal.setVisible(true);
    modal.draw(screen);
    std::cout << "PASS" << '\n';
}

void testModalDrawHidden()
{
    std::cout << "Modal : draw when hidden does not crash: ";
    Screen screen(640, 480);
    Modal modal = makeDefaultModal();
    modal.setVisible(false);
    modal.draw(screen);
    std::cout << "PASS" << '\n';
}

void modalTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Modal : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testModalConstructionDefaultsHidden();
    testModalSetVisibleRoundTrip();
    testModalSetTitleAndBodyDoNotCrash();
    testModalDrawVisible();
    testModalDrawHidden();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  Modal tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
