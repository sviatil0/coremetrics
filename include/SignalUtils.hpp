#ifndef __SIGNAL_UTILS_HPP__
#define __SIGNAL_UTILS_HPP__

#include <string>

// POSIX signal helpers used by the Processes tab's kill flow. Wraps the
// raw ::kill() syscall behind a typed signal enum, a name lookup, and a
// status-returning send() so the caller can show "no such process" or
// "permission denied" in the UI without parsing errno strings.
//
// On Windows, send() always returns SendStatus::Unsupported. A proper
// Windows backend would map SIGTERM / SIGKILL to TerminateProcess via
// OpenProcess(PROCESS_TERMINATE, pid) and is queued as a follow-up.
namespace SignalUtils
{
    enum class Signal
    {
        Term = 0,
        Kill = 1,
        Int = 2,
        Hup = 3,
        Stop = 4,
        Cont = 5,
    };

    enum class SendStatus
    {
        Ok = 0,
        NoSuchProcess = 1,
        PermissionDenied = 2,
        InvalidSignal = 3,
        InvalidPid = 4,
        Unsupported = 5,
    };

    // Short human label, e.g. "TERM", "KILL". Used inline in the signal
    // menu so the same string drives the picker label and the status flash.
    const char *name(Signal s);

    // Numeric POSIX signal value for the enum entry. Used by the platform
    // backend that calls ::kill(); the test suite also asserts a few
    // well-known equalities (SIGTERM == 15, SIGKILL == 9, ...).
    int posixNumber(Signal s);

    // Sends the signal to pid via ::kill on POSIX. Returns a typed status
    // so the UI can flash an appropriate footer message. Refuses pid <= 1
    // (init / kernel) up front to keep an accidental keystroke from
    // taking down the session.
    SendStatus send(int pid, Signal s);
}

#endif
