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

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  EventManager tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
