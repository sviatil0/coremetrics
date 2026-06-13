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
    // SIGHUP=1, SIGINT=2, SIGKILL=9, SIGTERM=15 are stable across every
    // supported target (Darwin, Linux glibc/musl, FreeBSD). SIGSTOP and
    // SIGCONT intentionally are NOT asserted here: their numeric values
    // differ between Darwin (17/19) and Linux (19/18). The map is
    // generated from the platform <signal.h>, so the runtime value is
    // already correct on each host; pinning the numbers would just be a
    // cross-platform footgun.
    bool passed = SignalUtils::posixNumber(SignalUtils::Signal::Hup) == 1
                  && SignalUtils::posixNumber(SignalUtils::Signal::Int) == 2
                  && SignalUtils::posixNumber(SignalUtils::Signal::Kill) == 9
                  && SignalUtils::posixNumber(SignalUtils::Signal::Term) == 15;
    report("SignalUtils::posixNumber matches stable POSIX values", passed);
}

static void testStopContAreNonzeroAndDistinct()
{
    // Looser assertion for the signals whose POSIX numbers vary between
    // Darwin and Linux: just confirm they're both non-zero and distinct
    // from each other. Anything else would re-introduce the cross-platform
    // hazard testPosixNumbersAreCorrect is documented to avoid.
    int s = SignalUtils::posixNumber(SignalUtils::Signal::Stop);
    int c = SignalUtils::posixNumber(SignalUtils::Signal::Cont);
    report("SignalUtils::posixNumber STOP and CONT are non-zero and distinct",
           s != 0 && c != 0 && s != c);
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
    testStopContAreNonzeroAndDistinct();
    testRejectsInvalidPids();
    testSignalToNonexistentPidReportsNoSuchProcess();

    std::cout << '\n';
    std::cout << "  Failures: " << g_failures << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  SignalUtils tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
