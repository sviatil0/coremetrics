#ifndef __KILL_LOG_HPP__
#define __KILL_LOG_HPP__

#include <string>

// Append-only audit log for successful process kills performed from the
// Processes-tab signal menu. Every line is one kill event so a user can
// look back later and reconstruct exactly which pid/signal pair was sent
// at which UTC instant.
//
// Path:
//   POSIX:    $HOME/.config/coremetrics/kill.log
//   Windows:  %APPDATA%\coremetrics\kill.log
//
// Line format (one per call, newline-terminated):
//   2026-06-15T18:42:03Z TERM 12345 firefox
//
// The implementation silently swallows every I/O error (missing env var,
// disk full, permission denied, ...) because the live UI must keep
// running even if the audit trail cannot be written. The signal still
// went out either way; the audit line is a best-effort sidecar.
namespace KillLog
{
    void append(int pid,
                const std::string &name,
                const std::string &signalName);
}

#endif
