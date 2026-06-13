#include "SignalUtils.hpp"

#if defined(__APPLE__) || defined(__linux__)
#include <signal.h>
#include <errno.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace SignalUtils
{

const char *name(Signal s)
{
    switch (s)
    {
    case Signal::Term: return "TERM";
    case Signal::Kill: return "KILL";
    case Signal::Int:  return "INT";
    case Signal::Hup:  return "HUP";
    case Signal::Stop: return "STOP";
    case Signal::Cont: return "CONT";
    }
    return "?";
}

int posixNumber(Signal s)
{
#if defined(__APPLE__) || defined(__linux__)
    switch (s)
    {
    case Signal::Term: return SIGTERM;
    case Signal::Kill: return SIGKILL;
    case Signal::Int:  return SIGINT;
    case Signal::Hup:  return SIGHUP;
    case Signal::Stop: return SIGSTOP;
    case Signal::Cont: return SIGCONT;
    }
    return 0;
#else
    // Windows: we still want the name table to work for the UI label even
    // though we cannot deliver the signal. The well-known POSIX values are
    // returned as constants so any future Windows backend that emulates
    // them can keep the same numeric API.
    switch (s)
    {
    case Signal::Term: return 15;
    case Signal::Kill: return 9;
    case Signal::Int:  return 2;
    case Signal::Hup:  return 1;
    case Signal::Stop: return 17;
    case Signal::Cont: return 19;
    }
    return 0;
#endif
}

SendStatus send(int pid, Signal s)
{
    // Reject pid 0 (current process group), pid -1 (broadcast), and pid 1
    // (init / launchd). Sending a signal to any of those by accident is
    // either nonsensical here or catastrophic; the kill flow in the
    // Processes tab should not have a path to either.
    if (pid <= 1)
    {
        return SendStatus::InvalidPid;
    }

#if defined(__APPLE__) || defined(__linux__)
    int sig = posixNumber(s);
    if (sig == 0)
    {
        return SendStatus::InvalidSignal;
    }
    int rc = ::kill(pid, sig);
    if (rc == 0)
    {
        return SendStatus::Ok;
    }
    switch (errno)
    {
    case ESRCH: return SendStatus::NoSuchProcess;
    case EPERM: return SendStatus::PermissionDenied;
    case EINVAL: return SendStatus::InvalidSignal;
    default:    return SendStatus::PermissionDenied;
    }
#elif defined(_WIN32)
    // Windows has no POSIX signals. Map the menu entries to the closest
    // analogs:
    //   TERM, KILL, INT, HUP -> TerminateProcess(handle, exitcode)
    //   STOP                 -> DebugBreakProcess(handle) (best-effort
    //                            "pause"; only works for processes the
    //                            current user can attach a debugger to)
    //   CONT                 -> rejected as Unsupported (no clean analog
    //                            without keeping a debugger attached, which
    //                            is well past this feature's scope)
    if (s == Signal::Cont)
    {
        return SendStatus::Unsupported;
    }

    DWORD access = PROCESS_TERMINATE;
    if (s == Signal::Stop)
    {
        access = PROCESS_ALL_ACCESS;
    }
    HANDLE h = OpenProcess(access, FALSE, static_cast<DWORD>(pid));
    if (h == nullptr)
    {
        DWORD err = GetLastError();
        if (err == ERROR_INVALID_PARAMETER)
        {
            return SendStatus::NoSuchProcess;
        }
        if (err == ERROR_ACCESS_DENIED)
        {
            return SendStatus::PermissionDenied;
        }
        return SendStatus::PermissionDenied;
    }

    BOOL ok = FALSE;
    if (s == Signal::Stop)
    {
        ok = DebugBreakProcess(h);
    }
    else
    {
        // Exit code 1 mirrors what `taskkill /F /PID <pid>` uses by default
        // so the killed process's exit reason looks the same to anything
        // watching its handle.
        ok = TerminateProcess(h, 1);
    }
    DWORD err = GetLastError();
    CloseHandle(h);

    if (ok)
    {
        return SendStatus::Ok;
    }
    if (err == ERROR_ACCESS_DENIED)
    {
        return SendStatus::PermissionDenied;
    }
    return SendStatus::PermissionDenied;
#else
    (void)s;
    return SendStatus::Unsupported;
#endif
}

}
