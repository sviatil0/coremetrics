#include <iostream>
#include "EventManagerTest.hpp"
#include "EventManager.hpp"
#include "LayoutManager.hpp"
#include "ClickEvent.hpp"
#include "ShowEvent.hpp"
#include "SoundEvent.hpp"

void testEventManagerSingleton()
{
    std::cout << "EventManager : getInstance() returns same instance: ";
    EventManager &first = EventManager::getInstance();
    EventManager &second = EventManager::getInstance();
    bool passed = (&first == &second);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testPushEventDoesNotCrash()
{
    std::cout << "EventManager : pushEvent() does not crash: ";
    EventManager &manager = EventManager::getInstance();
    manager.pushEvent(std::make_unique<ClickEvent>(10, 20));
    manager.processEvents(ivec2(0, 0), ivec2(63, 63));
    std::cout << "PASS" << '\n';
}

void testClickEventMissesEmpty()
{
    std::cout << "EventManager : click on empty layout does not crash: ";
    EventManager &manager = EventManager::getInstance();
    manager.pushEvent(std::make_unique<ClickEvent>(5, 5));
    manager.processEvents(ivec2(0, 0), ivec2(63, 63));
    std::cout << "PASS" << '\n';
}

void testShowEventUnknownName()
{
    std::cout << "EventManager : show event with unknown name does not crash: ";
    EventManager &manager = EventManager::getInstance();
    manager.pushEvent(std::make_unique<ShowEvent>("nonexistent", true));
    manager.processEvents(ivec2(0, 0), ivec2(63, 63));
    std::cout << "PASS" << '\n';
}

void testSoundEventMissingFile()
{
    std::cout << "EventManager : sound event with missing file does not crash: ";
    EventManager &manager = EventManager::getInstance();
    manager.pushEvent(std::make_unique<SoundEvent>("nonexistent.wav"));
    manager.processEvents(ivec2(0, 0), ivec2(63, 63));
    std::cout << "PASS" << '\n';
}

void testClickEventCreation()
{
    std::cout << "ClickEvent : stores mouse coordinates: ";
    ClickEvent event(42, 99);
    bool passed = (event.getMouseX() == 42 && event.getMouseY() == 99 && event.getType() == EVENT_CLICK);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testShowEventCreation()
{
    std::cout << "ShowEvent : stores layout name and show flag: ";
    ShowEvent event("overlay", true);
    bool passed = (event.getLayoutName() == "overlay" && event.getShow() == true && event.getType() == EVENT_SHOW);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testSoundEventCreation()
{
    std::cout << "SoundEvent : stores file path: ";
    SoundEvent event("assets/click.wav");
    bool passed = (event.getFilePath() == "assets/click.wav" && event.getType() == EVENT_SOUND);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testFindLayoutByName()
{
    std::cout << "EventManager : find layout by name: ";
    LayoutManager &manager = LayoutManager::getInstance();
    Tree<Layout> *child1 = manager.addChild(
        &manager.getRoot(),
        Layout(vec2(0.5f, 0.5f), vec2(1.0f, 1.0f), true, "first"));
    Tree<Layout> *child2 = manager.addChild(
        &manager.getRoot(),
        Layout(vec2(0.5f, 1.0f), vec2(1.0f, 1.0f), true, "second"));
    Tree<Layout> *child3 = manager.addChild(
        child1,
        Layout(vec2(0.5f, 1.0f), vec2(1.0f, 1.0f), true, "third"));
    // Fourth child kept in the tree (no local handle needed) so the
    // search exercises a node beyond the matching one.
    manager.addChild(
        child2,
        Layout(vec2(0.5f, 1.0f), vec2(1.0f, 1.0f), true, "fourth"));

    std::string name = "third";
    Tree<Layout> *found = EventManager::findLayoutByName(manager.getRoot(), name);
    bool passed = (child3 == found);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void eventManagerTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  EventManager : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testEventManagerSingleton();
    testPushEventDoesNotCrash();
    testClickEventMissesEmpty();
    testShowEventUnknownName();
    testSoundEventMissingFile();
    testClickEventCreation();
    testShowEventCreation();
    testSoundEventCreation();
    testFindLayoutByName();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  EventManager tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
