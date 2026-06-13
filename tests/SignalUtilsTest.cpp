#include <cstring>
#include <iostream>
#include "SignalUtilsTest.hpp"
#include "SignalUtils.hpp"

namespace
{
    int g_failures = 0;

    void report(const char *label, bool passed)
    {
        std::cout << "  " << label << ": " << (passed ? "PASS" : "FAIL") << '\n';
        if (!passed)
        {
            ++g_failures;
        }
    }
}

static void testNamesAreStable()
{
    bool passed = std::strcmp(SignalUtils::name(SignalUtils::Signal::Term), "TERM") == 0
                  && std::strcmp(SignalUtils::name(SignalUtils::Signal::Kill), "KILL") == 0
                  && std::strcmp(SignalUtils::name(SignalUtils::Signal::Int), "INT") == 0
                  && std::strcmp(SignalUtils::name(SignalUtils::Signal::Hup), "HUP") == 0
                  && std::strcmp(SignalUtils::name(SignalUtils::Signal::Stop), "STOP") == 0
                  && std::strcmp(SignalUtils::name(SignalUtils::Signal::Cont), "CONT") == 0;
    report("SignalUtils::name returns expected labels", passed);
}

static void testPosixNumbersAreCorrect()
{
    // These match the POSIX-defined values that ship on every supported
    // target (macOS Darwin, Linux glibc / musl). Asserting them here pins
    // the table so a future refactor cannot silently renumber the enum.
    bool passed = SignalUtils::posixNumber(SignalUtils::Signal::Hup) == 1
                  && SignalUtils::posixNumber(SignalUtils::Signal::Int) == 2
                  && SignalUtils::posixNumber(SignalUtils::Signal::Kill) == 9
                  && SignalUtils::posixNumber(SignalUtils::Signal::Term) == 15
                  && SignalUtils::posixNumber(SignalUtils::Signal::Cont) == 19;
    report("SignalUtils::posixNumber matches POSIX-defined values", passed);
}

static void testRejectsInvalidPids()
{
    bool passed = SignalUtils::send(0, SignalUtils::Signal::Term) == SignalUtils::SendStatus::InvalidPid
                  && SignalUtils::send(-1, SignalUtils::Signal::Term) == SignalUtils::SendStatus::InvalidPid
                  && SignalUtils::send(1, SignalUtils::Signal::Term) == SignalUtils::SendStatus::InvalidPid;
    report("SignalUtils::send rejects pid <= 1", passed);
}

static void testSignalToNonexistentPidReportsNoSuchProcess()
{
    // 0x7FFFFFFE is a synthetic pid well above any reasonable system; even
    // a heavily forked CI runner will not have claimed it. The send call
    // should reach ::kill, fail with ESRCH, and surface NoSuchProcess.
    SignalUtils::SendStatus s = SignalUtils::send(0x7FFFFFFE, SignalUtils::Signal::Term);
    bool passed = (s == SignalUtils::SendStatus::NoSuchProcess
                   || s == SignalUtils::SendStatus::Unsupported);
    report("SignalUtils::send to nonexistent pid surfaces NoSuchProcess (or Unsupported on Windows)",
           passed);
}

void signalUtilsTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  SignalUtils : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    g_failures = 0;

    testNamesAreStable();
    testPosixNumbersAreCorrect();
    testRejectsInvalidPids();
    testSignalToNonexistentPidReportsNoSuchProcess();

    std::cout << '\n';
    std::cout << "  Failures: " << g_failures << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  SignalUtils tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
