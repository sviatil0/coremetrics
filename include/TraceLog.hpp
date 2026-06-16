#ifndef __TRACE_LOG_HPP__
#define __TRACE_LOG_HPP__

#include <string>

// Verbose, opt-in debug log for CoreMetrics. Default state is disabled;
// every TraceLog::log() call becomes a cheap atomic-bool check that
// returns immediately so production runs pay no I/O cost. Once a session
// has been started with the --trace CLI flag, the launcher calls
// enable() with the resolved log path and subsequent log() calls append
// a single ISO 8601 UTC timestamp + message line.
//
// Default path (POSIX):
//   $HOME/.config/coremetrics/trace.log
//
// Line format (one per call, newline-terminated):
//   2026-06-15T18:42:03Z some interesting event
//
// Thread-safety: enable / disable / log are all safe to call from any
// thread. The active flag is an atomic bool so the fast disabled-path
// stays lock-free; the actual write is serialized by an internal mutex
// so two threads cannot interleave bytes inside one log line.
//
// Error policy: every I/O failure (missing env var, disk full, perms,
// ...) is swallowed silently. A trace facility that crashes its host is
// worse than a missing audit line, and the user can grep the file at
// the end of the run to see whether tracing actually wrote anything.
namespace TraceLog
{
    // Opens the given file path in append mode and flips the active
    // flag on so subsequent log() calls produce output. Calling enable()
    // twice rebinds the stream to the new path without dropping the
    // active state; calling enable() on a path that cannot be opened
    // leaves the facility disabled.
    void enable(const std::string &path);

    // Closes the stream and flips the active flag off. Safe to call
    // even when the facility was never enabled.
    void disable();

    // True iff a previous enable() succeeded and disable() has not been
    // called since. Exposed so the launcher can short-circuit message
    // formatting on the hot path when tracing is off.
    bool isEnabled();

    // Appends "<ISO timestamp> <message>\n" to the active log file. A
    // no-op (returns immediately) when the facility is disabled, when
    // the stream is not open, or when the timestamp conversion fails.
    void log(const std::string &message);
}

#endif
